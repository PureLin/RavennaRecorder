
device_ips=$(ip addr show | grep 'inet ' | awk '{print $2}')

if echo  -e "$device_ips" | grep -q "169.254"; then
  echo  -e "\n----Get git----\n"
else
  echo -e "Please config your network to link-local address (169.254.x.x) before run this install script\n"
  echo -e "Please config your network to link-local address (169.254.x.x) before run this install script\n"
  echo -e "Please config your network to link-local address (169.254.x.x) before run this install script\n"
  exit 1
fi

sudo apt-get update
sudo apt install git

cd ~

echo -e "\n----clean up old repo----\n"
rm -rf *.html
rm -rf RavennaRecorder

echo -e "\n----clone the project----\n"

if [ -d "RavennaRecorder" ]; then
    echo -e "\n----Old version of project found, remove it----\n"
    rm -rf RavennaRecorder
fi
git clone https://github.com/PureLin/RavennaRecorder.git RavennaRecorder
cd RavennaRecorder

echo -e  "\n----run install script----\n"
sudo chmod +x dependency.sh
sudo -E ./dependency.sh

sudo chmod +x install.sh
sudo -E ./install.sh soundpuzzle

echo -e "\n----save version number----\n"
curl https://api.github.com/repos/PureLin/RavennaRecorder/git/refs/heads/master > version.info

echo -e "\n----remove repo----\n"
cd ~
rm -rf RavennaRecorder