echo  -e "\n----Get latest version ifo----\n"
rm /tmp/version.info
curl -s https://api.github.com/repos/PureLin/RavennaRecorder/git/refs/heads/master > /tmp/version.info

new_ver="/tmp/version.info"
curr_ver="$HOME/version.info"

# 使用 diff 命令比较
if diff "$curr_ver" "$new_ver" > /dev/null; then
    echo "No new version available"
    exit 0
fi

cd ~

echo -e "New version available, start update"
echo -e "\n----clean up old repo----\n"
rm -rf RavennaRecorder

echo -e "\n----clone the project----\n"

if [ -d "RavennaRecorder" ]; then
    echo -e "\n----Old version of project found, remove it----\n"
    rm -rf RavennaRecorder
fi
git clone https://github.com/PureLin/RavennaRecorder.git RavennaRecorder
cd RavennaRecorder

sudo chmod +x dependency.sh
sudo -E ./dependency.sh

echo -e "\n----Build the project----\n"

rm -rf build
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4

echo -e "\n----Copy build result----\n"
cp RavennaRecorder /usr/local/bin
cd ..

echo -e "\n----Copy config page----\n"
cd resource/soundpuzzle
cp ConfigMain.html $HOME/ConfigMain.html
cd ../..

echo -e "\n----Remove Repo----\n"
cd ~
rm -rf RavennaRecorder

echo -e "\n----Update version info----\n"
curl https://api.github.com/repos/PureLin/RavennaRecorder/git/refs/heads/master > $HOME/version.info

echo -e "\n----Update finished----\n"
exit 1
