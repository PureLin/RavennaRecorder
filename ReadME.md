# RavennaRecorder

This project is using for recording audio stream from Ravenna network and save it to a file.

Since Ravenna is compatible with AES67, this project should works with AES67 stream,but not tested yet.

The recorder only works with multicast stream now.

## Hardware requirement

This project should run on a hardware exclusively, so when device is power on, it can start recording automatically.

The hardware should be a linux device, have one gigabit ethernet port, and a usb port to connect storage device.

Recommend two options:

1. A small x86_64 NUC with N100 processor, install Ubuntu 23.04 (Tested).
2. Any arm SBC with a gigabit ethernet port like Raspberry Pi 4b (Tested), Orange Pi, etc.

## Installation

First, you need install a ubuntu/Raspbian on the hardware, and connect the hardware to the internet.

Notice the ethernet port should be connected to the Ravenna network, so if you don't have two ethernet port,
The device should connect to internet by Wi-Fi.

Then you need to check the network status, it should get an IP with "169.254.x.x" from Ravenna network.
If not, you should make this ethernet port with "Link-Local" IPv4 mode, you can set it by using nmtui or in GUI.

After you have all above setup, you can install RavennaRecorder by the following steps:

1. If you don't have git installed, install it by `sudo apt install git`
2. Get the code from git repo: `git clone https://github.com/PureLin/RavennaRecorder.git`
3. Switch to the directory: `cd RavennaRecorder` and make the script executable: `sudo chmod +x install.sh`
4. Run the installation script: `sudo -E ./install.sh`
5. After install finished, you can check the status by `sudo systemctl status RavennaRecorder`
6. If everything works fine, you should open the browser and type `http://<device_ip>:3031` to access the web interface.

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
        //path to save the record file on last time, if no external storage avaiable, will use home path
        "defaultRecordPath": "/home/me/RavennaRecords",
        //split the record file every [1-60] minutes, when time reach, will start a new file with the start time
        "splitTimeInMinutes": 60,
        "startRecordImmediately": "when device see a new stream, start recording immediately or wait for user to start",
    }

## Control Webpage

The control for this application is a static webpage "HDMIMain.html" in the root of the project, using websocket to
communicate with the backend.
It will copy to the home directory of the user when install the application, when you connect to
the  `http://<device_ip>:3031`, the html file will be read again.
So you can change the html file in your home dir by your need, just refresh the control page will load the new html
file.
