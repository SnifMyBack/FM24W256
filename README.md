# FM24W256 Arduino Library

[![License: MIT](https://img.shields.io)](https://opensource.org)
[![Status: Work in Progress](https://img.shields.io)]()

A heavily modified fork of the original FM24W256 library by Tjaart van Aswegen. This version is optimized for backing up, flashing, and testing FM24W256 FRAM ICs.

> [!CAUTION]
> **Use at your own risk.** This library is a work in progress. Incorrect use may result in data corruption or hardware damage. I am not responsible for any damage this library may incur.

## 🚀 Status: Beta
Currently, only the following functions are verified and working:
- `StreamWrite()`: Efficiently write sequential data blocks.
- `StreamRead()`: Efficiently read sequential data blocks.

**Note:** Other functions in the library are unverified and may cause unexpected behavior or corrupt your IC.

## 🛠 Features
- **Compatibility:** Tested on Arduino Uno R3; compatible with most Arduino-framework boards.
- **Hardware Support:** Specifically for FM24W256 (256Kbit I2C F-RAM). Likely works with other I2C memories (e.g., MB85RC series).
- **Fast Operations:** Designed specifically for bulk backup and flashing tasks.

## 🔌 Pinout & Wiring
Connect the FM24W256 to your Arduino using the standard I2C pins (SDA/SCL). 

*   **Voltage:** Ensure your MCU and FRAM are voltage compatible (the FM24W256 typically supports 2.7V to 5.5V).
*   **Address Pins:** Ensure A0, A1, and A2 are tied to GND or VCC to set the I2C address (default for this library is usually `0x50`).
*   **WP (Write Protect):** Must be tied to GND to allow writing.

## 📦 Installation
Because this is an incomplete library, I suggest **not** installing it to your global Arduino libraries folder. Instead:
1. Download the source files.
2. Place them directly into your project folder.
3. Include them using `#include "FM24W256.h"` (using quotes instead of angle brackets).

## 📖 Usage
Please refer to the provided `.ino` project included in this repository for a practical implementation of the streaming functions.
