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

## Vocabulary

This project interfaces with the DFPlayer Mini mp3 module that accepts an SD card containing .mp3 or .wav audio files _ONLY_.

This repository comes with some default vocabulary in the folder `audio_files_source`. However, you must populate your SD card with the vocabulary by running `speakaudio_files_generation.py` because of a _TON OF LIMITATIONS_ of the DFPlayer Mini module. Simply copy-pasting mp3 files into the SD card will not work. If you wish to create your own vocabulary or replace the default audio files, feel free to do so in the audio file source folder

1. Your source audio files in the `audio_files_source` folder can only be of formats in ['mp3', 'wav', 'm4a', 'flac', 'ogg', 'wma']
2. Your source audio filename must be underscore-separated containing the spoken words. For example if your audio file says "hello world", the filename should be "HELLO_WORLD.<audio file extension>"
3. Run `speakaudio_files_generation.py` and follow the command prompt instructions to generate your vocabulary files to be stored in your SD card. If you do not use a SD card, the output will be to the folder `audio_files_target` for your debugging and verification
4. You can keep adding new audio files into `audio_files_source` folder and if you run `speakaudio_files_generation.py` and follow its instructions, there will be no problems with expanding the vocabulary present in the SD card
5. _DO NOT MANUALLY INSERT OR DELETE_ any files from your SD card at all. If you delete or insert anything, then the entire vocabulary will be rendered useless. Delete all the files from the SD card and rerun `speakaudio_files_generation.py` to refresh the entire vocabulary and your vocabulary variables in your Arduino code must be replaced because the numbering will be wrong

## DFPlayer Mini

DFPlayer Mini comes with its limitations. We have experimented and found the following

- The DFPlayer Mini for Arduino requires audio files to be uploaded into a SD card. Further, it requires the following conditions to be met for these audio files in the SD card:

  - The audio file filename must begin with a 4-digit number in the range 0001-9999.
  - The audio file's 4-digit number in the name must correspond to the sequence by which the file was written into the SD card.
    - e.g. 0001 is the very first file uploaded, and given 0001, 0002, 0003, the file with 0002 in the name must not be uploaded later than 0003.
  - There must be at least a 1-second difference between the time the file is created in the SD card
    - In hh:mm:ss:ms format, if file 0001 was created at 11:22:33:400 and file 0002 was created at 11:22:33:900 then the SD card will still fail.
  - The filenames can contain other characters, as long as it satisfies condition 1.
    - e.g. 0001_HELLO will be ok.
  - The file format must be .mp3 or .wav only

- Running `speakaudio_files_generation.py` will copy audio files present in the source folder {source_folder}, apply a 4-digit number to it and save it in the root folder such that it meets all the criterias above

- If you ever need to remove an audio file, do note that it WILL BREAK ALL of your Arduino code since the time order will be messed up. Therefore do your best and plan accordingly. Do not remove, only add.

- After running `speakaudio_files_generation.py`, you can call on the audio files using the filenames. Refer to our code for a demonstration
