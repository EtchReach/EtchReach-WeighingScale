# vocabulary.txt

You can create vocabulary items by typing the words/phrases you want in vocabulary.txt following the rules below

1. Type exactly what you want to be said
2. No punctuations are to be used, the vocabulary item will become the filename
3. Each line is one vocabulary item
4. Do not repeat vocabulary items (no duplicates, things will break)
5. You can begin a line with the # sign to mark the line as a comment. Comment lines will not count as vocabulary items
6. You can leave blank lines, they will not count as vocabulary items too
7. If you need numbers, try to type them out (this is untested)

# tts_audio_generation.py

You must run this python script with a SD card if you want to use it with the DFPlayer Mini. You can run this script locally as well for testing purposes.

This script will perform the following key operations

1. Generate audio files (.wav) from the vocabulary file provided using a text-to-speech engine (pyttsx3)
2. Remove leading and trailing silence from the generated audio using soundfile library, for snappier audio
3. Save the audio into a target location, likely a SD card
4. Take the path to your Arduino libraries and generate a TTS_AUDIO library in there with the .cpp and .h files that contains vocabulary-to-integer mappings

You will then be able to use the vocabulary items as per this example:

- vocabulary is _"hello world"_
- usage in the Arduino code will be _"playTrack(wav_HELLO_WORLD);"_

We have the following details in argparse, but copied here for your convenience

````
- get argument help:
  - python tts_audio_generation.py -h
- example - adding new vocabulary, local testing
  - python tts_audio_generation.py -l C:\\Users\\user\\Documents\\Arduino\\libraries
- example - rebuilding vocabulary, local testing
  - python tts_audio_generation.py -l C:\\Users\\user\\Documents\\Arduino\\libraries -r
- _example - adding new vocabulary, using SD card (recommended)_
  - python tts_audio_generation.py -l C:\\Users\\user\\Documents\\Arduino\\libraries -t D:\\ -s
- example - rebuilding vocabulary, using SD card
  - python tts_audio_generation.py -l C:\\Users\\user\\Documents\\Arduino\\libraries -t D:\\ -s -r
- example - maximally fully qualified command
  - python tts_audio_generation.py -l C:\\Users\\user\\Documents\\Arduino\\libraries -v vocabulary.txt -t audio_files -s -r
    ```
````

# DFPlayer Mini is WEIRD

The DFPlayer Mini for Arduino requires audio files to be uploaded into a SD card.
Further, it requires the following conditions to be met for these audio files:

1. The audio file filename must begin with a 4-digit number in the range 0001-9999.
2. The audio file's 4-digit number in the name must correspond to the sequence by which the file was written into the SD card.
   e.g. 0001 is the very first file uploaded, and given 0001, 0002, 0003, the file with 0002 in the name must not be uploaded later than 0003.
3. There must be at least a 1-second difference between the time the file is created in the SD card
   e.g. In hh:mm:ss:ms format, if file 0001 was created at 11:22:33:400 and file 0002 was created at 11:22:33:900 then the SD card will still fail.
4. The filenames can contain other characters, as long as it satisfies condition 1.
   e.g. 0001_HELLO will be ok.
5. The file format must be .mp3 or .wav only (we will use .wav)
6. The SD card must be formatted as FAT32

Running this code will generate audio files for the vocabulary items in vocabulary.txt, apply a 4-digit number to it
and save it in the target folder such that it meets all the criterias above

If you ever need to remove an audio file, do note that it WILL BREAK ALL of your Arduino code since the time order will be messed up.
Therefore do your best and plan accordingly. Do not remove manually, only add or rerun this code with an rebuild flag of -r

If you performed everything correctly but the audio playback from the mp3 messes up (e.g. playing wrong files), go format the SD card or check for hidden files/extra files beyond the .wav files
