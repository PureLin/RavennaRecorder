echo "Get git\n"

sudo apt install git

echo "clone the project\n"

if [ -d "RavennaRecorder" ]; then
    echo "Old version of project found, remove it\n"
    rm -rf RavennaRecorder
fi
git clone https://github.com/PureLin/RavennaRecorder.git RavennaRecorder
cd RavennaRecorder

echo "make install script executable\n"
sudo chmod +x install.sh

echo "run install script\n"
sudo -E ./install.sh soundpuzzle