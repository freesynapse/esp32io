
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <cstdlib>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>

#include <raylib.h>

#include "types.h"
#include "array.h"
#include "serial.h"
#include "plotter.h"
#include "timer.h"
#include "thread.h"
#include "gui.h"

//
#define PTHREAD_MUTEX_INIT(mtx)                                 \
    do {                                                        \
        if (pthread_mutex_init(&mtx, NULL) != 0)                \
        {                                                       \
            fprintf(stderr, "ERROR: mutext not initialized.\n");\
            exit(1);                                            \
        }                                                       \
    } while (0);


//----------------------------------------------------------------------------------------
// Globals

// data buffers and mutex

// the data bufffer protected under __global_data_mtx
array_t<data_t> *__global_data_buffer = NULL;
// copy of the data buffer, to prevent unnecessary blocking the serial read thread
array_t<data_t> *__global_data_buffer_cpy = NULL;
// keeps track of time of packet arrivals, protected under the __global_data_mtx
array_t<float> *__global_data_time_buffer = NULL;

// raylib
Vector2 __global_window_dim = {0};
Plotter *__global_plotter = NULL;


//----------------------------------------------------------------------------------------
// ESP32 read thread function
void *read_ESP32_data(void *_connection)
{
    SerialESP32 *serial = (SerialESP32 *)_connection;

    // timers and timings
    struct timeval timeval0, timeval1;
    float tstart, tnow;
    
    float esp_read_ms = 0.0f;
    float esp_signal_freq = 0.0f;
    uint64_t it = 0;
    float it_ms = 0.0f;

    // start time of (formatted as float s.ms).
    gettimeofday(&timeval0, NULL);
    tstart = timeval0.tv_sec + timeval0.tv_usec * 1e-6;
    
    // buffer to accumulate read bytes into
    char read_buffer[ESP32_DATA_SZ] = { 0 };
    size_t read_buffer_sz = 0;

    // flush tty and start reading
    serial->flush();
    while (1)
    {
        Timer t0;

        pthread_mutex_lock(&__global_serial_mtx);
        // read from serial
        int read_bytes = serial->recieve();
        // append data to read buffer
        memcpy(read_buffer + read_buffer_sz, serial->data(), read_bytes);
        read_buffer_sz += read_bytes;
        pthread_mutex_unlock(&__global_serial_mtx);

        // if (read_bytes == sizeof(data_t))
        if (read_buffer_sz >= sizeof(data_t))
        {
            data_t dat = *(data_t *)read_buffer;

            // check signature
            if (strcmp(dat.id, __global_packet_id) != 0)
            {
                // read the data one char at the time
                char *p = &read_buffer[0];
                size_t n = 0;
                // while (n < sizeof(data_t) - __global_packet_id_len  && read_buffer_sz > sizeof(data_t))
                while (n + __global_packet_id_len < read_buffer_sz)
                {
                    if (*(p+0) == __global_packet_id[0] && *(p+1) == __global_packet_id[1] && 
                        *(p+2) == __global_packet_id[2] && *(p+3) == __global_packet_id[3])
                    {
                        // found alignment boundary, moving data
                        printf("WARNING : packet misalignment (%ld byte(s))\n", n);
                        break;
                    }
                    n++;
                    p++;
                }
                memmove(read_buffer, p, read_buffer_sz - n);
                read_buffer_sz -= n;
            }
            else
            {
                // packet was aligned, so move the data for capturing the next packet
                // printf("aligned : read_buffer_sz %ld -> %ld\n", read_buffer_sz, read_buffer_sz - sizeof(data_t));
                read_buffer_sz -= sizeof(data_t);
                memmove(read_buffer, read_buffer + sizeof(data_t), read_buffer_sz);

                // get timestamp as float s.ms.
                gettimeofday(&timeval1, NULL);
                tnow = (timeval1.tv_sec + timeval1.tv_usec * 1e-6) - tstart;
                // push the data 
                pthread_mutex_lock(&__global_data_mtx);
                __global_data_buffer->push(dat);
                __global_data_time_buffer->push(tnow);
                
                // TODO : make this more general somehow?
                __global_write_data.ival = dat.ival[0];
                
                pthread_mutex_unlock(&__global_data_mtx);

            }

            // calculate and store frequency of incoming packets
            esp_signal_freq = 1000.0f / esp_read_ms;
            esp_read_ms = 0.0f;

        }
        
        usleep(20);

        pthread_mutex_lock(&__global_esp32_signal_mtx);
        if (__global_esp32_disconnect == true)
        {
           printf("%s(): exit signal recieved\n", __func__);
           break;
        }
        pthread_mutex_unlock(&__global_esp32_signal_mtx);

        // update timers and frequency
        esp_read_ms += t0.getDeltaTimeMs();
        it_ms += t0.getDeltaTimeMs();

        // update esp signal frequency once every ~1 second
        if (it_ms > 1000.0f)
        {
            pthread_mutex_lock(&__global_esp32_signal_freq_mtx);
            __global_esp32_signal_freq = esp_signal_freq;
            pthread_mutex_unlock(&__global_esp32_signal_freq_mtx);
            it_ms -= 1000.0f;
        }

        it++;

    }

    return NULL;

}

//----------------------------------------------------------------------------------------
void draw()
{
    // copy data buffer for drawing
    pthread_mutex_lock(&__global_data_mtx);
    // make a copy of the data buffer, to prevent the ESP32io thread being blocked
    memcpy(__global_data_buffer_cpy->data_ptr, 
           __global_data_buffer->data_ptr, 
           sizeof(data_t) * __global_data_buffer->size);
    __global_data_buffer_cpy->size = __global_data_buffer->size;
    pthread_mutex_unlock(&__global_data_mtx);

    //
    __global_plotter->copy_data_to_vertices(__global_data_buffer_cpy->data_ptr, 
                                            __global_data_buffer_cpy->size);

    BeginDrawing();

    ClearBackground(__rc.background_main_color);
    __global_plotter->draw();

    if (__global_is_showing_popup)
    {
        static Vector2 dim = { 400, 400 };
        Vector2 pos = { .x = __global_window_dim.x / 2 - dim.x / 2, 
                        .y = __global_window_dim.y / 2 - dim.y / 2 };
        draw_help_popup(pos.x, pos.y, dim.x, dim.y, "-- HELP --");
    }

    EndDrawing();
}

//----------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    // asserts for data_t field sizes
    assert(sizeof(float) == 4);
    assert(sizeof(uint32_t) == 4);

    std::string port = "/dev/ttyUSB0";
    if (argc > 1)
        port = std::string(argv[1]);

    // initialize mutexes, exits on error
    PTHREAD_MUTEX_INIT(__global_esp32_signal_mtx);
    PTHREAD_MUTEX_INIT(__global_data_mtx);
    PTHREAD_MUTEX_INIT(__global_serial_mtx);
    PTHREAD_MUTEX_INIT(__global_esp32_signal_freq_mtx);

    // initialize serial connection to ESP32
    __global_serial_esp32 = new SerialESP32(port.c_str(), ESP32_DATA_SZ);
    if (__global_serial_esp32->connect() != RETURN_SUCCESS)
    {
        fprintf(stderr, "ERROR %d: opening %s: %s\n", errno, port.c_str(), strerror (errno));
        exit(1);
    }

    // initalize raylib
    SetConfigFlags(FLAG_VSYNC_HINT); // FLAG_WINDOW_TOPMOST | FLAG_WINDOW_UNDECORATED | 
    SetTraceLogLevel(LOG_WARNING);
    //InitWindow(0, 0, "ESP32 I/O");
    //__global_window_dim.x = GetScreenWidth();
    //__global_window_dim.y = GetScreenHeight();
    //SetWindowPosition(0, 0);
    InitWindow(1600, 900, "ESP32 I/O");
    __global_window_dim.x = 1600;//GetScreenWidth();
    __global_window_dim.y = 900;//GetScreenHeight();
    SetWindowPosition(GetScreenWidth(), 0);
    printf("%d %d\n", GetScreenWidth(), GetScreenHeight());
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);
    // load fonts
    __rc.font18 = LoadFontEx("./font/UbuntuMono-Regular.ttf", __rc.font18_size, 0, 0);
    __rc.font24 = LoadFontEx("./font/UbuntuMono-Regular.ttf", __rc.font24_size, 0, 0);

    // plotter, responsible for plotting interleaved serial data into subplots
    __global_plotter = new Plotter(__global_window_dim, 
                                   4, 
                                   { 2, 2 },
                                   {"variable_0", 
                                    "variable_1", 
                                    "variable_2", 
                                    "variable_3"});

    // dimension the data buffers (written under mutex lock from the serial thread)
    // the max size of the buffers is equal to the size (in pixels) of one of the 
    // Plotter's subplots.
    //
    size_t subplot_width = __global_plotter->get_subplot_vertex_count();
    __global_data_buffer = new array_t<data_t>((size_t)subplot_width);
    __global_data_buffer_cpy = new array_t<data_t>((size_t)subplot_width);
    __global_data_time_buffer = new array_t<float>((size_t)subplot_width);
    
    // start ESP32 serial reading
    pthread_t esp32_read_thread_id;
    pthread_create(&esp32_read_thread_id, NULL, read_ESP32_data, (void *)__global_serial_esp32);

    // main thread main loop
    bool window_should_close = false;
    while (!window_should_close)
    {
        if (IsKeyPressed(KEY_F1))
            __global_is_showing_popup = !__global_is_showing_popup;

        // adjust value on arrow keys
        else if (IsKeyPressed(KEY_UP))
        {
            int delta = 0;
            if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) delta = 10;
            else if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) delta = 100;
            else delta = 1;
            __global_write_data.ival += delta;
            pthread_mutex_lock(&__global_serial_mtx);
            __global_write_data.ival = MIN(__global_write_data.ival, WRITE_INT_MAX);
            __global_serial_esp32->send(&__global_write_data, sizeof(write_data_t));
            pthread_mutex_unlock(&__global_serial_mtx);
        }
        else if (IsKeyPressed(KEY_DOWN))
        {
            int delta = 0;
            if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) delta = 10;
            else if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) delta = 100;
            else delta = 1;
            pthread_mutex_lock(&__global_serial_mtx);
            __global_write_data.ival -= delta;
            __global_write_data.ival = MAX(0, __global_write_data.ival);
            __global_serial_esp32->send(&__global_write_data, sizeof(write_data_t));
            pthread_mutex_unlock(&__global_serial_mtx);
        }

        //
        else if (IsKeyPressed(KEY_ESCAPE))
        {
            if (__global_is_showing_popup)
                __global_is_showing_popup = !__global_is_showing_popup;
            else
                window_should_close = true;
        }
            


        // }

        //
        draw();
    }

    CloseWindow();

    // signal thread and wait for it to halt
    pthread_mutex_lock(&__global_esp32_signal_mtx);
    __global_esp32_disconnect = true;
    pthread_mutex_unlock(&__global_esp32_signal_mtx);
    pthread_join(esp32_read_thread_id, NULL);

    delete __global_data_buffer;
    delete __global_data_buffer_cpy;
    delete __global_data_time_buffer;
    delete __global_plotter;
    delete __global_serial_esp32;

    //
    return 0;

}
