# TF2 Example Plugin

This is an example client/server plugin for Team Fortress 2 on Windows or Linux.

## Building - Windows (Visual Studio 2019 or later)
1. Open the project repository in Visual Studio
2. Press build

or if you don't plan on editing code with Visual Studio:
1. Run `build.cmd`

## Building - Windows (Older Visual Studio)
1. Generate your project files with CMake
2. Open the generated solution file in Visual Studio
3. Press build

## Building - Linux
**Requirements:** C++ build tools and `dpkg`
### Installing the Steam Client Runtime ([Wiki](https://developer.valvesoftware.com/wiki/Source_SDK_2013#Source_SDK_2013_on_Linux))
1. Create `/valve`: `sudo mkdir /valve && sudo chown -R $USER /valve && cd /valve`
2. Download the runtime: `wget https://media.steampowered.com/client/runtime/steam-runtime-sdk_latest.tar.xz`
3. Extract the runtime: `tar xvf steam-runtime-sdk_latest.tar.xz`
4. Symlink the runtime: `ln -sf ./steam-runtime-sdk_2013-09-05/ ./steam-runtime`
5. Set up the runtime: `./steam-runtime/setup.sh` (select defaults)

### Building
1. Activate the Steam Client Runtime: `/valve/steam-runtime/shell-i386.sh`
2. Run `cmake -DCMAKE_BUILD_TYPE=Release -G Ninja -B build`
3. Run `ninja -C build`


## License
[MIT](/LICENSE)
