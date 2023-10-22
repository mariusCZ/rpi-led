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

Installing ixwebsocket and ws281x is relatively straightforward. Get their latest releases, unpack them, run `cmake` to configure it and then `make && sudo make install` it.

### Compiling Wt
The main trouble is encountered when Wt needs to be compiled. The documentation for Raspberry Pi compilation is outdated on their page.

There are multiple ways to compile Wt, I will describe the two main ones I tried.

#### Compiling on the RPi
This is not recommended for several reasons. One is that compiling it on the RPi will take more than an hour and the second one is that its compilation takes a lot of memory, which may be problematic for the RPi. The general steps are:

- If you are using an RPi with more than 2GB of RAM, simply follow the instructions [here](https://www.webtoolkit.eu/wt/doc/reference/html/InstallationUnix.html).
- If you are using an RPi with less than 2GB of RAM, before following the instructions, make sure to increase swap size to at least 1 GB (instructions [here](https://nebl.io/neblio-university/enabling-increasing-raspberry-pi-swap/)) and make sure that you only use a single make job.

#### Compiling with distcc
TODO
