# RavennaRecorder

Ravenna Recorder is an open-source software developed in collaboration with PureLIN and Sound Puzzle.

This software enables reliable and convenient recording of AES67/Ravenna audio streams on a Linux-based platform, 

making it a perfect solution for AoIP multi-track recording applications such as broadcasting, live events, and field recording.

Sound Puzzle providing funding and project oversight, while PureLIN led the development.

Sound Puzzle, based in Canada, specializes in high-quality audio production and solutions for broadcasting, games, music, and system integration.
![SoundPuzzle Logo](./resource/soundpuzzle/SP.png)

## Compatibility with other AoIP protocol

Since Ravenna is compatible with AES67, this project works with AES67 stream.

The recorder only works with multicast stream now, to use with Dante, you should change the stream to multicast.

## Hardware requirement

This project should run on a hardware exclusively, so when device is power on, it can start recording automatically.

The hardware should be a linux device, have one gigabit ethernet port, and a usb port to connect storage device.

Recommend two options:

1. A small x86_64 NUC with N100 processor, install Ubuntu 23.04 (Tested).
2. Any arm SBC with a gigabit ethernet port like Raspberry Pi 4b (Tested), Orange Pi, etc.

## Installation

### Prepare the hardware

1. Install system on the hardware, and connect the hardware to the internet. 

    PS: If you don't have two ethernet port, The device should connect to internet by Wi-Fi since the ethernet port should be connected to the Ravenna network.

2. Check the network status by run command "ifconfig" in terminal, you should get an IP with "169.254.x.x" for the port connected to Ravenna network.

   **Important:On newer system, you should make this ethernet port with "Link-Local" IPv4 mode, you can set it by using nmtui or in GUI.**

### Running the installation script
After you have all above setup, you can install RavennaRecorder by the following steps in terminal:

    1. Install git by `sudo apt install git`
    2. Get the code from git repo: `git clone https://github.com/PureLin/RavennaRecorder.git`
    3. Switch to the directory: `cd RavennaRecorder` and make the script executable: `sudo chmod +x install.sh`
    4. Run the installation script: `sudo -E ./install.sh` (if you want to use a SoundPuzzle logo version UI, add "soundpuzzle" to the params: `sudo -E ./install.sh "soundpuzzle"` )
    5. After install finished, you can check the status by `sudo systemctl status RavennaRecorder`

### Check the installation
If everything works fine, you should open the browser and type `http://<device_ip>` to access the web interface.

If you don't see the web interface, you can check the status by `sudo systemctl status RavennaRecorder`, and see the log file `Recorder.log` in your home dir.

The most common issue is not set the ethernet port to "Link-Local" mode, you can check it by `ifconfig` in terminal.

## Record file structure

This program allow you to select a storage device to save the record file, and will create a folder "RavennaRecords" in
the root of the storage device.

Each new stream will create a new folder with the stream name in the "RavennaRecords" folder, and the record file will
in that folder with name format "YYYY-MM-DD_HH-MM-SS.wav".

    /path/to/your/storage/device
    ├── RavennaRecords
    │   ├── StreamName1
    │   │   ├── 2021-09-01_00-00-00.wav
    │   │   ├── 2021-09-01_01-00-00.wav
    │   │   └── ...
    │   ├── StreamName2
    │   │   ├── 2021-09-02_00-00-00.wav
    │   │   ├── 2021-09-02_01-00-00.wav
    │   │   └── ...
    │   └── ...
    └── ...

## Settings

This program contain some settings, you can change it on the browser.

Or, the config file is store in `RecorderConfig.json` file in home directory.

Example of the config file, and the explanation of each field:

    {
        //password to put when using web interface, default is 0000
        "configPassword": "0000",
        //path to save the record file on last time, if this path related external storage not avaiable, will use home path
        "defaultRecordPath": "/home/me/RavennaRecords",
        //split the record file every [1-60] minutes, when time reach, will start a new file
        "splitTimeInMinutes": 60,
        //when device see a new stream, start recording immediately or wait for user to push start button
        "startRecordImmediately": false,
    }

## Control Webpage

The control for this application is a static webpage "ConfigMain.html" in the root of the project, using websocket to
communicate with the backend.
It will copy to the home directory when install application, and when you connect to
the  `http://<device_ip>`, the html file will be read again.
So you can change the html file in your home dir by your need, just refresh the control page will load the new html
file, no restart is required.
