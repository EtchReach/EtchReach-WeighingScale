# EtchReach-WeighingScale-Firmware

Firmware for talking weighing scale

## How to use

1. Download VSCode (https://code.visualstudio.com/download)
2. Download PlatformIO extension on VSCode (https://platformio.org/install/ide?install=vscode)
3. Clone or download this repo
4. Open `Weighing_Scale` folder in VSCode
5. Plug in microcontroller into your computer
6. Open device manager on your computer to identify which port your microcontroller has been assigned to (e.g. COM3)
7. In VSCode, open `platformio.ini` and change your `board` and `upload_port` to the correct one
8. In VSCode, near the bottom, select &#10004; for building or :arrow_right: for uploading

## Schematic

![Schematic](https://github.com/klim077/EtchReach-WeighingScale-Firmware/blob/main/assets/WeighingScaleSchematic1.png)

## Vocabulary and Audio Files

Please refer to the folder `TTS_AUDIO`, and the `README.md` file there to understand the audio generation and library creation
