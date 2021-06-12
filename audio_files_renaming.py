import os
from tqdm import tqdm
import time
from pydub import AudioSegment

# GLOBAL settings, change for own testing
# TARGET_FOLDER does not apply when writing to SD cards, user input will be requested for the SD card drive assignment
SOURCE_FOLDER = "audio_files_source"
TARGET_FOLDER = "audio_files_target" 


def audio_files_renaming(source_folder, target_folder):
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
    
    Running this code will copy audio files present in the source folder {source_folder}, apply a 4-digit number to it 
    and save it in the root folder such that it meets all the criterias above 

    If you ever need to remove an audio file, do note that it WILL BREAK ALL of your Arduino code since the time order will be messed up.
    Therefore do your best and plan accordingly. Do not remove, only add.

    Have you read the notice? 
    Type 'yes' or 'y' if you have. 
    Your input here:::::::::::::::::::::: """ )
    

    # Check if user read the 
    if proceed not in ["yes", "y"]:
        print("You decided not to proceed. Exiting...")
        return

    using_sd_card = input("""
    Are you using a SD card now? 
    Type 'yes' or 'y' to if you are, and "no' or 'n' otherwise. 
    Your input here:::::::::::::::::::::: """ )

    if using_sd_card in ["yes", "y"]:
        target_folder = input("""
        Ensure your SD card is inserted into your computer.
        What is the drive that your SD card has been assigned to? 
        Please key in just the letter, e.g. d/D for D drive 
        Do not get this wrong!
        Your input here:::::::::::::::::::::: """ )
        target_folder = target_folder.upper() + ":\\"

        print(f"Your SD card is {target_folder} drive")
    elif using_sd_card in ["no", "n"]:
        try:
            os.mkdir(TARGET_FOLDER)
            print(f"Making {TARGET_FOLDER}")
        except:
            print(f"{TARGET_FOLDER} exists")
    else:
        print("Invalid input. Exiting...")
        return

    # just for tracking
    copied = 0
    unchanged = 0

    # grab source audio files that correspond to several popular audio formats
    source_filenames = next(os.walk(source_folder))[2]
    source_filenames = [source_filename for source_filename in source_filenames
        if source_filename.split('.')[1] in ['mp3', 'wav', 'm4a', 'flac', 'ogg', 'wma']
    ]
    # audio files must be named using what was spoken in the audio
    # e.g. audio file saying "hello world" has a filename "hello_world" with underscores
    source_vocab = [source_filename.split('.')[0].upper() for source_filename in source_filenames]


    # grab target audio files that might have been generated through this program in the past
    target_filenames = next(os.walk(target_folder))[2]
    target_filenames = [target_filename for target_filename in target_filenames
        if target_filename.split('.')[1] in ['mp3', 'wav']
    ]
    target_vocab = [target_filename.split('.')[0].split('_', 1)[1] for target_filename in target_filenames]


    # numbering must be strictly increasing with no gaps in between, and within the range of 0001-9999 inclusive
    # find the next number to start at
    target_numbers_used = [int(target_filename.split('_', 1)[0]) for target_filename in target_filenames]
    target_numbers = list(set(list(range(1,10000))) - set(target_numbers_used))
    
    # loop over every word in the source folder 
    for index, source_word in enumerate(tqdm(source_vocab)):
        # skip over words that are already in the target folder 
        if (source_word not in target_vocab):
            # find the source file
            source_filename = f"{source_folder}/{source_filenames[index]}"

            # set the 4-digit number and name for the file to be created (mp3 format)
            next_target_file_number = target_numbers.pop(0)
            target_filename = f"{target_folder}/{next_target_file_number:04d}_{source_filenames[index].split('.')[0]}.mp3"
            
            # open the audio file, since the file format might not be mp3, we need to reexport it
            audio = AudioSegment.from_file(source_filename)
            audio.export(target_filename, format="mp3")
            
            # require at least 1 second difference between file creation time, use 1.1 seconds to be safe
            if using_sd_card in ["yes", "y"]:
                time.sleep(1.1)

            # counter 
            copied += 1

        else:
            # counter
            unchanged += 1
    
    # completion
    print(f"Finished copying and renaming audio files from {source_folder} and saved in {target_folder}")
    print(f"{copied} files copied, {unchanged} files unchanged")


# calling main function
audio_files_renaming(SOURCE_FOLDER, TARGET_FOLDER)