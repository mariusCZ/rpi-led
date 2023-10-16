#include <termios.h> // Contains POSIX terminal control definitions
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#include <stdio.h>
#include <string.h>

class SerialDevice
{
public:
    SerialDevice(char device[]);
    ~SerialDevice();
    void writeSerial(unsigned char data[]);
    void readSerial();
private:
    struct termios tty;
    int serial_port;
};