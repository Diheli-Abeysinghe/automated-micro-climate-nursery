# Automated Commercial Micro-Climate Nursery

An ESP32-based embedded IoT prototype designed to monitor and regulate
environmental conditions within a miniature plant nursery.

## Overview

The system monitors temperature, humidity, ambient light and soil
moisture. Based on these measurements, the ESP32 autonomously controls
ventilation, supplemental grow lighting and irrigation.

Environmental readings and actuator states are also periodically
uploaded to ThingSpeak for remote monitoring and historical data
logging. Environmental control remains local to the ESP32 so that the
prototype can continue operating if network connectivity is unavailable.

## Features

- DHT22 temperature and humidity monitoring
- LDR-based ambient light monitoring
- Capacitive soil-moisture monitoring
- Servo-controlled automated ventilation
- Temperature hysteresis
- Automatic grow-light control
- Soil-moisture-based irrigation
- Irrigation hysteresis
- Maximum pump runtime safety mechanism
- Manual override mode
- OLED environmental and system-status display
- Critical sensor fault detection and fail-safe operation
- Wi-Fi connectivity
- ThingSpeak cloud monitoring and historical data logging

## Hardware

- ESP32
- DHT22 temperature and humidity sensor
- LDR
- Capacitive soil-moisture sensor
- Servo motor
- Two LEDs
- I2C OLED display
- Push button
- 5 V relay module
- DC water pump
- 1N4007 flyback diode
- External battery supply

## Control Behaviour

### Ventilation

- Temperature >= 30 C: Vent opens
- Temperature <= 28 C: Vent closes
- Between 28 C and 30 C: Previous state retained

### Grow Lighting

Grow LEDs are automatically activated when the measured light level
falls below the configured threshold.

### Irrigation

- Soil moisture <= 30%: Irrigation begins
- Soil moisture >= 40%: Irrigation stops
- Between 30% and 40%: Previous pump state retained
- Maximum continuous pump runtime: 1 second

## Manual Override

The manual override button uses a latched toggle mechanism.

- First press: Manual Override enabled
- Second press: Manual Override disabled

During Manual Override:

- Ventilation panel: OPEN
- Grow LEDs: OFF
- Irrigation pump: OFF

## Fail-Safe Behaviour

If a critical DHT22 fault is detected:

- Ventilation panel opens
- Grow LEDs switch off
- Irrigation pump switches off
- A warning is displayed on the OLED

## Cloud Monitoring

The ESP32 periodically uploads the following data to ThingSpeak:

1. Temperature
2. Humidity
3. Light level
4. Soil moisture
5. Vent state
6. Grow-light state
7. Pump state
8. Operating mode

ThingSpeak is used for monitoring and historical data logging only.
Autonomous control remains local to the ESP32.

## Wokwi Simulation

https://wokwi.com/projects/475381934405977089

## Security
Wi-Fi credentials and ThingSpeak API credentials have intentionally
been excluded from the public source code. Users must provide their
own credentials before deploying the system.

## ThingSpeak Monitoring

The prototype uploads environmental measurements and actuator states
to ThingSpeak at approximately 20-second intervals.


## Repository Structure

```text
src/                  ESP32 source code
wokwi/                Wokwi simulation information
docs/system-design/   Architecture, control-flow and wiring diagrams
images/prototype/     Physical prototype photographs
images/testing/       Testing and ThingSpeak evidence
