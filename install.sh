echo "Get dependencies libs and build tools"

sudo apt-get update
sudo apt install cmake build-essential libsndfile1 libsndfile1-dev libboost-all-dev htop net-tools debhelper devscripts lockfile-progs -y

echo "Install usbmount"

git clone https://github.com/rbrito/usbmount.git
cd usbmount
sudo dpkg-buildpackage -us -uc -b
sudo dpkg -i ../usbmount_*.deb
cd ..

echo "Build the project"

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4

echo "Copy build result"
cp RavennaRecorder /usr/local/bin
cd ..

echo "Copy config page"
cp HDMIMain.html $HOME/HDMIMain.html

echo "Create service"

sudo rm /etc/systemd/system/RavennaRecorder.service
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

echo "Start service"
sudo systemctl daemon-reload
sudo systemctl enable RavennaRecorder
sudo systemctl start RavennaRecorder

echo "Installation complete, service status:"
sudo systemctl status RavennaRecorder

echo "You can reboot the system to check if the service starts automatically."