# CMSIS RTX Threads - Embedded Real-Time System

A comprehensive embedded systems project demonstrating multi-threaded application development using the CMSIS RTX operating system on an ARM Cortex-M3 microcontroller.

## 📋 Project Overview

This project implements a real-time embedded system for the **STM32F103RB** microcontroller board (MCB STM32E). Developed for the Embedded Operating Systems (ECOS03) course at UNIFEI,  it showcases advanced threading concepts, real-time task management, and hardware interaction through multiple concurrent threads orchestrated by the CMSIS RTX RTOS.

### ✨ Key Features
- **Multi-threaded Architecture**: Multiple concurrent threads for independent subsystem management
- **Real-Time Scheduling**: CMSIS RTX kernel for deterministic task execution
- **Hardware Integration**: Real-time ADC sampling (potentiometer), LCD 16x2 display updates, and Buzzer frequency control
- **Lighting Patterns**: LED control implementing Sequential, Gray Code, and Parity-based animations.
- **Responsive User Interface**: 16-switch input monitoring with immediate visual and auditory feedback
- **Modular Design**: Separate threads for distinct functionalities


## 🎯 System Components

### Hardware Interfaces

#### LEDs (8 units)
- **Pins**: PA0–PA2, PA5–PA6, PA8, PA11, PA15
- **GPIO Port**: Port A
- Dynamic control through thread-based switching patterns

#### Push Buttons/Switches (16 units)
- **Pins**: PA3–PA4, PA7, PB3–PB5, PB8–PB15, PC13–PC15
- **GPIO Ports**: Port A, Port B, Port C
- Real-time input monitoring with debouncing

#### Analog Input
- **Potentiometer**: Connected to ADC1 (PB1)
- **ADC Configuration**: 12-bit resolution sampling
- Used for dynamic parameter adjustment (e.g., LED blinking speed)

#### Output Devices
- **Buzzer**: PB0 (PWM capable pin)
- **LCD Display**: 16×2 character display (HD44780 compatible)
  - RS Signal: PA15
  - EN Signal: PA12
  - Data Pins: PA5–PA6, PA8, PA11

### Clock Configuration
- **External Crystal**: 8 MHz
- **APB2 Peripheral Clock**: Enabled for GPIO and ADC1

## 🧵 Thread Architecture

The application implements the following concurrent threads:

| Thread | Function | Purpose |
|--------|----------|---------|
| `bot_thread` | Button Reading | Monitors all 16 push button inputs |
| `func1_thread` | LED Pattern 1 | Controls even-numbered LED pattern |
| `func2_thread` | LED Pattern 2 | Controls odd-numbered LED pattern |
| `func3_thread` | Binary to Gray | Converts binary to Gray code LED display |
| `func4_thread` | Potentiometer Driven | Controls LED pattern based on ADC value |
| `pot_thread` | Analog Reading | Reads potentiometer and updates global value |
| `vel_thread` | Velocity Control | Manages LED blinking speed adjustments |
| `lcd_thread` | Display Output | Updates LCD with system status |

## 🚀 Getting Started

### Prerequisites
- **Development Environment**: Keil MDK-ARM
- **Microcontroller**: STM32F103RB
- **Board**: MCB STM32E evaluation board
- **CMSIS**: ARM CMSIS-RTOS v1.x
- **Compiler**: ARM C Compiler

### Building the Project
1. Open `CMSISrtxThreads.uvprojx` in Keil MDK-ARM
2. Configure the build target: **Target 1 STM32F103RB**
3. Build the project (F7 or Project → Build)
4. Output files will be generated in the build directory

### Flashing to Device
1. Connect the evaluation board to your computer via USB
2. In Keil MDK, select **Flash → Download** (F8)

### Project Files
- `main.c`: Core application logic and thread definitions
- `Board_LED.h`: LED interface header (ARM CMSIS standard)
- `RTE/CMSIS/RTX_Conf_CM.c`: CMSIS RTX kernel configuration
- `RTE/Device/STM32F103RB/`: Device-specific includes and startup code
- `CMSISrtxThreads.uvprojx`: Keil project file

## 📊 System Behavior

### Initialization Sequence
1. Configure GPIO ports for LEDs, buttons, and peripherals
2. Initialize ADC1 for analog conversion
3. Set up LCD display
4. Create and start all threads
5. Begin CMSIS RTX kernel scheduling

### Runtime Operation
- **Button Handler**: Detects switch presses and triggers corresponding functions
- **LED Controllers**: Execute various patterns and animations
- **ADC Sampler**: Continuously reads potentiometer voltage
- **Display Manager**: Updates LCD with real-time system information
- **Velocity Adjuster**: Modulates LED blinking speed based on user input

### Timing Control
- Global velocity variable allows dynamic speed adjustment (default: 1000ms)
- Minimum velocity change interval: 1 second
- ADC sampling interval: Configurable per application needs

## 🔐 RTOS Features & Synchronization
To ensure determinism and safe resource sharing, the system leverages core RTOS primitives:

* **Signals (Event Flags)**: Extensive use of signals (e.g., `0x10`, `0x20`, `0x11`) to activate threads and `0x01-0x05` to stop them, allowing threads to remain in a low-power "Wait" state until needed.
* **Mutual Exclusion**: Implementation of task prioritization (from Normal to Real-time) to ensure critical UI updates and safety alarms always have CPU time.
* **Velocity Management**: Global `vel` variable shared across threads, with manual overrides (Buttons L/M/N) and dynamic Potentiometer control.

## 🔌 Hardware Pin Configuration

### Port A (GPIOA)
```
PA0-PA2   : LED1-LED3 (Output, Push-Pull)
PA3, PA4  : SW8, SW9 (Input, Pull-Up)
PA5-PA6, PA8, PA11 : LCD Data Lines (Output, Push-Pull)
PA7       : SW14 (Input, Pull-Up)
PA15      : LED4 / LCD RS (Output, Push-Pull)
```

### Port B (GPIOB)
```
PB0       : Buzzer (Output, Alternate Function)
PB1       : Potentiometer ADC (Analog Input)
PB3-PB5   : SW7, SW6, SW5 (Input, Pull-Up)
PB8-PB15  : SW10-SW13 (Input, Pull-Up)
```

### Port C (GPIOC)
```
PC13-PC15 : SW17, SW16, SW15 (Input, Pull-Up)
```

## 📄 License

This project contains components from ARM Limited, distributed under the BSD 3-Clause License. See individual file headers for specific licensing information.


---

*Created as a final project for ECOS03 - Sistemas Operacionais Embarcados at UNIFEI.*
