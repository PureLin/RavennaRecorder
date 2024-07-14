
device_ips=$(ip addr show | grep 'inet ' | awk '{print $2}')

# 检查是否包含特定的IP地址或地址段
if echo "$device_ips" | grep -q "169.254"; then
  echo "Get git\n"
else
  echo "Please config your network to link-local address (169.254.x.x) before run this install script\n"
  echo "Please config your network to link-local address (169.254.x.x) before run this install script\n"
  echo "Please config your network to link-local address (169.254.x.x) before run this install script\n"
  exit 1
fi

sudo apt-get update
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