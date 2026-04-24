# codec-h264-ESP32S3

[![Arduino Library](https://img.shields.io/badge/Arduino-Library-blue.svg)](https://www.arduino.cc/reference/en/libraries/)
[![License: Apache](https://img.shields.io/badge/License-Apache-yellow.svg)](https://opensource.org/licenses/Apache-2.0)

This project provides Espressif's lightweight H.264 encoder and decoder component as Arduino library, offering a software implementations. The software encoder (SW encoder) is sourced from v2.2.0 of [openh264](https://github.com/cisco/openh264), while the decoder is obtained from [tinyH264](https://github.com/udevbe/tinyh264). Both the software encoder and decoder are optimized for memory and CPU usage, ensuring optimal performance on Espressif chips.

## Project Scope

The project provides the following functionality
- Encoding of YUV422, RGB565 and I420 frames
- Sending Data over UDP
- RDP Packetization 
- Decoding of h.264 data

## Logging

This library is using the standard ESP32 logger that you can configure via the Arduino Tools menu.

## Documentation

- [Class Documentation](https://pschatzmann.github.io/codec-h264-ESP32S3/namespaceesp__h264.html)
- [Wiki](https://github.com/pschatzmann/codec-h264-ESP32S3/wiki)
- [Examples](examples)


## Installation

For Arduino, you can download the library as zip and call include Library -> zip library. Or you can git clone this project into the Arduino libraries folder e.g. with

```
cd  ~/Documents/Arduino/libraries
git clone https://github.com/pschatzmann/codec-h264-ESP32S3.git
```

## Additional Comments

You can use my [TinyGPU](https://github.com/pschatzmann/TinyGPU) library to draw arbitrary lines or shapes, use sprites and render simple 3D wireframes as RGB565 that can be fed to the encoder.

