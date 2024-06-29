echo "Get dependencies libs and build tools\n"

sudo apt-get update
sudo apt install cmake build-essential libsndfile1 libsndfile1-dev libboost-all-dev htop net-tools debhelper devscripts lockfile-progs -y

echo "\nInstall usbmount\n"
set -e
git clone https://github.com/rbrito/usbmount.git
cd usbmount
sudo dpkg-buildpackage -us -uc -b
sudo dpkg -i ../usbmount_*.deb
cd ..

echo "\nBuild the project\n"

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

if [ -f "/etc/systemd/system/RavennaRecorder.service" ]; then
    sudo rm /etc/systemd/system/RavennaRecorder.service
fi
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