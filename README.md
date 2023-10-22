# Raspberry Pi web controlled LEDs
## Introduction
This is a project that allows control of WS281x LEDs from a web page with a particular focus on wireless audio visualization. Currently, it still lacks a lot of functionality as it is a work in progress, but the audio visualization part is currently functional. The other projects that actually transmit audio data in to the RPi web server will be published soon.
# Documentation
I will be adding proper documentation later on. As it is right now it's not exactly easy to build this project. Currently, this project uses GPIO18 for the data output to the LEDs.
## Installation
If you would like to give building a shot, three external dependencies are needed:

- [ixwebsocket](https://machinezone.github.io/IXWebSocket/build/)
- [ws281x RPi userspace library](https://github.com/jgarff/rpi_ws281x)
- [Wt](https://github.com/emweb/wt)

Installing ixwebsocket and ws281x is relatively straightforward. Get their latest releases, unpack them, run `cmake` to configure them and then `make && sudo make install`.

### Compiling Wt
The main trouble is encountered when Wt needs to be compiled. The documentation for Raspberry Pi compilation is outdated on their page.

There are multiple ways to compile Wt, I will describe the two main ones I tried.

#### Compiling on the RPi
This is not recommended for several reasons. One is that compiling it on the RPi will take more than an hour and the second one is that its compilation takes a lot of memory, which may be problematic for the RPi. The general steps are:

- If you are using an RPi with more than 2GB of RAM, simply follow the instructions [here](https://www.webtoolkit.eu/wt/doc/reference/html/InstallationUnix.html).
- If you are using an RPi with less than 2GB of RAM, before following the instructions, make sure to increase swap size to at least 1 GB (instructions [here](https://nebl.io/neblio-university/enabling-increasing-raspberry-pi-swap/)) and make sure that you only use a single make job.

#### Compiling with distcc
This is a much faster method of building Wt. However, the set up is a bit more complicated.

- On your host Linux machine:
  - Download aarch64 gcc and gcc-c++ (ensure it is more or less the same version as on the Raspberry pi);
  - Install distcc (or distcc-server/distccd depending on your distro);
  - Update symlinks by running `update-distcc-symlinks`;
  - Make sure that the distcc server is configured properly:
    - For Fedora, the start up config of distccd is found in /etc/sysconfig/distccd (it may differ for other distros).
    - Make sure no other port is set, if it is, delete the port argument to ensure default one is used.
    - Make sure the --allow argument allows your network interface. For example, if your network interface is 192.168.1.0, set the --allow to 192.168.1.0/24.
  - Start the service, with systemd it is `sudo systemcl start distccd` or `sudo systemcl restart distccd` if it was already started and you want config changes to take effect.
- On the RPi:
  - Install essentials `sudo apt install build-essential cmake unzip pkg-config gfortran gcc g++ gperf flex texinfo gawk bison openssl pigz libncurses-dev autoconf automake tar figlet`; 
  - Install distcc `sudo apt install distcc`;
  - Add the IP of host Linux machine to ~/.distcc/hosts or/and /etc/distcc/hosts
  - Install Wt dependencies `sudo apt install libboost-all-dev`;
  - Get the latest release of Wt, extract it (`tar xvf _release_name.tar.gz_`) and cd in to it (`cd _release_name_`);
  - Create a build folder and enter it (`mkdir build`, `cd build`);
  - Configure the project with cmake with the following command: `CC=/usr/lib/distcc/aarch64-linux-gnu-gcc CXX=/usr/lib/distcc/aarch64-linux-gnu-g++ cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON ../`;
  - Run `make -j4` to compile. You should not get any distcc errors at this stage, if you do, something is wrong with the distcc config. If that's the case, please open an issue so I could update the guide.
  - If compilation is successful, run `sudo make install` and you are done.

 ### Installing and running the actual project
 Simply clone the repository, create a build directory and enter it within the project folder, run `cmake ../` and `make`.

 To start the program, run `./ledserver.wt --docroot /usr/local/share/Wt/ --http-listen 0.0.0.0:8080` and then enter your RPi's ip with the port 8080 in your browser to view the web page.
