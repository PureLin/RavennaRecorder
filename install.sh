echo "Get dependencies libs and build tools\n"

sudo apt-get update
sudo apt install cmake build-essential libsndfile1 libsndfile1-dev libboost-all-dev htop net-tools debhelper devscripts lockfile-progs exfat-fuse exfatprogs hfsplus hfsprogs -y

if [ -f "/etc/systemd/system/RavennaRecorder.service" ]; then
  echo "Old version of service found, remove it\n"
  sudo systemctl stop RavennaRecorder.service
  sudo systemctl disable RavennaRecorder.service
  sudo rm /etc/systemd/system/RavennaRecorder.service
  sudo systemctl daemon-reload
  sudo rm /usr/local/bin/RavennaRecorder
fi

if ! dpkg -l | grep -qw usbmount; then
    echo "\nInstall usbmount\n"
    set -e
    git clone https://github.com/rbrito/usbmount.git
    cd usbmount
    sudo dpkg-buildpackage -us -uc -b
    sudo dpkg -i ../usbmount_*.deb
    cd ..
else
    echo "\nusbmount already installed\n"
fi

echo "\nCreate usbmount config\n"
if [ -f "/etc/usbmount/usbmount.conf" ]; then
    sudo rm /etc/usbmount/usbmount.conf
fi
echo "ENABLED=1" | sudo tee /etc/usbmount/usbmount.conf
echo "" | sudo tee -a /etc/usbmount/usbmount.conf
echo "MOUNTPOINTS=\"/media/usb0 /media/usb1 /media/usb2 /media/usb3 /media/usb4 /media/usb5 /media/usb6 /media/usb7\"" | sudo tee -a /etc/usbmount/usbmount.conf
echo "" | sudo tee -a /etc/usbmount/usbmount.conf
echo "FILESYSTEMS=\"vfat ext2 ext3 ext4 hfsplus exfat ntfs\"" | sudo tee -a /etc/usbmount/usbmount.conf
echo "" | sudo tee -a /etc/usbmount/usbmount.conf
echo "MOUNTOPTIONS=\"sync,noexec,nodev,noatime,nodiratime\"" | sudo tee -a /etc/usbmount/usbmount.conf
echo "FS_MOUNTOPTIONS=\"\"" | sudo tee -a /etc/usbmount/usbmount.conf
echo "" | sudo tee -a /etc/usbmount/usbmount.conf
echo "VERBOSE=no" | sudo tee -a /etc/usbmount/usbmount.conf

echo "\nBuild the project\n"

rm -rf build
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4

echo "\nCopy build result\n"
cp RavennaRecorder /usr/local/bin
cd ..

echo "\nCopy config page\n"
cd resource
if [ "$1" = "soundpuzzle" ]; then
    echo "Use SoundPuzzle config page."
    cd soundpuzzle
fi
cp ConfigMain.html $HOME/ConfigMain.html
cd ..
if [ "$1" = "soundpuzzle" ]; then
    cd ..
fi

echo "\nCreate service\n"

echo "[Unit]" | sudo tee /etc/systemd/system/RavennaRecorder.service
echo "Description=RavennaRecorder" | sudo tee -a /etc/systemd/system/RavennaRecorder.service
echo "After=network.target" | sudo tee -a /etc/systemd/system/RavennaRecorder.service
echo "" | sudo tee -a /etc/systemd/system/RavennaRecorder.service
echo "[Service]" | sudo tee -a /etc/systemd/system/RavennaRecorder.service
echo "Environment=\"HOME=$HOME\"" | sudo tee -a /etc/systemd/system/RavennaRecorder.service
echo "ExecStartPre=/bin/sleep 3" | sudo tee -a /etc/systemd/system/RavennaRecorder.service
echo "ExecStart=/usr/local/bin/RavennaRecorder" | sudo tee -a /etc/systemd/system/RavennaRecorder.service
echo "Restart=always" | sudo tee -a /etc/systemd/system/RavennaRecorder.service
echo "User=root" | sudo tee -a /etc/systemd/system/RavennaRecorder.service
echo "Group=root" | sudo tee -a /etc/systemd/system/RavennaRecorder.service
echo "" | sudo tee -a /etc/systemd/system/RavennaRecorder.service
echo "[Install]" | sudo tee -a /etc/systemd/system/RavennaRecorder.service
echo "WantedBy=multi-user.target" | sudo tee -a /etc/systemd/system/RavennaRecorder.service

echo "\nStart service\n"
sudo systemctl daemon-reload
sudo systemctl enable RavennaRecorder
sudo systemctl start RavennaRecorder

echo "\nInstallation complete, service status:\n"
sudo systemctl status RavennaRecorder

echo "\nYou can reboot the system to check if the service starts automatically."