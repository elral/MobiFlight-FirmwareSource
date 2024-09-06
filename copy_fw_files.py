Import("env")
import os, zipfile, shutil
from pathlib import Path

# Get the version number from the build environment.
firmware_version = os.environ.get('VERSION', "")
if firmware_version == "":
    firmware_version = "0.0.1"
firmware_version = firmware_version.lstrip("v")
firmware_version = firmware_version.strip(".")

zip_file_name = 'Mobiflight-Connector'
build_path = './_build'
build_path_fw = build_path + '/firmware'
build_path_json = build_path + '/Boards'
distrubution_path = './_dist'
board_folder = ['./_Boards/Atmel', './_Boards/RaspberryPi', './_Boards/ESP32']
platform = env.BoardConfig().get("platform", {})

def copy_fw_files (source, target, env):
    fw_file_name=str(target[0])

    if os.path.exists(build_path_fw) == False:
        os.makedirs(build_path_fw)
    if os.path.exists(build_path_json) == False:
        os.makedirs(build_path_json)

    if fw_file_name[-3:] == "bin" and platform == "raspberrypi":
        fw_file_name=fw_file_name[0:-3] + "uf2"

    if platform == "espressif32":
        merge_bin(source, target, env)
        old_name = fw_file_name[0:-4] + "_merged.bin"
        fw_file_name = fw_file_name[0:-9]  + "merged_" + firmware_version.replace(".", "_") + ".bin"
        os.replace(old_name, fw_file_name)

    # Copy build FW file
    shutil.copy(fw_file_name, build_path_fw)

    # Copy reset/uf2/json files
    file_extension = '.hex'
    copy_files_by_extension(board_folder, build_path_fw, file_extension)
    file_extension = '.uf2'
    copy_files_by_extension(board_folder, build_path_fw, file_extension)
    file_extension = '.json'
    copy_files_by_extension(board_folder, build_path_json, file_extension)

    # Create ZIP file and add files from distrubution folder
    zip_file_path = distrubution_path + '/' + zip_file_name + '_' + firmware_version + '.zip'
    createZIP(build_path, zip_file_path, zip_file_name)


def copy_files_by_extension(source_folders, target_folder, file_extension):
    for source_folder in source_folders:
        for root, dirs, files in os.walk(source_folder):
            for file in files:
                if file.endswith(file_extension):
                    source_path = os.path.join(root, file)
                    target_path = os.path.join(target_folder, file)
                    shutil.copy2(source_path, target_path)


def createZIP(original_folder_path, zip_file_path, new_folder_name):
    if os.path.exists(distrubution_path) == False:
        os.mkdir(distrubution_path)
    with zipfile.ZipFile(zip_file_path, 'w') as zipf:
        for root, dirs, files in os.walk(original_folder_path):
            for file in files:
                # Create a new path in the ZIP file
                new_path = os.path.join(new_folder_name, os.path.relpath(os.path.join(root, file), original_folder_path))
                # Add the file to the ZIP file
                zipf.write(os.path.join(root, file), new_path)

# Script from: https://github.com/platformio/platform-espressif32/issues/1078
# creates one .bin file for ESP32 which can be flashed at once
# e.g. C:/Users/ralfk/.platformio/penv/Scripts/python.exe C:/Users/ralfk/.platformio/packages/tool-esptoolpy/esptool.py write_flash 0x0000 C:/_PlatformIO/MobiFlight-FirmwareSource/.pio/build/mobiflight_esp32_s2_mini/mobiflight_esp32_s2_mini_merged_0_0_1.bin
APP_BIN = "$BUILD_DIR/${PROGNAME}.bin"
MERGED_BIN = "$BUILD_DIR/${PROGNAME}_merged.bin"
BOARD_CONFIG = env.BoardConfig()

def merge_bin(source, target, env):
    # The list contains all extra images (bootloader, partitions, eboot) and
    # the final application binary
    flash_images = env.Flatten(env.get("FLASH_EXTRA_IMAGES", [])) + ["$ESP32_APP_OFFSET", APP_BIN]

    # Run esptool to merge images into a single binary
    env.Execute(
        " ".join(
            [
                "$PYTHONEXE",
                "$OBJCOPY",
                "--chip",
                BOARD_CONFIG.get("build.mcu", "esp32"),
                "merge_bin",
                "--fill-flash-size",
                BOARD_CONFIG.get("upload.flash_size", "4MB"),
                "-o",
                MERGED_BIN,
            ]
            + flash_images
        )
    )

# Patch the upload command to flash the merged binary at address 0x0
#env.Replace(
#    UPLOADERFLAGS=[
#        ]
#        + ["write_flash", "0x0", MERGED_BIN],
#    UPLOADCMD='"$PYTHONEXE" "$UPLOADER" $UPLOADERFLAGS',
#)

env.AddPostAction("$BUILD_DIR/${PROGNAME}.hex", copy_fw_files)
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_fw_files)
