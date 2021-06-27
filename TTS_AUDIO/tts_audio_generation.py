import argparse
import os
from tqdm import tqdm
import time

import pyttsx3
engine = pyttsx3.init()
voices = engine.getProperty('voices') # getting details of current voice
engine.setProperty('voice', voices[1].id)  # 1 is female voice
# rate = engine.getProperty('rate')  # getting details of current speaking rate
# engine.setProperty('rate', rate*0.75)     # setting up new voice rate


def audio_files_renaming(args):
    proceed = input(f"""
    ============================== NOTICE - MUST READ ==============================
    The DFPlayer Mini for Arduino requires audio files to be uploaded into a SD card. 
    Further, it requires the following conditions to be met for these audio files:
    \t 1. The audio file filename must begin with a 4-digit number in the range 0001-9999.
    \t 2. The audio file's 4-digit number in the name must correspond to the sequence by which the file was written into the SD card.
    \t\t e.g. 0001 is the very first file uploaded, and given 0001, 0002, 0003, the file with 0002 in the name must not be uploaded later than 0003.
    \t 3. There must be at least a 1-second difference between the time the file is created in the SD card
    \t\t e.g. In hh:mm:ss:ms format, if file 0001 was created at 11:22:33:400 and file 0002 was created at 11:22:33:900 then the SD card will still fail.
    \t 4. The filenames can contain other characters, as long as it satisfies condition 1.
    \t\t e.g. 0001_HELLO will be ok.
    \t 5. The file format must be .mp3 or .wav only
    
    Running this code will generate audio files for the vocabulary items in VOCABULARY.txt, apply a 4-digit number to it 
    and save it in the target folder such that it meets all the criterias above 

    If you ever need to remove an audio file, do note that it WILL BREAK ALL of your Arduino code since the time order will be messed up.
    Therefore do your best and plan accordingly. Do not remove manually, only add or rerun this code with an rebuild flag of -o

    Have you read the notice? 
    Type 'yes' or 'y' if you have. 
    Your input here:::::::::::::::::::::: """ )
    

    # Check if user read the notice
    if proceed not in ["yes", "y"]:
        print("You decided not to proceed. Exiting...")
        return


    # parsing arguments
    library_location = args['library_location']
    vocabulary_file = args['vocabulary_file']
    target_location = args['target_location']
    sd_card = args['sd_card']
    rebuild = args['rebuild']


    # making directory at target location if necessary
    try:
        print(f"Making {target_location}...")
        os.mkdir(target_location)
        print(f"{target_location} created")
    except:
        print(f"{target_location} exists")

    # making directory at library location if necessary
    try:
        print(f"Making TTS_AUDIO arduino library...")
        os.mkdir(f"{library_location}/TTS_AUDIO")
        
        print("Making .h file...")
        with open(f"{library_location}/TTS_AUDIO/TTS_AUDIO.h", "w") as file:
            file.write("// TTS_AUDIO library\n")
            file.write("// Copyright 2021, Tan Chuan Xin\n\n")
            file.write("#include <Arduino.h>\n\n")
            file.close()
        
        print("Making .cpp file...")
        with open(f"{library_location}/TTS_AUDIO/TTS_AUDIO.cpp", "w") as file:
            file.write("// TTS_AUDIO library\n")
            file.write("// Copyright 2021, Tan Chuan Xin\n\n")
            file.write("#include <Arduino.h>\n")
            file.write("#include <TTS_AUDIO.h>\n\n")
            file.close()

        print(f"TTS_AUDIO library created at {library_location}")
    except:
        print(f"TTS_AUDIO library exists at {library_location}")



    # deleting existing files if rebuild is selected
    if rebuild:
        print("Rebuild selected, rebuilding...")

        # purge target folder
        print("Deleting all files in target_location...")
        target_filenames = next(os.walk(target_location))[2]
        target_filenames = [target_filename for target_filename in target_filenames
            if target_filename.split('.')[1] == 'mp3'
        ]        
        for index, target_filename in enumerate(tqdm(target_filenames)):
            os.remove(f"{target_location}/{target_filename}")
    
        # purge header file
        print("Recreating .h file...")
        with open(f"{library_location}/TTS_AUDIO/TTS_AUDIO.h", "w") as file:
            file.write("// TTS_AUDIO library\n")
            file.write("// Copyright 2021, Tan Chuan Xin\n\n")
            file.write("#include <Arduino.h>\n\n")
            file.close()
            
        # purge cpp file
        print("Recreating .cpp file...")
        with open(f"{library_location}/TTS_AUDIO/TTS_AUDIO.cpp", "w") as file:
            file.write("// TTS_AUDIO library\n")
            file.write("// Copyright 2021, Tan Chuan Xin\n\n")
            file.write("#include <Arduino.h>\n")
            file.write("#include <TTS_AUDIO.h>\n\n")
            file.close()

        print("Rebuild complete")       


    # just for tracking
    added = 0
    unchanged = 0

    # read in the full vocabulary from a text file
    vocabulary = open(vocabulary_file, "r").readlines()
    vocabulary = [vocabulary_item.rstrip('\n') for vocabulary_item in vocabulary]

    # create a uppercase version, with spaces replaced by underscores 
    source_vocab = [vocabulary_item.upper().replace(" ", "_") for vocabulary_item in vocabulary]

    # grab target audio files that might have been generated through this program in the past
    target_filenames = next(os.walk(target_location))[2]
    target_filenames = [target_filename for target_filename in target_filenames
        if target_filename.split('.')[1] == 'mp3'
    ]
    target_vocab = [target_filename.split('.')[0].split('_', 1)[1] for target_filename in target_filenames]

    # numbering must be strictly increasing with no gaps in between, and within the range of 0001-9999 inclusive
    # find the next number to start at
    target_numbers_used = [int(target_filename.split('_', 1)[0]) for target_filename in target_filenames]
    target_numbers = list(set(list(range(1,10000))) - set(target_numbers_used))


    # loop over every word in the vocabulary
    print(f"Generating audio files based on {vocabulary_file} and saving in {target_location}")
    for index, source_vocab_item in enumerate(tqdm(source_vocab)):
        
        # words not in target folder must be added
        if (source_vocab_item not in target_vocab):
            # set the 4-digit number and name for the file to be created (mp3 format)
            next_target_file_number = target_numbers.pop(0)
            target_filename = f"{target_location}/{next_target_file_number:04d}_{source_vocab_item}.mp3"
            
            # call on the text-to-speech engine to create the audio file for the vocabulary_item through the vocabulary
            engine.save_to_file(vocabulary[index], target_filename)
            engine.runAndWait()

            # update the .h and .cpp file so that the vocabulary is discoverable in the library
            with open(f"{library_location}/TTS_AUDIO/TTS_AUDIO.h", "a") as file:
                file.write(f"extern const int mp3_{source_vocab_item};\n")
                file.close()
            with open(f"{library_location}/TTS_AUDIO/TTS_AUDIO.cpp", "a") as file:
                file.write(f"extern const int mp3_{source_vocab_item} = {next_target_file_number};\n")
                file.close()
            
            # require at least 1 second difference between file creation time, use 1.1 seconds to be safe
            if sd_card:
                time.sleep(1.1)

            # counter 
            added += 1

        else:
            # counter
            unchanged += 1

    # completion
    print(f"Finished generating audio files from {vocabulary_file} and saved in {target_location}")
    print(f"{added} files added, {unchanged} files unchanged")


# parsing arguments
print("""
get argument help: 
    python tts_audio_generation.py -h
example of a minimally fully qualified command
    python tts_audio_generation.py -l C:\\Users\\user\\Documents\\Arduino\\libraries
example of a maximally fully qualified command
    python tts_audio_generation.py -l C:\\Users\\user\\Documents\\Arduino\\libraries -v vocabulary.txt -t audio_files -s -r
example of using SD card and rebuilding
    python tts_audio_generation.py -l C:\\Users\\user\\Documents\\Arduino\\libraries -t D:\\ -s -r
    
""")

parser = argparse.ArgumentParser(description='')
parser.add_argument('-l', '--library_location', required=True, help='full folder path where all your arduino libraries are stored')
parser.add_argument('-v', '--vocabulary_file', default='vocabulary.txt', help='text file that sets the vocabulary for the audio files. this text file should be in the same folder as this script')
parser.add_argument('-t', '--target_location', default='audio_files', help='full folder path where you want the target mp3 files to be saved')
parser.add_argument('-s', '--sd_card', default=False, action='store_true', help='flag to indicate if SD card is being used. defaults to true if present. omit if you are not using sd card')
parser.add_argument('-r', '--rebuild', default=False, action='store_true', help='flag to delete and rebuild the library and audio files. defaults to true if present. omit if you do not want to rebuild')
args = vars(parser.parse_args())

# calling main function
audio_files_renaming(args)

