#include "../include/scomms.h"
#include <iostream>

/*
    Code for potentially receiving audio data through serial
    communications. Currently not used but kept in case needed
    for any future purposes.
*/

SerialDevice::SerialDevice(char device[]) {
    serial_port = open(device, O_RDWR);

    if(tcgetattr(serial_port, &tty) != 0) {
        //printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
        std::cout << "Error " << std::to_string(errno) << " from tcgetattr: " << strerror(errno) << std::endl;
    }

    tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
    tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
    tty.c_cflag &= ~CSIZE; // Clear all bits that set the data size 
    tty.c_cflag |= CS8; // 8 bits per byte (most common)
    tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ECHOE; // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable new-line echo
    tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

    tty.c_cc[VTIME] = 1;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;

    // Set in/out baud rate to be 9600
    cfsetispeed(&tty, B4000000);
    cfsetospeed(&tty, B2000000);

    // Save tty settings, also checking for error
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        //printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
        std::cout << "Error " << std::to_string(errno) << " from tcgsetattr: " << strerror(errno) << std::endl;
    }
}

SerialDevice::~SerialDevice() {
    close(serial_port);
}

void SerialDevice::writeSerial(unsigned char data[]) {
    unsigned char test[900] = { 0 };
    std::cout << "Write size " << sizeof(test) << std::endl;
    int ret = write(serial_port, test, sizeof(test));
    std::cout << "Write return " << ret << std::endl;
}

void SerialDevice::readSerial(){
    int num = 0;
    unsigned char buf;
    num = read(serial_port, &buf, 1);
}
