# Wireless Door Lock System

A low-cost, battery-powered wireless door lock system built with two ESP32 nodes communicating via ESP-NOW — no Wi-Fi router or internet connection required.

## Overview

The system consists of two ESP32 nodes:
- **Controller Node** — handles user input and sends lock/unlock commands
- **Lock Node** — receives commands and physically actuates the solenoid lock

Both nodes communicate directly using the ESP-NOW protocol, enabling fast and reliable peer-to-peer communication without any network infrastructure.

## Features

- Wireless control with no Wi-Fi dependency (ESP-NOW)
- Visual status feedback via 2×16 LCD display
- Audio feedback via buzzer
- Battery-powered (18650 2S Li-ion pack)
- MOSFET-driven solenoid for safe high-current switching
- Flyback diode protection for inductive load

## Hardware

| Component | Purpose |
|-----------|---------|
| 2× ESP32 | Main microcontrollers (both nodes) |
| Solenoid Lock | Physical locking mechanism |
| IRLZ34N | N-channel MOSFET to drive solenoid |
| 1N5819 | Flyback diode for solenoid protection |
| 18650 2S | Battery pack (power supply) |
| 2×16 LCD | Status display |
| Buzzer | Audio feedback on lock/unlock |

## Tech Stack

| Item | Detail |
|------|--------|
| Framework | ESP-IDF |
| Language | C |
| Protocol | ESP-NOW |
| IDE | VS Code + ESP-IDF Extension |

## Getting Started

### Prerequisites
- ESP-IDF v5.x installed
- VS Code with the ESP-IDF extension
- Two ESP32 development boards

### Setup

1. Clone the repository
```bash
git clone https://github.com/majidkhan-dev/wireless-door-lock-esp32
cd wireless-door-lock-esp32
```

2. Get the MAC address of the **lock node** ESP32 and update it in the controller firmware
```c
// controller/main/main.c
static uint8_t lock_node_mac[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};
```

3. Flash the **lock node** firmware
```bash
cd lock-node
idf.py build flash monitor
```

4. Flash the **controller** firmware
```bash
cd controller
idf.py build flash monitor
```

## How It Works

1. User triggers lock/unlock on the controller node
2. Controller sends an ESP-NOW packet directly to the lock node MAC address
3. Lock node receives the packet and drives the IRLZ34N MOSFET gate HIGH/LOW
4. MOSFET switches the solenoid coil — lock opens or closes
5. Status is shown on the 2×16 LCD and confirmed with a buzzer beep
6. The 1N5819 flyback diode suppresses the voltage spike when the solenoid de-energises

## License

MIT — feel free to use, modify, and distribute.
