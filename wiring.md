# Fan Controller — Complete Net-by-Net Connection List

## Updated configuration

* Input power: **24 VDC**
* Converter U2: **24 V → 12 V** for the Noctua fan
* Converter U3: **24 V → 5 V** for the Pro Micro and control circuit
* Arduino: **Pro Micro 5 V**
* Fan: Noctua four-wire 12 V PWM fan
* Temperature sensor: 10 kΩ NTC thermistor
* PWM driver: 2N3904
* Status LEDs:

  * Green: power
  * Yellow: fan running
  * Red: high-temperature alarm
  * Blue: sensor failure

## Component labels

| Label | Component                                     |
| ----- | --------------------------------------------- |
| U1    | Arduino Pro Micro, 5 V                        |
| U2    | LM2596 adjusted to 12.0 V                     |
| U3    | LM2596 adjusted to 5.0 V                      |
| Q1    | 2N3904 NPN transistor                         |
| R1    | 1 kΩ, Q1 base resistor                        |
| R2    | 10 kΩ, Q1 base pull-down                      |
| R3    | 10 kΩ, thermistor divider resistor            |
| R4    | 1 kΩ, green LED resistor                      |
| R5    | 1 kΩ, yellow LED resistor                     |
| R6    | 1 kΩ, red LED resistor                        |
| R7    | 1 kΩ, blue LED resistor                       |
| C1    | 104 ceramic capacitor, 100 nF                 |
| C2    | 100 µF electrolytic capacitor, 25 V preferred |
| J1    | 24 V input connector                          |
| J2    | Four-pin fan connector                        |
| J3    | Two-pin thermistor connector                  |

---

# Net 1: +24V_INPUT

Connect all of these points together:

```text
J1 pin 1: +24V
U2 IN+
U3 IN+
```

Connection structure:

```text
24V supply positive
        │
        ├── U2 IN+  — 12V fan converter
        │
        └── U3 IN+  — 5V logic converter
```

Do not connect the fan directly to this 24 V net.

---

# Net 2: COMMON_GROUND

Connect all of these points together:

```text
J1 pin 2: GND

U2 IN-
U2 OUT-

U3 IN-
U3 OUT-

U1 Pro Micro GND
J2 fan GND
J3 thermistor GND

Q1 emitter
R2 one end

C1 one leg
C2 negative leg

Green LED cathode
Yellow LED cathode
Red LED cathode
Blue LED cathode
```

Everything in the controller must share this common ground:

```text
24V supply negative
        │
        ├── both LM2596 modules
        ├── Pro Micro
        ├── fan
        ├── thermistor
        ├── 2N3904
        ├── capacitors
        └── LEDs
```

The negative leg of C2 is identified by the stripe on the electrolytic capacitor body.

---

# Net 3: +12V_FAN

Connect all of these points together:

```text
U2 OUT+
J2 fan +12V
C2 positive leg
```

Diagram:

```text
U2 OUT+ at 12.0V
        │
        ├── Fan +12V
        └── C2 positive
```

C2 connection:

```text
+12V ── C2 100µF ── GND
```

Place C2 close to the fan connector.

The fan gets its power from U2 only. It does not get power from the 5 V converter or the Pro Micro.

---

# Net 4: +5V_LOGIC

Connect all of these points together:

```text
U3 OUT+
U1 Pro Micro VCC
C1 one leg
R3 one end
R4 one end
```

Diagram:

```text
U3 OUT+ at 5.0V
        │
        ├── Pro Micro VCC
        ├── C1 100nF
        ├── R3 thermistor-divider resistor
        └── R4 green-LED resistor
```

Do not connect the LM2596 5 V output to the Pro Micro RAW pin.

Use:

```text
U3 OUT+ → Pro Micro VCC
```

C1 connection:

```text
+5V ── C1 100nF ── GND
```

C1 is not polarized. Place it close to the Pro Micro VCC and GND pins.

---

# Thermistor circuit

The thermistor divider is:

```text
+5V
 │
R3 10kΩ
 │
 ├──────── Pro Micro A0
 │
 └──────── J3 TEMP
               │
         10kΩ NTC sensor
               │
              GND
```

Connections:

```text
R3 first end  → +5V_LOGIC
R3 second end → Pro Micro A0
R3 second end → J3 pin 1, TEMP

J3 pin 2      → GND
```

The external thermistor connects between:

```text
J3 pin 1: TEMP
J3 pin 2: GND
```

The NTC thermistor has no polarity.

---

# Fan connector J2

Use this order:

| J2 pin | Function       | Noctua wire |
| -----: | -------------- | ----------- |
|      1 | GND            | Black       |
|      2 | +12 V          | Yellow      |
|      3 | RPM/tachometer | Green       |
|      4 | PWM control    | Blue        |

Mark the connector clearly so it cannot be accidentally reversed.

## Fan RPM wire

The current Arduino program does not use the tachometer signal.

Therefore:

```text
J2 pin 3 → leave electrically unconnected
```

You may route it to an unused perfboard pad for future use. Do not connect it to ground or +5 V.

---

# PWM transistor circuit

Connections:

```text
Pro Micro D9
      │
     R1
     1kΩ
      │
      ├──────── Q1 base
      │
     R2
    10kΩ
      │
     GND
```

The transistor’s remaining connections:

```text
Q1 collector → J2 pin 4, fan PWM
Q1 emitter   → COMMON_GROUND
```

Complete circuit:

```text
Pro Micro D9 ── R1 1kΩ ──┬── Q1 base
                           │
                         R2 10kΩ
                           │
                          GND

Fan PWM ───────────────── Q1 collector
GND ───────────────────── Q1 emitter
```

## 2N3904 pin orientation

Connect by transistor function, not merely by physical position:

* **E — emitter:** GND
* **B — base:** junction of R1 and R2
* **C — collector:** fan PWM wire

Verify the pin order of your particular 2N3904 before soldering. Many TO-92 2N3904 parts are E-B-C when viewed from the flat side, but this should still be checked.

---

# Green power LED

The green LED is connected directly to the 5 V supply and is not controlled by software.

```text
+5V_LOGIC
    │
   R4
   1kΩ
    │
Green LED anode
Green LED cathode
    │
   GND
```

Connections:

```text
R4 first end       → +5V_LOGIC
R4 second end      → green LED anode
Green LED cathode  → GND
```

This LED remains illuminated whenever the Pro Micro’s 5 V supply is present.

---

# Yellow fan-running LED

Use Pro Micro pin D16:

```text
Pro Micro D16
      │
     R5
     1kΩ
      │
Yellow LED anode
Yellow LED cathode
      │
     GND
```

Connections:

```text
D16                → R5 first end
R5 second end      → yellow LED anode
Yellow LED cathode → GND
```

---

# Red high-temperature LED

Use Pro Micro pin D10:

```text
Pro Micro D10
      │
     R6
     1kΩ
      │
Red LED anode
Red LED cathode
      │
     GND
```

Connections:

```text
D10             → R6 first end
R6 second end   → red LED anode
Red LED cathode → GND
```

---

# Blue sensor-failure LED

Use Pro Micro pin D14:

```text
Pro Micro D14
      │
     R7
     1kΩ
      │
Blue LED anode
Blue LED cathode
      │
     GND
```

Connections:

```text
D14              → R7 first end
R7 second end    → blue LED anode
Blue LED cathode → GND
```

For all LEDs:

* Long lead is normally the anode.
* Short lead is normally the cathode.
* The flat side of the LED body normally marks the cathode.

The resistor can physically be placed before or after the LED, provided it is in series.

---

# Complete Pro Micro connection table

| Pro Micro pin | Connection                  |
| ------------- | --------------------------- |
| VCC           | U3 LM2596 5 V output        |
| GND           | Common ground               |
| A0            | Thermistor-divider junction |
| D9            | R1 → Q1 base, fan PWM       |
| D16           | Yellow fan-running LED      |
| D10           | Red high-temperature LED    |
| D14           | Blue sensor-failure LED     |
| RAW           | Not connected               |
| Other pins    | Not connected               |

---

# Complete connector table

## J1 — 24 V input

| Pin | Connection |
| --: | ---------- |
|   1 | +24 V      |
|   2 | GND        |

## J2 — fan

| Pin | Connection            |
| --: | --------------------- |
|   1 | GND                   |
|   2 | +12 V from U2         |
|   3 | RPM, currently unused |
|   4 | PWM from Q1 collector |

## J3 — thermistor

| Pin | Connection       |
| --: | ---------------- |
|   1 | TEMP/A0 junction |
|   2 | GND              |

---

# Simplified complete schematic

```text
                      +24V INPUT
                           │
                ┌──────────┴──────────┐
                │                     │
             U2 IN+                U3 IN+
          LM2596 12V             LM2596 5V
                │                     │
              +12V                   +5V
                │                     │
        ┌───────┴───────┐       ┌────┴───────────────┐
        │               │       │                    │
     Fan +12V       C2 100µF  Pro Micro VCC      C1 100nF
                        │                             │
                       GND                           GND

+5V ── R3 10k ──┬── A0
                 │
                 └── TEMP connector ── 10k NTC ── GND

+5V ── R4 1k ── Green LED ── GND

D16 ── R5 1k ── Yellow LED ── GND
D10 ── R6 1k ── Red LED ───── GND
D14 ── R7 1k ── Blue LED ──── GND

D9 ── R1 1k ──┬── Q1 base
               │
             R2 10k
               │
              GND

Fan PWM ───── Q1 collector
GND ───────── Q1 emitter

Fan GND ───── GND
Fan RPM ───── Unconnected
```

# Before connecting the Pro Micro or fan

1. Connect only the two LM2596 modules to 24 V.
2. Adjust U2 to exactly **12.0 V**.
3. Adjust U3 to approximately **5.0 V**.
4. Disconnect the 24 V supply.
5. Connect the fan, Arduino and other components.
6. Check resistance between +24 V and ground for an accidental short.
7. Check resistance between +12 V and ground.
8. Check resistance between +5 V and ground.
9. Apply power and verify all three voltages again.

## USB warning

Because the Pro Micro receives regulated 5 V directly on VCC, avoid connecting its USB cable while the external 5 V supply is active unless you have confirmed that your board safely isolates the two supplies.

For programming and initial testing, the safest procedure is:

```text
Turn off 24V power
Connect USB
Upload firmware
Disconnect USB
Turn on 24V power
```
