# MIC-7700 Temperature-Controlled PWM Fan Controller

A compact temperature-controlled cooling system for an Advantech MIC-7700 industrial computer or a similar fanless computer/heatsink.

The controller uses an Arduino Pro Micro, a 10 kΩ NTC thermistor, and a four-wire Noctua PWM fan. Fan speed increases automatically as the heatsink temperature rises. Status LEDs indicate power, fan operation, excessive temperature, and sensor failure.

## Features

- Accepts **24 VDC input power**
- Uses two LM2596 converters:
  - **24 VDC → 12 VDC** for the fan
  - **24 VDC → 5 VDC** for the Arduino and logic
- Six fan states: 0%, 20%, 40%, 60%, 80%, and 100%
- Approximately **25 kHz PWM** for a four-wire PC fan
- Startup kick for reliable low-speed startup
- Temperature hysteresis to prevent rapid speed switching
- Full-speed fail-safe operation if the temperature sensor fails
- High-temperature panic mode
- Four status LEDs
- Serial diagnostic output
- Optional future fan-RPM monitoring

## Status LEDs

| LED | Behavior | Meaning |
|---|---|---|
| Green | Solid | Controller logic power is present |
| Yellow | Solid | Fan command is greater than 0% |
| Red | Blinking | Heatsink temperature is above the panic threshold |
| Blue | Blinking | Thermistor is disconnected, shorted, or producing an invalid reading |

The green LED is connected directly to the regulated 5 V supply. It therefore indicates actual logic power even if the Arduino firmware is not running.

## System Overview

```text
                         24 VDC INPUT
                               │
                 ┌─────────────┴─────────────┐
                 │                           │
          LM2596 converter            LM2596 converter
             24 V → 12 V                 24 V → 5 V
                 │                           │
          Noctua PWM fan              Arduino Pro Micro
                                             │
                                      Temperature sensor
                                      LEDs and PWM control
```

Both LM2596 converters are powered directly from the 24 V source. All grounds are connected together.

## Hardware

### Main Components

| Quantity | Component | Purpose |
|---:|---|---|
| 1 | Arduino Pro Micro, 5 V | Controller |
| 1 | Noctua NF-P14s redux-1200 PWM | 140 mm cooling fan |
| 2 | LM2596 adjustable buck converters | 24 V to 12 V and 24 V to 5 V |
| 1 | 10 kΩ NTC thermistor | Heatsink temperature sensor |
| 1 | 2N3904 NPN transistor | Open-collector PWM driver |
| 4 | LEDs: green, yellow, red, blue | Status indicators |
| 1 | Perfboard | Controller assembly |
| 1 | Four-pin connector | Fan connection |
| 1 | Two-pin connector | Thermistor connection |
| 1 | Two-pin connector | 24 V input |

### Resistors and Capacitors

| Reference | Value | Purpose |
|---|---:|---|
| R1 | 1 kΩ | Arduino-to-2N3904 base resistor |
| R2 | 10 kΩ | 2N3904 base pull-down |
| R3 | 10 kΩ | Thermistor voltage-divider resistor |
| R4 | 1 kΩ | Green LED current limiting |
| R5 | 1 kΩ | Yellow LED current limiting |
| R6 | 1 kΩ | Red LED current limiting |
| R7 | 1 kΩ | Blue LED current limiting |
| C1 | 100 nF, code `104` | Arduino supply bypass |
| C2 | 100 µF, 25 V | Fan 12 V supply smoothing |

A 470 Ω resistor may be used for the blue LED if it is too dim with 1 kΩ.

## Arduino Pin Assignments

| Pro Micro pin | Function |
|---|---|
| VCC | Regulated 5 V from the logic LM2596 |
| GND | Common ground |
| A0 | Thermistor-divider input |
| D9 | Fan PWM through R1 and Q1 |
| D16 | Yellow fan-running LED |
| D10 | Red high-temperature LED |
| D14 | Blue sensor-failure LED |
| RAW | Not connected |

D14 is used as an ordinary digital output.

## Fan Pinout

| Fan pin | Wire color | Function | Controller connection |
|---:|---|---|---|
| 1 | Black | Ground | Common ground |
| 2 | Yellow | +12 V fan power | 12 V LM2596 output |
| 3 | Green | Tachometer/RPM | Unused in the current version |
| 4 | Blue | PWM control | Q1 collector |

Do not apply 12 V to the blue PWM wire.

## Complete Connection Summary

### 24 V Input

```text
24 V input positive
├── 12 V LM2596 IN+
└── 5 V LM2596 IN+
```

Connect the 24 V input negative to the common ground bus.

### Common Ground

Connect all of these points together:

```text
24 V input negative
12 V LM2596 IN-
12 V LM2596 OUT-
5 V LM2596 IN-
5 V LM2596 OUT-
Arduino GND
Fan black wire
Thermistor ground connector
Q1 emitter
R2 ground end
C1 ground side
C2 negative leg
All LED cathodes
```

### 12 V Fan Supply

```text
12 V LM2596 OUT+
├── Fan yellow wire
└── C2 positive leg
```

Connect the negative leg of C2 to common ground. The stripe on an electrolytic capacitor identifies its negative side.

### 5 V Logic Supply

```text
5 V LM2596 OUT+
├── Arduino VCC
├── C1
├── R3
└── R4
```

Connect the other side of C1 to common ground. Do not connect the regulated 5 V output to the Arduino RAW pin.

## Thermistor Circuit

```text
5 V
 │
R3 10 kΩ
 │
 ├──────── Arduino A0
 │
 └──────── TEMP connector
               │
          10 kΩ NTC
               │
              GND
```

Connections:

```text
R3 first end  → regulated 5 V
R3 second end → Arduino A0 and TEMP connector
Thermistor    → between TEMP and GND
```

The thermistor is not polarized.

The firmware initially assumes:

- 10 kΩ resistance at 25°C
- Beta value of approximately 3950 K

Because the available thermistor is only labeled `10k`, calibration is recommended.

## PWM Transistor Circuit

```text
Arduino D9
    │
   R1
   1 kΩ
    │
    ├──────── Q1 base
    │
   R2
  10 kΩ
    │
   GND

Q1 collector ───── Fan blue PWM wire
Q1 emitter ─────── Common ground
```

R2 connects between the transistor base and ground. The emitter connects directly to ground; R2 is not placed in series with the emitter.

Verify the emitter, base, and collector order of the specific 2N3904 before soldering.

## LED Wiring

### Green Power LED

```text
5 V ── R4 1 kΩ ── Green LED ── GND
```

### Yellow Fan-Running LED

```text
Arduino D16 ── R5 1 kΩ ── Yellow LED ── GND
```

### Red High-Temperature LED

```text
Arduino D10 ── R6 1 kΩ ── Red LED ── GND
```

### Blue Sensor-Failure LED

```text
Arduino D14 ── R7 1 kΩ ── Blue LED ── GND
```

For a typical through-hole LED:

- Longer lead: anode/positive side
- Shorter lead: cathode/negative side
- Flat edge on the LED body: normally the cathode side

## Fan-Control Behavior

| Temperature | Fan command |
|---|---:|
| Below T1 | 0% |
| T1 to T2 | 20% |
| T2 to T3 | 40% |
| T3 to T4 | 60% |
| T4 to T5 | 80% |
| At or above T5 | 100% |

The current proposed thresholds are:

```cpp
constexpr float THRESHOLDS_C[5] = {
    35.0F,  // T1: 20%
    43.0F,  // T2: 40%
    51.0F,  // T3: 60%
    59.0F,  // T4: 80%
    67.0F   // T5: 100%
};
```

Suggested panic-temperature settings:

```cpp
constexpr float PANIC_TEMPERATURE_C = 72.0F;
constexpr float PANIC_CLEAR_TEMPERATURE_C = 68.0F;
```

These are starting values, not universal limits. Final thresholds should be selected after comparing the thermistor reading with CPU temperatures reported by the operating system.

## Hysteresis

Temperature hysteresis prevents the fan from rapidly switching between adjacent levels.

For example, with a 2°C hysteresis:

- The fan changes from 20% to 40% at 43°C.
- It does not return to 20% until the temperature falls below 41°C.

## Startup Kick

When the fan changes from stopped to running, the firmware briefly commands 100% before applying the requested lower speed. This helps the fan start reliably at a 20% PWM command.

## Fail-Safe Behavior

### Sensor Failure

The controller treats these conditions as sensor failures:

- Thermistor disconnected
- Thermistor wiring shorted
- ADC reading near either supply rail
- Invalid calculated temperature
- Temperature outside a reasonable measurement range

When a sensor failure is detected:

- Fan is forced to 100%
- Blue LED blinks
- Red LED remains off
- An error is printed to the serial monitor

### High-Temperature Panic

When the panic temperature is reached:

- Fan is forced to 100%
- Red LED blinks
- Blue LED remains off

The panic condition uses a separate lower clearing temperature to avoid rapidly entering and leaving alarm mode.

## Temperature Sensor Placement

Mount the thermistor on the heatsink:

- Near the base of a central heatsink fin
- Close to the solid heatsink body
- Near the expected hottest area
- Protected from direct fan airflow

A recommended temporary installation is:

1. Apply a tiny amount of thermal compound between the thermistor and heatsink.
2. Hold it in place using Kapton tape or aluminum foil tape.
3. Secure the thermistor wires separately for strain relief.
4. Cover the bead enough to prevent direct airflow from cooling the sensor independently of the heatsink.

Do not drill into thin heatsink fins. If a drilled installation is used, drill only a shallow blind hole in a known solid aluminum section and ensure that no electronics, heat pipes, or internal components can be reached.

## Power-Up Procedure

Before connecting the fan or Arduino:

1. Connect both LM2596 modules to the 24 V source.
2. Adjust the fan converter output to **12.0 V**.
3. Adjust the logic converter output to **5.0 V**.
4. Disconnect input power.
5. Connect the Arduino, fan, thermistor, LEDs, and transistor circuit.
6. Check for continuity between all intended ground points.
7. Check that there is no short between:
   - 24 V and ground
   - 12 V and ground
   - 5 V and ground
8. Apply power.
9. Verify 12 V and 5 V again under load.

## USB Programming Warning

The Arduino receives regulated 5 V directly through its VCC pin.

To avoid connecting the external 5 V supply and USB 5 V supply simultaneously, use this programming procedure:

```text
Turn off the 24 V supply
Connect USB
Upload the firmware
Disconnect USB
Turn on the 24 V supply
```

Whether simultaneous USB and external power are safe depends on the specific Pro Micro clone and its onboard power circuitry.

## Connectors

Standard 2.54 mm Dupont-style connectors are adequate for the low current used by this project.

| Connection | Connector |
|---|---|
| 24 V input | Two-pin |
| Fan | Four-pin |
| Thermistor | Two-pin |

Use strain relief and clearly mark connector polarity. Do not use Dupont connectors for exposed 120 VAC wiring.

## Wire Size

24 AWG stranded wire is sufficient for:

- 24 V input wiring
- LM2596 input and output wiring
- Fan power
- Arduino power
- LEDs and control signals

Use short, secure connections and a heavier ground or power bus on the perfboard when practical.

## Recommended Repository Structure

```text
mic7700-fan-controller/
├── README.md
├── firmware/
│   └── mic7700_fan_controller.ino
├── docs/
│   ├── pcb-top-view.png
│   ├── wiring-diagram.png
│   └── assembly-notes.md
└── LICENSE
```

## Firmware Installation

1. Open the `.ino` file in the Arduino IDE.
2. Select the correct **5 V Pro Micro** board profile.
3. Confirm that the selected board frequency matches the hardware.
4. Disconnect external 24 V power.
5. Connect the Pro Micro using USB.
6. Compile and upload the firmware.
7. Open the serial monitor at the baud rate configured in the sketch.
8. Disconnect USB before restoring external power.

The firmware calculates the Timer1 value from `F_CPU`, allowing the PWM setup to adapt to the clock frequency selected during compilation.

## Calibration

Before permanent use:

1. Measure ambient temperature with a trusted thermometer.
2. Compare it with the controller’s thermistor reading.
3. Run the MIC-7700 at idle and record:
   - Thermistor temperature
   - CPU temperature reported by Linux
4. Run a sustained CPU workload and record both temperatures again.
5. Verify that the fan changes through all requested speed levels.
6. Adjust:
   - Thermistor Beta value
   - Temperature thresholds
   - Panic threshold
   - Hysteresis

Do not treat an uncalibrated 10 kΩ thermistor as a precision temperature sensor.

## Future Improvements

- Fan tachometer/RPM monitoring
- Fan-stall alarm
- Second temperature sensor
- Configurable thresholds stored in EEPROM
- Serial configuration menu
- LinuxCNC alarm input
- Audible alarm
- Data logging
- Watchdog reset
- Dedicated PCB

## Safety

- This controller board is for low-voltage DC only.
- Do not place exposed 120 VAC wiring on the perfboard.
- Verify the polarity of both LM2596 outputs before connecting the Arduino or fan.
- Never connect 24 V or 12 V to the Arduino VCC pin.
- Never connect 12 V to the fan PWM wire.
- Confirm the 2N3904 pinout before soldering.
- Insulate the underside of the board from the computer chassis.
- Add strain relief to all external cables.
- Mount the board using insulated standoffs.

## License

Choose a license appropriate for the repository, such as MIT, GPL-3.0, or CERN Open Hardware Licence.

The hardware and firmware are provided without warranty. Verify all wiring and voltage levels before connecting the controller to valuable equipment.
