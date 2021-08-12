import argparse
import os
from tqdm import tqdm
import soundfile as sf
import numpy as np
import time


# set up text-to-speech
import pyttsx3
engine = pyttsx3.init()
voices = engine.getProperty('voices') # getting details of current voice
engine.setProperty('voice', voices[1].id) # 1 is female voice
volume = engine.getProperty('volume') # getting to know current volume level (min=0 and max=1)
engine.setProperty('volume',1.0) # setting up volume level between 0 and 1



# function to remove silence, given the signal obtained by opening an audio file using soundfile library
def cut_silence(signal):
    # the signal represents the amplitude of the audio file, hence if value is zero, then we have silence
    # we will slice the signal from the start to the first instance where the amplitude is not zero
    # the signal variable is a 2D numpy array of (frames, channels), but in this case pyttsx3 library generates only one channel

    cutoff_index = 0
    for i, frame in enumerate(signal):
        if frame == 0:
            cutoff_index = i
        else:
            break

    cut_signal = signal[cutoff_index:]
    
    return cut_signal


# main function 
def tts_audio_generation(args):
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
            if target_filename.split('.')[1] == 'wav'
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
    vocabulary = [vocabulary_item for vocabulary_item in vocabulary if vocabulary_item != '']
    vocabulary = [vocabulary_item for vocabulary_item in vocabulary if vocabulary_item[0] != '#']

    # create a uppercase version, with spaces replaced by underscores 
    source_vocab = [vocabulary_item.upper().replace(" ", "_") for vocabulary_item in vocabulary]

    # grab target audio files that might have been generated through this program in the past
    target_filenames = next(os.walk(target_location))[2]
    target_filenames = [target_filename for target_filename in target_filenames
        if target_filename.split('.')[1] == 'wav'
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
            # set the 4-digit number and name for the file to be created (wav format)
            next_target_file_number = target_numbers.pop(0)
            target_filename = f"{target_location}/{next_target_file_number:04d}_{source_vocab_item}.wav"
            
            # call on the text-to-speech engine to create the audio file for the vocabulary_item through the vocabulary
            engine.save_to_file(vocabulary[index], target_filename)
            engine.runAndWait()

            # call on soundfile library to open the audio file that was just created
            # signal represents the audio signal amplitude, hence if amplitude zero, that means silence
            signal, samplerate = sf.read(target_filename)

            # cut the silence in the audio clip from the beginning, flipping it to cut from the end, then restore original direction
            signal = cut_silence(signal)
            signal = np.flip(signal)
            signal = cut_silence(signal)
            signal = np.flip(signal)

            # write out the audio file again, now the beginning and ending silence has been removed
            sf.write(target_filename, signal, samplerate)


            # update the .h and .cpp file so that the vocabulary is discoverable in the library
            with open(f"{library_location}/TTS_AUDIO/TTS_AUDIO.h", "a") as file:
                file.write(f"extern const int wav_{source_vocab_item};\n")
                file.close()
            with open(f"{library_location}/TTS_AUDIO/TTS_AUDIO.cpp", "a") as file:
                file.write(f"extern const int wav_{source_vocab_item} = {next_target_file_number};\n")
                file.close()
            
            # require at least 1 second difference between file creation time, use 2.1 seconds to be safe
            if sd_card:
                time.sleep(2.1)

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

example - adding new vocabulary, local testing
    python tts_audio_generation.py -l C:\\Users\\user\\Documents\\Arduino\\libraries
example - rebuilding vocabulary, local testing
    python tts_audio_generation.py -l C:\\Users\\user\\Documents\\Arduino\\libraries -r

example - adding new vocabulary, using SD card (recommended)
    python tts_audio_generation.py -l C:\\Users\\user\\Documents\\Arduino\\libraries -t D:\\ -s
example - rebuilding vocabulary, using SD card 
    python tts_audio_generation.py -l C:\\Users\\user\\Documents\\Arduino\\libraries -t D:\\ -s -r

example - maximally fully qualified command
    python tts_audio_generation.py -l C:\\Users\\user\\Documents\\Arduino\\libraries -v vocabulary.txt -t audio_files -s -r    
""")

parser = argparse.ArgumentParser(description='')
parser.add_argument('-l', '--library_location', required=True, help='full folder path where all your arduino libraries are stored')
parser.add_argument('-v', '--vocabulary_file', default='vocabulary.txt', help='text file that sets the vocabulary for the audio files. this text file should be in the same folder as this script')
parser.add_argument('-t', '--target_location', default='audio_files', help='full folder path where you want the target wav files to be saved')
parser.add_argument('-s', '--sd_card', default=False, action='store_true', help='flag to indicate if SD card is being used. defaults to true if present. omit if you are not using sd card')
parser.add_argument('-r', '--rebuild', default=False, action='store_true', help='flag to delete and rebuild the library and audio files. defaults to true if present. omit if you do not want to rebuild')
args = vars(parser.parse_args())


# call main function
tts_audio_generation(args)


