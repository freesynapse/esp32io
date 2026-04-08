#ifndef __THREAD_H
#define __THREAD_H

#include <pthread.h>

// mutex for data read/write
pthread_mutex_t __global_data_mtx;

// ESP32 serial I/O mutex and signals
pthread_mutex_t __global_esp32_signal_mtx;  // signaling mutex for the esp32 thread
bool __global_esp32_disconnect = false; // thread stop signal condition
pthread_mutex_t __global_serial_mtx;

pthread_mutex_t __global_esp32_signal_freq_mtx;
float __global_esp32_signal_freq = 0.0f;    // frequency of incoming packets from esp32


#endif // __THREAD_H
