# Pankha — Scope of the Application

## Overview
**Pankha** is a unified hardware fan control and telemetry application designed specifically for 64-bit Windows systems. Its primary purpose is to consolidate disparate cooling channels across a PC into a centralized, low-latency control interface.

---

## In-Scope Capabilities

1. **Hardware Coverage**
   - **Motherboard Headers**: Detection and control of fan headers managed by Super I/O controllers (Nuvoton, ITE, Fintek, Winbond).
   - **CPU Cooling**: Monitoring and regulation of CPU fan headers and AIO pump tachometers.
   - **Discrete GPUs**: Readout and duty-cycle control for supported NVIDIA and AMD graphics card fans.
   - **Thermal Telemetry**: Aggregated real-time temperature polling across CPU cores, GPU dies, and motherboard thermistors.

2. **Control Modes**
   - **BIOS / Firmware Default**: Restoring hardware curve regulation back to native motherboard/GPU firmware.
   - **Manual Speed Override**: Direct manual PWM duty cycle control from 0% to 100%.
   - **Custom Fan Curves**: Mapping arbitrary temperature sensor inputs to dynamic fan duty response curves.

3. **System & Safety Features**
   - **Automated Calibration**: Stepping fan channels through duty cycle ranges to detect physical minimum and maximum operating RPM limits.
   - **Hardware Clamping Awareness**: Detecting and advising when motherboard firmware enforces minimum fan speeds or requires UEFI "Fan Stop" configuration.
   - **Thermal Safety Cutoff**: Automatic fail-safe threshold (80°C) that aborts overrides and restores BIOS defaults during unsafe temperatures.
   - **Driver Management**: Automated detection and silent provisioning of the required PawnIO kernel driver with Windows HVCI (Core Isolation) compliance.
   - **Windows Integration**: Zero-file registry configuration (`HKCU`), minimize-to-tray execution, and optional silent launch on Windows startup.

---

## Out of Scope

- **Non-Windows Operating Systems**: macOS and Linux are not supported (Linux natively uses `sysfs`/`hwmon`).
- **Overclocking & Voltage Tuning**: No manipulation of clock multipliers, power limits, or voltage offsets.
- **RGB & Lighting Control**: No synchronization or control of ARGB/RGB lighting protocols.
- **Proprietary Closed Ecosystems**: Proprietary USB-only controllers without standard SMBus/Super I/O interfaces or open driver access.
