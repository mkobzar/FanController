/*
Pin assignment
Function	Pro Micro pin
Thermistor divider	A0
Fan PWM through 2N3904	D9
Yellow “fan running” LED	D4
Red high-temperature LED	D5
Blue sensor-failure LED	D6
Green power LED	Connected directly to VCC, not controlled by code

Normal behavior:

Indicator	Meaning
Green solid	Controller has power
Yellow solid	Fan command is above 0%
Red blinking	Temperature has reached panic level
Blue blinking	Thermistor is disconnected, shorted, or gives an invalid reading


LED wiring

Each LED needs its own resistor:

Pro Micro pin ── 1 kΩ ── LED anode
                           LED cathode ── GND

The LED’s longer lead is normally the anode. The shorter lead and flat side of the body normally identify the cathode.

For the green power LED:

Pro Micro VCC ── 1 kΩ ── green LED ── GND

The code works at either 8 MHz or 16 MHz because the PWM timer value is calculated from F_CPU. Select the actual Pro Micro version in the Arduino IDE before uploading.

One caution: verify the physical E-B-C pin order of your exact 2N3904. Many TO-92 2N3904 parts are E-B-C when viewed from the flat side, 
but manufacturers and substitutes can use different arrangements.

*/

#include <Arduino.h>
#include <math.h>

// ============================================================
// FAN CONTROLLER
//
// Arduino Pro Micro / ATmega32U4
// 10k NTC thermistor
// Noctua 4-wire PWM fan
// 2N3904 open-collector PWM driver
//
// Green LED: power, wired directly to VCC
// Yellow LED: fan running
// Red LED: high-temperature alarm
// Blue LED: temperature-sensor failure
// ============================================================


// ============================================================
// PIN ASSIGNMENTS
// ============================================================

constexpr uint8_t THERMISTOR_PIN = A0;

// D9 is Timer1 OC1A on the ATmega32U4.
constexpr uint8_t FAN_PWM_PIN = 9;

constexpr uint8_t YELLOW_LED_PIN = 14;
constexpr uint8_t RED_LED_PIN = 10;
constexpr uint8_t BLUE_LED_PIN = 16;


// ============================================================
// THERMISTOR CONFIGURATION
// ============================================================
//
// Divider wiring:
//
// VCC
//  |
// 10k fixed resistor
//  |
//  +---------- A0
//  |
// 10k NTC thermistor
//  |
// GND
//
// These settings assume:
//   10k resistance at 25°C
//   Beta value approximately 3950
//
// Your thermistor is unmarked except for "10k", so B3950 is
// initially an assumption. It can be calibrated later.
// ============================================================

constexpr float FIXED_RESISTOR_OHMS =
  10000.0F;

constexpr float THERMISTOR_NOMINAL_OHMS =
  10000.0F;

constexpr float NOMINAL_TEMPERATURE_C =
  25.0F;

constexpr float THERMISTOR_BETA =
  3950.0F;

// Values very near either ADC rail indicate a short circuit,
// disconnected thermistor, or broken wiring.
constexpr int ADC_MIN_VALID = 2;
constexpr int ADC_MAX_VALID = 1021;

constexpr uint8_t ADC_SAMPLE_COUNT = 16;

// Actual PWM command for levels 0–5.
constexpr uint8_t FAN_PERCENT[6] = {
  0,
  20,
  40,
  60,
  80,
  100
};

// ============================================================
// FAN LEVELS
// ============================================================

// 
constexpr float THRESHOLDS_C[5] = {
  40.0F,  // T1: 20%
  46.5F,  // T2: 40%
  53.0F,  // T3: 60%
  59.5F,  // T4: 80%
  66.0F   // T5: 100%
};

// constexpr float THRESHOLDS_C[5] = {
//   39.0F,  // T1: 20%
//   45.8F,  // T2: 40%
//   52.5F,  // T3: 60%
//   59.3F,  // T4: 80%
//   66.0F   // T5: 100%
// };

constexpr float LEVEL_HYSTERESIS_C = 1.0F;


// ============================================================
// PANIC TEMPERATURE
// ============================================================
//
// Panic begins at 72°C.
//
// It clears only after the temperature falls to 68°C,
// preventing the alarm from rapidly switching on and off.
// ============================================================

constexpr float PANIC_TEMPERATURE_C =
  72.0F;

constexpr float PANIC_CLEAR_TEMPERATURE_C =
  68.0F;

// ============================================================
// TIMING
// ============================================================

constexpr unsigned long TEMPERATURE_INTERVAL_MS =
  5000UL;

constexpr unsigned long ALARM_BLINK_INTERVAL_MS =
  400UL;

constexpr unsigned long FAN_STARTUP_KICK_MS =
  800UL;


// ============================================================
// CONTROLLER STATE
// ============================================================

uint8_t currentFanLevel = 5;
uint8_t currentFanPercent = 100;

bool sensorFault = false;
bool panicActive = false;

unsigned long lastTemperatureTime = 0;
unsigned long lastAlarmBlinkTime = 0;

bool alarmBlinkState = false;

enum class AlarmMode : uint8_t {
  None,
  HighTemperature,
  SensorFailure
};

AlarmMode previousAlarmMode = AlarmMode::None;


// ============================================================
// 25 kHz FAN PWM
// ============================================================
//
// Fan connections:
//
// Fan +12V  -> 12V supply
// Fan GND   -> common ground
// Fan PWM   -> 2N3904 collector
//
// 2N3904 emitter -> common ground
//
// Pro Micro D9 -> 1k resistor -> 2N3904 base
// 2N3904 base  -> 10k resistor -> ground
//
// The 2N3904 inverts the Arduino output:
//
// Arduino HIGH:
//   transistor ON
//   fan PWM line LOW
//
// Arduino LOW:
//   transistor OFF
//   fan internal pull-up makes PWM line HIGH
//
// A fan PWM HIGH condition means "run."
// ============================================================

void setupFanPwm() {
  pinMode(FAN_PWM_PIN, OUTPUT);

  // Configure Timer1 for Fast PWM mode 14.
  //
  // TOP = ICR1
  // Prescaler = 1
  //
  // At 16 MHz:
  //   TOP = 639
  //
  // At 8 MHz:
  //   TOP = 319
  //
  // Both produce approximately 25 kHz.

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  TCCR1A |= _BV(WGM11);

  TCCR1B |=
    _BV(WGM13) | _BV(WGM12) | _BV(CS10);

  ICR1 = static_cast<uint16_t>(
    (F_CPU / 25000UL) - 1UL);
}


void setFanPercent(uint8_t percent) {
  if (percent > 100) {
    percent = 100;
  }

  currentFanPercent = percent;

  // Yellow LED indicates that the fan has a nonzero command.
  digitalWrite(
    YELLOW_LED_PIN,
    percent > 0 ? HIGH : LOW);

  if (percent == 0) {
    // Disconnect Timer1 from D9.
    TCCR1A &=
      ~(_BV(COM1A1) | _BV(COM1A0));

    // Arduino HIGH turns on the transistor.
    // Fan PWM line is held LOW.
    digitalWrite(FAN_PWM_PIN, HIGH);
    return;
  }

  if (percent >= 100) {
    // Disconnect Timer1 from D9.
    TCCR1A &=
      ~(_BV(COM1A1) | _BV(COM1A0));

    // Arduino LOW turns off the transistor.
    // Fan internal pull-up holds PWM HIGH.
    digitalWrite(FAN_PWM_PIN, LOW);
    return;
  }

  // Inverting OC1A mode:
  //
  // Arduino output is LOW from BOTTOM until compare match.
  // The external NPN transistor inverts that signal.
  //
  // The resulting fan-side HIGH time corresponds to the
  // requested percentage.

  TCCR1A |=
    _BV(COM1A1) | _BV(COM1A0);

  const uint32_t timerCounts =
    static_cast<uint32_t>(ICR1) + 1UL;

  uint32_t compareValue =
    timerCounts * static_cast<uint32_t>(percent) / 100UL;

  if (compareValue < 1UL) {
    compareValue = 1UL;
  }

  if (compareValue > ICR1) {
    compareValue = ICR1;
  }

  OCR1A =
    static_cast<uint16_t>(compareValue);
}


// ============================================================
// THERMISTOR READING
// ============================================================

bool readTemperatureC(float &temperatureC) {
  uint32_t adcTotal = 0;

  for (uint8_t sample = 0;
       sample < ADC_SAMPLE_COUNT;
       ++sample) {
    adcTotal += analogRead(THERMISTOR_PIN);
    delayMicroseconds(500);
  }

  const float adc =
    static_cast<float>(adcTotal) / static_cast<float>(ADC_SAMPLE_COUNT);

  // Detect disconnected or shorted sensor.
  if (adc <= ADC_MIN_VALID || adc >= ADC_MAX_VALID) {
    return false;
  }

  // For this divider:
  //
  // VCC -- fixed resistor -- A0 -- NTC -- GND
  //
  // Rntc = Rfixed × ADC / (1023 - ADC)

  const float thermistorResistance =
    FIXED_RESISTOR_OHMS * adc / (1023.0F - adc);

  if (!isfinite(thermistorResistance) || thermistorResistance <= 0.0F) {
    return false;
  }

  // Beta equation.

  float inverseKelvin =
    log(
      thermistorResistance / THERMISTOR_NOMINAL_OHMS)
    / THERMISTOR_BETA;

  inverseKelvin +=
    1.0F / (NOMINAL_TEMPERATURE_C + 273.15F);

  if (!isfinite(inverseKelvin) || inverseKelvin <= 0.0F) {
    return false;
  }

  temperatureC =
    (1.0F / inverseKelvin) - 273.15F;

  if (!isfinite(temperatureC)) {
    return false;
  }

  // Broad sanity check.
  if (temperatureC < -40.0F || temperatureC > 150.0F) {
    return false;
  }

  return true;
}


// ============================================================
// FAN LEVEL CONTROL
// ============================================================

void applyFanLevel(uint8_t requestedLevel) {
  if (requestedLevel > 5) {
    requestedLevel = 5;
  }

  const uint8_t requestedPercent =
    FAN_PERCENT[requestedLevel];

  if (requestedLevel == currentFanLevel && requestedPercent == currentFanPercent) {
    return;
  }

  const bool startingFromStopped =
    currentFanPercent == 0 && requestedPercent > 0;

  if (startingFromStopped) {
    // Brief full-speed command helps the fan start
    // reliably before reducing it to 20% or another
    // low setting.
    setFanPercent(100);
    delay(FAN_STARTUP_KICK_MS);
  }

  currentFanLevel = requestedLevel;
  setFanPercent(requestedPercent);
}


void updateFanLevel(float temperatureC) {
  uint8_t newLevel = currentFanLevel;

  // Increase speed immediately when an upper threshold
  // is crossed.
  while (
    newLevel < 5 && temperatureC >= THRESHOLDS_C[newLevel]) {
    ++newLevel;
  }

  // Reduce speed only after falling below the previous
  // threshold minus hysteresis.
  while (
    newLevel > 0 && temperatureC < (THRESHOLDS_C[newLevel - 1] - LEVEL_HYSTERESIS_C)) {
    --newLevel;
  }

  applyFanLevel(newLevel);
}


// ============================================================
// ALARM LED CONTROL
// ============================================================

AlarmMode getAlarmMode() {
  // Sensor failure has priority because the controller
  // no longer knows the actual temperature.
  if (sensorFault) {
    return AlarmMode::SensorFailure;
  }

  if (panicActive) {
    return AlarmMode::HighTemperature;
  }

  return AlarmMode::None;
}


void updateAlarmLeds() {
  const unsigned long now = millis();
  const AlarmMode alarmMode = getAlarmMode();

  if (alarmMode != previousAlarmMode) {
    previousAlarmMode = alarmMode;

    // Begin every new alarm with its LED illuminated.
    alarmBlinkState = true;
    lastAlarmBlinkTime = now;
  }

  if (alarmMode == AlarmMode::None) {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, LOW);
    return;
  }

  if (
    now - lastAlarmBlinkTime >= ALARM_BLINK_INTERVAL_MS) {
    lastAlarmBlinkTime = now;
    alarmBlinkState = !alarmBlinkState;
  }

  digitalWrite(
    RED_LED_PIN,
    (
      alarmMode == AlarmMode::HighTemperature && alarmBlinkState)
      ? HIGH
      : LOW);

  digitalWrite(
    BLUE_LED_PIN,
    (
      alarmMode == AlarmMode::SensorFailure && alarmBlinkState)
      ? HIGH
      : LOW);
}


// ============================================================
// SETUP
// ============================================================

void setup() {
  Serial.begin(9600);

  pinMode(THERMISTOR_PIN, INPUT);

  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);

  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(BLUE_LED_PIN, LOW);

  setupFanPwm();

  // Fail-safe startup:
  //
  // Run the fan at full speed until the first valid
  // temperature reading is obtained.
  currentFanLevel = 5;
  setFanPercent(100);

  // Cause an immediate temperature reading.
  lastTemperatureTime =
    millis() - TEMPERATURE_INTERVAL_MS;

  Serial.println(
    F("Temperature fan controller started"));

  Serial.println(
    F("Green: power"));

  Serial.println(
    F("Yellow: fan running"));

  Serial.println(
    F("Blinking red: high temperature"));

  Serial.println(
    F("Blinking blue: sensor failure"));
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop() {
  // Alarm blinking must be updated continuously.
  updateAlarmLeds();

  const unsigned long now = millis();

  if (
    now - lastTemperatureTime < TEMPERATURE_INTERVAL_MS) {
    return;
  }

  lastTemperatureTime = now;

  float temperatureC = NAN;

  sensorFault =
    !readTemperatureC(temperatureC);

  if (sensorFault) {
    // Unknown temperature: fail safe to full fan speed.
    panicActive = false;
    currentFanLevel = 5;
    setFanPercent(100);

    Serial.println(
      F(
        "ERROR: thermistor failure; "
        "fan forced to 100%"));

    return;
  }

  // High-temperature alarm with separate clearing
  // threshold.
  if (
    !panicActive && temperatureC >= PANIC_TEMPERATURE_C) {
    panicActive = true;
  } else if (
    panicActive && temperatureC <= PANIC_CLEAR_TEMPERATURE_C) {
    panicActive = false;
  }

  if (panicActive) {
    currentFanLevel = 5;
    setFanPercent(100);
  } else {
    updateFanLevel(temperatureC);
  }

  Serial.print(F("Temperature: "));
  Serial.print(temperatureC, 1);
  Serial.print(F(" C | level: "));
  Serial.print(currentFanLevel);
  Serial.print(F(" | fan: "));
  Serial.print(currentFanPercent);
  Serial.print(F("% | panic: "));

  if (panicActive) {
    Serial.println(F("YES"));
  } else {
    Serial.println(F("NO"));
  }
}
