/*
 * Copyright (C) 2026 Oleh Kulykov <olehkulykov@gmail.com>
 * All Rights Reserved.
 *
 * Unauthorized copying of this file, transferring or reproduction of the
 * contents of this project, via any medium, is strictly prohibited.
 * The contents of this project are proprietary and confidential.
 */

#include <memory>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cerrno>
#include <climits>

#include "r2d9.hpp"

#include "ctm_manager.hpp"
#include "r2d9_logger.hpp"
#include "r2d9_trio.hpp"

#include "r2d9_c_string.h"

#include <sys/signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>

#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/signalfd.h>

static void printHelp(void) noexcept {
    static const char * helpText = "coretempmgr"
#if defined(VERSION)
    ", " VERSION
#endif
#if defined(R2D9_TIMESTAMP)
    ", " R2D9_TIMESTAMP
#elif defined(__DATE__) && defined(__TIME__)
    ", " __DATE__ " " __TIME__
#elif defined(__DATE__)
    ", " __DATE__
#elif defined(__TIME__)
    ", " __TIME__
#endif
    ":\n-h, --help   Display this help text and exit"
    "\n-c           <configuration file path>\n"
    "\nSignals:"
    "\n-SIGUSR1     Reload configuration from previously provided(-c) file path";
    ::fprintf(stdout, "%s\n", helpText);
}

static const char * parseArgs(int argc, const char * argv[]) noexcept {
    const char * path = nullptr;
    for (int i = 0; i < argc; i++) {
        const char * arg = argv[i];
        if (!arg) continue;
        
        if ((::strcmp(arg, "--help") == 0) || (::strcmp(arg, "-h") == 0)) {
            printHelp();
            std::exit(EXIT_SUCCESS);
        } else if ((::strcmp(arg, "-c") == 0) && ((i + 1) < argc)) {
            path = argv[++i];
        }
    }
    if (!path) {
        printHelp();
        std::exit(EXIT_FAILURE);
    }
    return path;
}

static void initManager(ctm::Manager & manager, const char * configPath) noexcept {
    try {
        auto config = std::make_unique<ctm::Config>();
        config->load(configPath);
        
        auto logger = std::make_unique<r2d9::Logger>();
        logger->open(config->logFilePath());
        
        auto sensors = std::make_unique<ctm::Sensors>();
        sensors->init();
        
        manager.init(static_cast<std::unique_ptr<ctm::Config> &&>(config),
                     static_cast<std::unique_ptr<ctm::Sensors> &&>(sensors),
                     static_cast<std::unique_ptr<r2d9::Logger> &&>(logger));
    } catch (const std::exception & exception) {
        const char * what = exception.what(), * formatStr = "Init manager exception, what: %s, errno: %i (%s)";
        const auto & logger = manager.logger();
        if (logger) {
            logger->log(r2d9::loggerTypeCritical, formatStr, (what ?: emptyCString), errno, ::strerror(errno));
        } else {
            r2d9::Logger::systemLog(r2d9::loggerTypeCritical, formatStr, (what ?: emptyCString), errno, ::strerror(errno));
        }
        std::exit(EXIT_FAILURE);
    }
}

static void deinitEPoll(const r2d9::TrioPOD<int, int, int> & epollFDs) noexcept {
    ::close(epollFDs.first);
    ::close(epollFDs.second);
    ::close(epollFDs.third);
}

static r2d9::TrioPOD<int, int, int> initEPoll(const r2d9::Logger & logger) noexcept {
    int epoll_fd = ::epoll_create(1);
    if (epoll_fd < 0) {
        logger.log(r2d9::loggerTypeCritical, "Create epoll, errno: %i (%s)", errno, ::strerror(errno));
        std::exit(EXIT_FAILURE);
    }
    
    int t_fd = ::timerfd_create(CLOCK_MONOTONIC, 0);
    if (t_fd < 0) {
        logger.log(r2d9::loggerTypeCritical, "Create timer, errno: %i (%s)", errno, ::strerror(errno));
        std::exit(EXIT_FAILURE);
    }
    
    struct itimerspec it;
    ::memset(&it, 0, sizeof(struct itimerspec));
    it.it_interval.tv_sec = 4;
    it.it_interval.tv_nsec = 0;
    it.it_value.tv_sec = 1;
    it.it_value.tv_nsec = 500000000; // 0.5 sec
    
    if (::timerfd_settime(t_fd, 0, &it, NULL) < 0) {
        logger.log(r2d9::loggerTypeCritical, "Set timer's time, errno: %i (%s)", errno, ::strerror(errno));
        std::exit(EXIT_FAILURE);
    }
    
    sigset_t mask;
    if (::sigemptyset(&mask) != 0) {
        logger.log(r2d9::loggerTypeCritical, "Error set empty signals set, errno: %i (%s)", errno, ::strerror(errno));
        std::exit(EXIT_FAILURE);
    }
    
    static const int signals[6] = {SIGHUP, SIGINT, SIGQUIT, SIGABRT, SIGTERM, SIGUSR1};
    for (size_t i = 0; i < 6; i++) {
        if (::sigaddset(&mask, signals[i]) != 0) {
            logger.log(r2d9::loggerTypeCritical, "Error add signal %i, errno: %i (%s)", signals[i], errno, ::strerror(errno));
            std::exit(EXIT_FAILURE);
        }
    }
    
    if (::sigprocmask(SIG_BLOCK, &mask, NULL) != 0) {
        logger.log(r2d9::loggerTypeCritical, "Error block signals set, errno: %i (%s)", errno, ::strerror(errno));
        std::exit(EXIT_FAILURE);
    }
    
    int s_fd = ::signalfd(-1, &mask, 0);
    if (s_fd < 0) {
        logger.log(r2d9::loggerTypeCritical, "Error create signals file descriptior, errno: %i (%s)", errno, ::strerror(errno));
        std::exit(EXIT_FAILURE);
    }
    
    struct epoll_event ev;
    ::memset(&ev, 0, sizeof(struct epoll_event));
    ev.events = EPOLLIN;
    ev.data.fd = t_fd;
    if (::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, t_fd, &ev) != 0) {
        logger.log(r2d9::loggerTypeCritical, "Error setup timer, errno: %i (%s)", errno, ::strerror(errno));
        std::exit(EXIT_FAILURE);
    }
    
    ::memset(&ev, 0, sizeof(struct epoll_event));
    ev.events = EPOLLIN;
    ev.data.fd = s_fd;
    if (::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, s_fd, &ev) != 0) {
        logger.log(r2d9::loggerTypeCritical, "Error setup signals, errno: %i (%s)", errno, ::strerror(errno));
        std::exit(EXIT_FAILURE);
    }
    
    r2d9::TrioPOD<int, int, int> res;
    res.first = t_fd;
    res.second = s_fd;
    res.third = epoll_fd;
    return res;
}

int main(int argc, const char * argv[]) {
#if defined(DEBUG)
    printHelp();
#endif
    
    parseArgs(argc, argv);
    
    if (::mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        r2d9::Logger::systemLog(r2d9::loggerTypeCritical, "Error lock all pages, errno: %i (%s)", errno, ::strerror(errno));
        return EXIT_FAILURE;
    }
    
    ctm::Manager manager;
    initManager(manager, parseArgs(argc, argv));
    manager.logger()->log(nullptr, emptyCString);
    
#if !defined(DEBUG)
    r2d9_stdio_to_dev_null();
#endif
    
    const auto epollFDs = initEPoll(*manager.logger());
    // epollFDs.first   t_fd
    // epollFDs.second  s_fd
    // epollFDs.third   epoll_fd
    
    bool running = true;
    while (running) {
        struct epoll_event events[2];
        const int nfds = ::epoll_wait(epollFDs.third, events, 2, -1);
        if (nfds < 0) {
            if (errno == EINTR) continue;
            manager.logger()->log(r2d9::loggerTypeCritical, "Error waiting events, errno: %i (%s)", errno, ::strerror(errno));
            break;
        }
        
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == epollFDs.first) {
                uint64_t expirations;
                if (::read(epollFDs.first, &expirations, sizeof(expirations)) > 0) {
                    try {
                        manager.tick();
                    } catch (const std::exception & exception) {
                        deinitEPoll(epollFDs);
                        manager.logger()->log(r2d9::loggerTypeCritical, "Tick error, what: %s, errno: %i (%s)", (exception.what() ?: emptyCString), errno, ::strerror(errno));
                        manager.onExit();
                        return EXIT_FAILURE;
                    }
                }
            } else if (events[i].data.fd == epollFDs.second) {
                struct signalfd_siginfo fdsi;
                fdsi.ssi_signo = 0;
                if (::read(epollFDs.second, &fdsi, sizeof(fdsi)) > 0) {
                    switch (fdsi.ssi_signo) {
                        case SIGHUP:
                        case SIGINT:
                        case SIGQUIT:
                        case SIGABRT:
                        case SIGTERM:
                            running = false;
                            manager.logger()->log(r2d9::loggerTypeInfo, "Received exit signal %i", fdsi.ssi_signo);
                            break;
                        case SIGUSR1:
                            try {
                                manager.reloadConfig();
                            } catch (const std::exception & exception) {
                                deinitEPoll(epollFDs);
                                manager.logger()->log(r2d9::loggerTypeCritical, "Reload config error, what: %s, errno: %i (%s)", (exception.what() ?: emptyCString), errno, ::strerror(errno));
                                manager.onExit();
                                return EXIT_FAILURE;
                            }
                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }
    
    deinitEPoll(epollFDs);
    manager.onExit();
    
    return EXIT_SUCCESS;
}
