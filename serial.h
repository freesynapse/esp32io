#ifndef __SERIAL_H
#define __SERIAL_H

#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <stdio.h>
#include <string>
#include <pthread.h>

#define BAUDRATE        B115200
#define RETURN_SUCCESS        0
#define RETURN_FAILURE        1
#define ESP32_DATA_SZ      1024  // upper read limit (in bytes), not packet size


//
class SerialESP32
{
public:
    //
    SerialESP32(const char *_port, size_t data_size, int _baud_rate=BAUDRATE) :
        m_baudrate(_baud_rate), m_port(_port), m_dataSz(data_size)
    {
        m_buffer = new char[data_size];
    }
    
    //
    ~SerialESP32()
    {
        if (m_fd >= 0)
            close(m_fd);

        if (m_buffer)
            delete[] m_buffer;

        printf("Serial connection to '%s' closed.\n", m_port.c_str());
    }

    //
    int connect()
    {
        printf("Opening connection to '%s'.\n", m_port.c_str());
        
        m_fd = open(m_port.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
        if (m_fd < 0)
            return RETURN_FAILURE;

        usleep(100000);

        // serial port parameters
        struct termios newtio;
        memset(&newtio, 0, sizeof(newtio));
        struct termios oldtio;
        tcgetattr(m_fd, &oldtio);

        newtio = oldtio;
        newtio.c_cflag = m_baudrate | CS8 | CLOCAL | CREAD;
        newtio.c_iflag = 0;
        newtio.c_oflag = 0;
        newtio.c_lflag = 0;
        newtio.c_cc[VMIN] = 0;
        newtio.c_cc[VTIME] = 0;
        tcflush(m_fd, TCIFLUSH);

        cfsetispeed(&newtio, m_baudrate);
        cfsetospeed(&newtio, m_baudrate);
        tcsetattr(m_fd, TCSANOW, &newtio);
        fcntl(m_fd, F_SETFL, 0);

        return RETURN_SUCCESS;
    }

    //
    int recieve()
    {
        memset(m_buffer, 0, m_dataSz);
        int n = read(m_fd, m_buffer, m_dataSz);
        return n;
    }

    //
    int send(void *_data, size_t _n_bytes)
    {
        int n = write(m_fd, _data, _n_bytes);
        return n;
    }

    // accessors
    char *data()        {   return m_buffer;    }
    size_t data_size()  {   return m_dataSz;    }
    void flush()        {   tcflush(m_fd, TCIFLUSH); }


public:
    char *m_buffer;
    size_t m_dataSz = 0;
    std::string m_port = "";
    int m_fd = -1;
    int m_baudrate = -1;
    pthread_mutex_t *m_mutex_ptr = NULL;

};

SerialESP32 *__global_serial_esp32 = nullptr;






#endif // __SERIAL_H

