# capture2SD — Arducam Mega on STM32F429 (NUCLEO-F429ZI)

Captures JPEG photos with an Arducam Mega camera and saves them to a FAT-formatted SD card. A new numbered file (`0.jpg`, `1.jpg`, …) is written every 5 seconds.

## Requirements

- [Arducam_Mega](https://github.com/ArduCAM/Arducam_Mega) library
- Arduino built-in [**SdFat - Adafruit Fork**](https://github.com/adafruit/SdFat) library

