
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include "types.h"

#define BAUDRATE B115200


//
int main(int argc, char** argv)
{
    int us_delay = 5000;
    if (argc > 1)
        us_delay = atoi(argv[1]); 

    int fd = open("/dev/ptmx", O_RDWR | O_NOCTTY);
    if (fd == -1) {
        LOG_ERROR("%s\n", strerror(errno));
        return 1;
    }

    grantpt(fd);
    unlockpt(fd);

    const char *pts_name = ptsname(fd);
    LOG_INFO("ptsname: %s\n", pts_name);
    LOG_INFO("Data size: %ld\n", sizeof(data_t));

    // serial port parameters
    struct termios newtio;
    memset(&newtio, 0, sizeof(newtio));
    struct termios oldtio;
    tcgetattr(fd, &oldtio);

    newtio = oldtio;
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = 0;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VMIN] = 1;
    newtio.c_cc[VTIME] = 0;
    tcflush(fd, TCIFLUSH);

    cfsetospeed(&newtio, BAUDRATE);
    tcsetattr(fd, TCSANOW, &newtio);

    // payload
    uint32_t it = 0;
    data_t data = { 0 };
    strcpy(data.id, "ESPD");
    data.fval[0] = 100.0f;
    data.fval[1] = 20.0f;
    data.ival[0] = 0;
    data.ival[1] = 0;

    //
    while (1)
    {
        if (it >= 200)
        {
            it = 0;
            data.ival[0] = (data.fval[0] > 75.0f ? 0 : 10);
            data.fval[0] = (data.fval[0] > 75.0f ? 50.0f : 100.0f);
            data.ival[1] = (data.fval[1] > 10.0f ? 0 : 5);
            data.fval[1] = (data.fval[1] > 10.0f ? 0.0f : 20.0f);
            // data.__debug_print();
        }
        ssize_t n = write(fd, &data, sizeof(data));

        it++;

        usleep(us_delay);
    }

    close(fd);
    return 0;

}
