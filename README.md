# Zone Temperature & Fan Control Unit

## Project Overview

A simplified single-zone HVAC control prototype based on an ESP32-S3 controller.

The system measures zone temperature and relative humidity using a DHT22 sensor, provides an operator-adjustable temperature setpoint through a 10 kΩ potentiometer, and automatically controls a 12 V DC cooling fan through a relay.

The project demonstrates concepts used in DDC/BMS-style HVAC control, including field inputs, control logic, actuator commands, status indication, alarm handling, fault response, and commissioning.

## System Architecture

<p align="center">
  DHT22 + Potentiometer<br>
  ↓<br>
  ESP32-S3 Controller<br>
  ↓<br>
  Fan Relay + Status LEDs<br>
  ↓<br>
  12 V DC Cooling Fan
</p>

The DHT22 provides temperature and humidity measurements. The potentiometer provides the operator-adjustable temperature setpoint. The ESP32-S3 evaluates these inputs and controls the fan relay and status indicators.

## Key Control Functions

* Temperature and relative humidity monitoring
* Adjustable temperature setpoint from 15°C to 30°C
* Fan control using ±0.5°C hysteresis
* High temperature deviation alarm at more than 5°C above setpoint
* Fan command and system status indication using LEDs
* Fan forced OFF during invalid control input conditions
* Automatic recovery after valid sensor input is restored
* Active-low relay control for the 12 V DC fan

## Hardware

* ESP32-S3
* DHT22 temperature and humidity sensor
* 10 kΩ potentiometer
* 5 V relay module
* 12 V DC cooling fan
* 12 V, 2 A DC power supply
* 12 V to 5 V buck converter
* Green, yellow, and red status LEDs
* 330 Ω LED current-limiting resistors

## Software

The firmware is developed using ESP-IDF.

The application is divided into separate modules for:

* DHT22 sensor handling
* Setpoint ADC input
* Fan control logic
* Status indicators
* Main application control flow

The control logic uses temperature hysteresis to prevent unnecessary relay switching.

## Documentation

Detailed engineering documentation is available in the docs folder:

* 01_Points_List.xlsx — Point definitions and I/O schedule
* 02_Sequence_of_Operations.docx — Control sequence, alarms, fault handling, and limitations
* 03_Electrical_Schematic.kicad_sch — Electrical schematic
* 04_Commissioning_Test_Sheet.xlsx — Commissioning and functional test results

## Commissioning

The prototype was tested for:

* Controller startup
* Temperature and setpoint acquisition
* Fan control and hysteresis operation
* High temperature alarm
* Sensor fault handling
* Sensor recovery
* Relay and 12 V fan operation

All defined commissioning tests passed.

## Limitations

This is a prototype for demonstrating basic HVAC control concepts. It is not intended to replace a commercial HVAC or building automation controller.

The system does not implement:

* Physical fan feedback
* Airflow proving
* Variable-speed fan control
* BACnet or Modbus communication
* Multi-zone control
* Production-level safety interlocks
* Redundant sensing or independent safety control

## Future Improvements

Possible extensions include:

* Fan feedback or current sensing
* Variable-speed fan control
* Multiple temperature zones
* BACnet or Modbus communication
* HMI or web-based operator interface
* Additional HVAC equipment control
* PCB implementation and enclosure design
