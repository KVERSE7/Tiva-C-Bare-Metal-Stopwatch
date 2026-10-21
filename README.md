# ⏱️ Tiva C Bare-Metal Stopwatch

A hardware-interrupt-driven stopwatch implemented in **Embedded C** for the **Texas Instruments Tiva C Series TM4C123G LaunchPad**. Developed using Code Composer Studio (CCS) and TI TivaWare, this project demonstrates bare-metal programming through direct peripheral configuration, timer interrupts, GPIO interrupts, LCD interfacing, and UART communication.

The stopwatch tracks elapsed time with **10 ms resolution**, supports lap recording, displays timing information on a character LCD, and transmits status updates to a PC serial terminal.

## ✨ Features

- **Precise Timekeeping:** Uses the SysTick hardware timer to maintain elapsed time with 10 ms resolution.
- **Interrupt-Driven Controls:** Detects button presses through falling-edge GPIO interrupts.
- **Start/Stop Control:** SW1 toggles the stopwatch between running and stopped states.
- **Lap Recording:** SW2 captures the current elapsed time without interrupting the stopwatch.
- **RGB LED Status:** Green indicates running, red indicates stopped, and blue flashes when a lap is recorded.
- **16×2 Character LCD:** Displays the current time in `MM:SS.cc` format and the most recently recorded lap time.
- **UART Telemetry:** Sends status messages, timestamps, and lap records to a PC serial terminal.
- **Bare-Metal Implementation:** Uses hardware peripherals and interrupt handlers without requiring an operating system.

## 🛠️ Hardware Requirements

| Component | Description |
|---|---|
| Microcontroller | TI Tiva C Series TM4C123G LaunchPad |
| Display | HD44780-compatible character LCD |
| Clock source | 16 MHz main oscillator configuration |
| Development environment | Code Composer Studio (CCS) |
| Peripheral library | TI TivaWare for C Series |
| Serial terminal | PuTTY, Tera Term, or an equivalent terminal application |

### Pin Mapping

#### LCD Interface

The LCD operates in 4-bit mode.

| LCD Signal | Tiva C GPIO Pin |
|---|---|
| D4 | PD0 |
| D5 | PD1 |
| D6 | PD2 |
| D7 | PD3 |
| RS | PE2 |
| Enable (E) | PE3 |

#### UART Communication

| UART Signal | Tiva C GPIO Pin |
|---|---|
| UART0 RX | PA0 |
| UART0 TX | PA1 |

#### Push Buttons

| Button | GPIO Pin | Function |
|---|---|---|
| SW1 | PF4 | Start/stop toggle |
| SW2 | PF0 | Record lap time |

Both buttons use weak internal pull-up resistors and falling-edge interrupt detection. **PF0 is a locked pin** and must be unlocked in software before configuring it for GPIO input.

#### RGB LED

| LED Color | GPIO Pin | Indication |
|---|---|---|
| Red | PF1 | Stopwatch stopped |
| Blue | PF2 | Lap recorded |
| Green | PF3 | Stopwatch running |

## 🧠 System Architecture

The stopwatch uses a combination of hardware peripherals and interrupt service routines to manage timing and user input.

1. **SysTick Timer:** Generates periodic interrupts every 10 ms.
2. **Timing Logic:** Updates the elapsed-time counter while the stopwatch is running.
3. **GPIO Interrupts:** Detect button presses and trigger start/stop or lap-recording operations.
4. **LCD Interface:** Presents the current stopwatch time and the last recorded lap.
5. **RGB LED:** Provides visual feedback about the stopwatch state.
6. **UART0:** Sends status information and timing data to an external serial terminal.

The main timing mechanism is interrupt-driven, allowing the stopwatch to maintain its elapsed-time count independently of normal program execution.

## 📂 Project Development Phases

The project was developed incrementally in four phases, with each phase introducing and verifying additional functionality.

### Phase 1: Basic LCD Setup — `phase1.c`

- Initializes the character LCD in 4-bit mode.
- Configures the two display rows.
- Displays static text to verify the LCD wiring and initialization.

Example display:

```text
 STOPWATCH
 READY...
```

### Phase 2: SysTick Integration — `phase2.c`

- Configures the SysTick hardware timer to generate interrupts every 10 ms.
- Implements the `SysTickHandler`.
- Starts the stopwatch automatically at power-on.
- Updates the elapsed-time display on the LCD.

### Phase 3: Start/Stop Control — `phase3.c`

- Configures SW1 as a GPIO input with falling-edge interrupt detection.
- Implements start/stop toggle functionality.
- Uses the RGB LED to indicate the stopwatch state.
- Displays green while running and red while stopped.

### Phase 4: Complete Stopwatch — `main.c`

Integrates all major features into a complete application:

- Unlocks and configures PF0 for SW2.
- Implements interrupt-driven lap recording.
- Displays the most recent lap time on the second LCD row.
- Flashes the blue LED for approximately 100 ms after a lap is recorded.
- Initializes UART0 for serial communication.
- Transmits stopwatch status messages and formatted timestamps to a PC.

## 🚀 Getting Started

### 1. Install the Required Software

Install the following:

- [Code Composer Studio](https://www.ti.com/tool/CCSTUDIO)
- [TI TivaWare for C Series](https://www.ti.com/tool/SW-TM4C)

Ensure the TivaWare `driverlib` and `inc` directories are available and correctly configured in the CCS project.

### 2. Connect the Hardware

Connect the LCD data and control pins according to the pin mapping tables above.

The onboard buttons and RGB LED use the LaunchPad's existing hardware connections. Connect the LaunchPad to your PC using USB for programming and serial communication.

### 3. Import and Build the Project

1. Open Code Composer Studio.
2. Import or create the project for the TM4C123G LaunchPad.
3. Configure the required TivaWare include paths and library dependencies.
4. Select `main.c` for the complete Phase 4 implementation.
5. Build the project and resolve any configuration or compilation errors.
6. Connect the LaunchPad and flash the compiled program.

### 4. Configure the Serial Terminal

Open PuTTY, Tera Term, or another serial terminal and select the LaunchPad's virtual COM port.

| Setting | Value |
|---|---|
| Baud rate | 115200 |
| Data bits | 8 |
| Stop bits | 1 |
| Parity | None |
| Flow control | None |

The terminal should display stopwatch status messages and timing information as the application operates.

## 🎮 How to Use

1. **Start:** Press SW1 to start the stopwatch.
2. **Record a Lap:** Press SW2 while the stopwatch is running to capture the current elapsed time.
3. **View the Lap:** Read the most recently recorded lap time on the second LCD row or in the serial terminal.
4. **Stop:** Press SW1 again to pause the stopwatch.
5. **Resume:** Press SW1 again to continue timing from the previously recorded elapsed time.

### Example LCD Output

```text
