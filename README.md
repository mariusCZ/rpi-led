# Raspberry Pi web controlled LEDs
## Introduction
This is a project that allows control of WS281x LEDs from a web page with a particular focus on wireless audio visualization. Currently, it still lacks a lot of functionality as it is a work in progress, but the audio visualization part is currently functional. The other projects that actually transmit audio data in to the RPi web server will be published soon.
# Documentation
I will be adding proper documentation later on. As it is right now it's not exactly easy to build this project. However, if you would like to give it a shot before I write documentation, three external dependencies are needed:

- [ixwebsocket](https://machinezone.github.io/IXWebSocket/build/)
- [ws281x RPi userspace library](https://github.com/jgarff/rpi_ws281x)
- (Wt)[https://github.com/emweb/wt]

While installing ixwebsocket and ws281x library is not hard, the Wt installation guide on their site is quite out of date and some significant tinkering was needed to finally cross-compile it to an RPi.

Currently, this project uses GPIO18 for the data output to the LEDs.
