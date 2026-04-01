
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

//
//#define DEBUG_ESP32_READ
#define ESP32_DATA_SZ 1024  // upper read limit, not packet size

//
#define PTHREAD_MUTEX_LOCK(mtx)                             \
    do {                                                    \
        if (pthread_mutex_init(&mtx, NULL) != 0)            \
        {                                                   \
            printf("ERROR: mutext not initialized.\n");     \
            exit(1);                                        \
        }                                                   \
    } while (0);


//----------------------------------------------------------------------------------------
// Globals

// packet signature, used for alignment
const char *__global_packet_id = "ESPD";


// data buffers and mutex

// mutex for data array read/write
pthread_mutex_t __global_data_mtx;  
array_t<data_t> *__global_data_buffer = NULL;
// copy of the data buffer, to prevent unnecessary blocking the serial read thread
array_t<data_t> *__global_data_buffer_cpy = NULL;
// keeps track of time of packet arrivals, protected under the __global_data_mtx
array_t<float> *__global_data_time_buffer = NULL;

// raylib
IVector2 __global_window_dim = {0};
Plotter *__global_plotter = NULL;

// serial I/O mutex and signals
pthread_mutex_t __global_esp32_signal_mtx;  // signaling mutex for the esp32 thread
bool __global_esp32_disconnect = false; // thread stop signal condition
pthread_mutex_t __global_serial_mutex;

//----------------------------------------------------------------------------------------
// ESP32 read thread function
void *read_ESP32_data(void *_connection)
{
    SerialESP32 *serial = (SerialESP32 *)_connection;
    data_point_t data_packet = { 0 };
    struct timeval timeval0, timeval1;
    float tstart, tnow;
    // start time of (formatted as float s.ms).
    gettimeofday(&timeval0, NULL);
    tstart = timeval0.tv_sec + timeval0.tv_usec * 1e-6;

    //
    printf("%s(): reading ESP32 data.\n", __func__);

    //
    while (1)
    {
        pthread_mutex_lock(&__global_serial_mutex);
        int read_bytes = serial->recieve();
        data_t dat = *(data_t *)serial->data();
        pthread_mutex_unlock(&__global_serial_mutex);

        if (read_bytes == sizeof(data_t))
        {
            // TODO : 
            // implement correction of misaligned packets in case of overwhelming data?
            //
            assert(dat.id[0]==__global_packet_id[0] &&
                   dat.id[1]==__global_packet_id[1] &&
                   dat.id[2]==__global_packet_id[2] &&
                   dat.id[3]==__global_packet_id[3]);

            // get timestamp as float s.ms.
            gettimeofday(&timeval1, NULL);
            tnow = (timeval1.tv_sec + timeval1.tv_usec * 1e-6) - tstart;
            // push the data 
            pthread_mutex_lock(&__global_data_mtx);
            __global_data_buffer->push(dat);
            __global_data_time_buffer->push(tnow);
            pthread_mutex_unlock(&__global_data_mtx);
        }
        
        usleep(10);

        pthread_mutex_lock(&__global_esp32_signal_mtx);
        if (__global_esp32_disconnect == true)
        {
           printf("%s(): exit signal recieved\n", __func__);
           break;
        }
        pthread_mutex_unlock(&__global_esp32_signal_mtx);
    }

    return NULL;

}

//----------------------------------------------------------------------------------------
float scale_y_to_window(float _y)
{
    // requires max and min to be set
    static float ymax = 150.0f;
    static float ymin = 0.0f;
    //
    float y_scaled = _y / (ymax - ymin);
    return __global_window_dim.y - (y_scaled * __global_window_dim.y);

}

//----------------------------------------------------------------------------------------
void draw_scene()
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

    ClearBackground({60, 60, 60, 255});
    __global_plotter->draw();
    
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

    // initialize mutexes
    PTHREAD_MUTEX_LOCK(__global_esp32_signal_mtx);
    PTHREAD_MUTEX_LOCK(__global_data_mtx);
    PTHREAD_MUTEX_LOCK(__global_serial_mutex);

    // initialize serial connection to ESP32
    SerialESP32 serial(port.c_str(), ESP32_DATA_SZ);
    serial.connect();

    // initalize raylib
    SetConfigFlags(FLAG_VSYNC_HINT); // FLAG_WINDOW_TOPMOST | FLAG_WINDOW_UNDECORATED | 
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1600, 900, "ESP32 I/O");
    __global_window_dim.x = 1600;//GetScreenWidth();
    __global_window_dim.y = 900;//GetScreenHeight();
    SetWindowPosition(0, 0);
    SetTargetFPS(60);
    // load fonts
    __rc.font18 = LoadFontEx("./font/UbuntuMono-Regular.ttf", __rc.font18_size, 0, 0);
    __rc.font24 = LoadFontEx("./font/UbuntuMono-Regular.ttf", __rc.font24_size, 0, 0);

    // plotter, responsible for plotting interleaved serial data into subplots
    __global_plotter = new Plotter(__global_window_dim, 
                                   4, 
                                   { 2, 2 },
                                   {"signal A", 
                                    "signal B", 
                                    "signal C", 
                                    "signal D"});

    // dimension the data buffers (written under mutex lock from the serial thread)
    // the max size of the buffers is equal to the size (in pixels) of one of the 
    // Plotter's subplots.
    //
    size_t subplot_width = __global_plotter->get_subplot_vertex_count();
    // int subplot_width = __global_plotter->get_subplot_dims().x;
    __global_data_buffer = new array_t<data_t>((size_t)subplot_width);
    __global_data_buffer_cpy = new array_t<data_t>((size_t)subplot_width);
    __global_data_time_buffer = new array_t<float>((size_t)subplot_width);
    
    // start ESP32 serial reading
    pthread_t esp32_read_thread_id;
    pthread_create(&esp32_read_thread_id, NULL, read_ESP32_data, (void *)&serial);

    // main thread main loop
    while (!WindowShouldClose())
    {
        // pull events
        if (IsKeyPressed(KEY_W))
        {
            pthread_mutex_lock(&__global_serial_mutex);
            serial.send(KEY_W);
            pthread_mutex_unlock(&__global_serial_mutex);
        }
        else if (IsKeyPressed(KEY_S))
        {
            pthread_mutex_lock(&__global_serial_mutex);
            serial.send(KEY_S);
            pthread_mutex_unlock(&__global_serial_mutex);
        }

        //
        draw_scene();
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

    //
    return 0;

}
