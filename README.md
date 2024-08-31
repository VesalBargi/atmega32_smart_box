# ATmega32 Smart Box Project

This is an Atmel Studio project that implements a smart box control system using the ATmega32 microcontroller, written in C. The project features a real-time clock display, temperature monitoring, alarm system, and automatic box lid operation controlled by a stepper motor and ultrasonic sensor. Additionally, a Proteus project file is provided for simulation and testing purposes.

## Features
 
- Time Display: Real-time clock shown on the LCD, updated every second. Time setting via serial command.
- Temperature Monitoring: The ambient temperature is monitored and displayed on the LCD.
- Alarm System: Customizable alarm using the DS3232 RTC, adjustable via serial command.
- Lid Control: The lid of the box is controlled by a stepper motor, providing precise lid operation.
- Automatic Lid Operation: The lid opens automatically when the ultrasonic sensor detects an object nearby.
- Auto-Close Function: Lid closes automatically after 10 seconds post-opening, controlled by a timer interrupt.
- Manual Button: Disables automatic lid function, keeping the lid open until pressed again to close.
- Help Command: Displays a list of supported commands for user reference.

## Hardware Components

- ATmega32 Microcontroller: Manages all the system's inputs, outputs, and operations.
- LCD Display: Shows the current date, time, temperature, and the distance measured by the ultrasonic sensor.
- DS3232 RTC Module: Provides accurate time and date for display and alarm functionality.
- Stepper Motor: Controls the precise opening and closing of the box lid.
- Buzzer: Used for the alarm system to emit sound notifications.
- Manual Button: Enables or disables the automatic lid function when pressed.
- Ultrasonic Sensor: Detects objects within 20 cm to trigger the automatic opening of the lid.
- Serial Port: Enables interaction with the system via UART for setting the time and configuring alarm.
  
## Libraries Used

- Liquid_crystal: Handles LCD communication.
- I2c_Lib: Low-level library for I2C communication.
- DS3232: Library for interfacing with the DS3232 RTC module.
- TWI: Another low-level I2C communication library.
- IO_Macros: Macros used by the TWI library.
- HCSR04: Calculates distance based on echo timing.
- Serial: Manages serial communication with interrupt support.

## Proteus Schematic

![Datapath](https://i.imgur.com/9V57PTz.png)

## Simulating the Project

1. Open the provided Proteus project file `SmartBox.pdsprj` in Proteus.
2. Select the ATmega32 component and open its properties.
3. In the properties window, locate the `Program File` section and click the folder icon.
4. Navigate to the `Debug` folder within your Atmel Studio project directory.
5. Select the `SmartBox.hex` file and click "Open."
6. Close the properties window.
7. Start the simulation by clicking the "Play" button in Proteus.

## Serial Commands

Set the time

```bash
set time HH:MM:SS MM/DD/YY
```

Set the alarm

```bash
set alarm HH:MM:SS
```

List available commands

```bash
help
```

## Notes

- The lid system assumes 90° motor rotation for full operation.
- The ultrasonic sensor detects objects within 20 cm to trigger the lid opening.
- The default alarm time is 12:00 AM. You can modify the time via serial commands.
- Ensure that the correct time and date are set initially to maintain accurate RTC operation.
- The manual override button disables automatic lid operations and keeps the lid open until reactivated.

## License

This project is licensed under the MIT License.

[![MIT License](https://img.shields.io/badge/License-MIT-green.svg)](https://choosealicense.com/licenses/mit/)
