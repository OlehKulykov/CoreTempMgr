# CoreTempManager

**CoreTempManager** is a high-performance Linux system daemon designed for intelligent CPU thermal management and workload orchestration. Developed specifically for multi-chiplet architectures (like AMD Ryzen), it balances system performance with thermal longevity using a deterministic Finite State Machine (FSM).

## 🚀 Key Features

* **Thermal-Aware Orchestration**: Dynamically manages an external service (via `fork`/`execvp`) based on real-time CPU temperature.
* **CCD-Aware Core Rotation**: Implements a unique core-pairing strategy to distribute heat across different Compute Complex Dies (CCDs), preventing "hot spots" on the silicon.
* **Dynamic P-State Management**: Interfaces with `amd-pstate-epp` to adjust frequency limits (`min`/`max`) in real-time, moving beyond static P-state switching.
* **Industrial-Grade Reliability**:
    * Uses `mlockall` to prevent memory paging to disk.
    * Signal-safe process management (handling `SIGCHLD`, `SIGTERM`, `SIGSTOP/SIGCONT`).
    * Synchronous command execution with built-in timeouts to prevent service hangs.
* **Gentoo Ready**: Optimized for heavy-load scenarios like `@world` updates, ensuring the system stays within the "green zone" (e.g., 42°C).

## 🛠 Architecture & Logic

The manager operates as a reactive system with the following thermal thresholds:

1.  **Stable Zone (< 39°C)**: Gradually increases frequency and activates additional core pairs every 5 minutes (`check_time_inc`) to maximize throughput.
2.  **Warning Zone (>= 40°C)**: Begins stepping down frequency or offloading cores every 60 seconds (`check_time_dec`) to stabilize temperature.
3.  **Critical Zone (>= 42°C)**: Triggers immediate safety protocols—drops frequency to minimum, pauses or stops the external workload, and activates maximum cooling.

### Core Rotation Strategy
The daemon utilizes a "Core Pairing" algorithm. By pairing cores from different physical dies (e.g., Core 0 from CCD1 and Core 6 from CCD2), it ensures optimal heat dissipation across the integrated heat spreader (IHS).

## 📋 Configuration

The behavior is fully customizable via a JSON configuration file:

```json
{
    "check_time_inc": 300, // tracking time < 'temp_max_stable' & inc
    "check_time_dec": 60,  // tracking time >= 'temp_max' & dec
    "temp_max_stable": 39, // temp same during 'check_time_inc'
    "temp_max": 40,        // >= at any time & down
    "temp_crit": 42,       // >= at any time & (set min freq & pause|stop)
    "freq_min": 1000,    // pause|stop, critical, wait, <= 'cores_start'
    "freq_start": 1200,  // init|start app, = 'cores_start'
    "freq_overcl": 1600, // temp < 'temp_max_stable' -> inc, <= 'cores_max_stable'
    "freq_exit": 1600,   // set on app exit
    "step_inc": 200,
    "step_dec": 200,
    "cores": [2,8, 3,9, 4,10, 5,11], // possible core pairs [{,},...]
    "cores_start":      2, // init|start, number of 'cores'
    "cores_max_stable": 4,
    "cores_swap_time":  3600,
    "set_freq_name": "cpupower",
    "set_freq_argv": [
        "frequency-set",
        "--governor=powersave",
        "--min=410MHz",
        "--max=%iMHz"
    ],
    "xname": "",
    "xconfig": "",
    "xargv": [
        "",
        ""
    ],
    "log": "/run/coretempmgr.log"
}
```

## License
Copyright (C) 2026 Oleh Kulykov <olehkulykov@gmail.com>
All Rights Reserved.

Unauthorized copying of this file, transferring or reproduction of the
contents of this project, via any medium, is strictly prohibited.
The contents of this project are proprietary and confidential.
