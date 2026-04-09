#ifndef __TYPES_H
#define __TYPES_H

#include <stdio.h>
#include <stdint.h>
#include <vector>

// some debug macros
char __debug_trace_buffer[64] = { "\0" };
size_t __debug_trace_width = 40;

char *__debug_trace_fmt(const char *_file, const char* _func, int _line)
{
    memset(__debug_trace_buffer, '\0', 64);
    char b[64] = { "\0 "};
    int n = sprintf(b, "%s[%d]:%s()", _file, _line, _func);
    if (n > __debug_trace_width) __debug_trace_width = n;    
    memcpy(__debug_trace_buffer, b, n);
    memset(__debug_trace_buffer + n, ' ', __debug_trace_width - n);
    return __debug_trace_buffer;
}
#define LOG_INFO(msg, ...) do { fprintf(stderr, "%s [INFO]    " msg "", __debug_trace_fmt(__FILE__, __func__, __LINE__), ##__VA_ARGS__); } while(0)
#define LOG_WARNING(msg, ...) do { fprintf(stderr, "%s [WARNING] " msg "", __debug_trace_fmt(__FILE__, __func__, __LINE__), ##__VA_ARGS__); } while(0)
#define LOG_ERROR(msg, ...) do { fprintf(stderr, "%s [ERROR]   " msg "", __debug_trace_fmt(__FILE__, __func__, __LINE__), ##__VA_ARGS__); } while(0)
#define ARRAY_LEN(x) (sizeof((x)) / sizeof((x)[0]))
//
// ESP32-related data structures
// the data_t struct is extendable, matching the data sent in binary from the ESP32
typedef struct
{
    char id[4];
    float fval[2];
    int32_t ival[2];
    void __debug_print()
    {
        printf("data_t: '%s', %.2f, %.2f, %d, %d\n", id, fval[0], fval[1], ival[0], ival[1]);
    }
} data_t;

// packet signature, used for alignment
const char *__global_packet_id = "ESPD";
size_t __global_packet_id_len = strlen(__global_packet_id);

// sampling count -- how many data packets (when plotting) are averaged
#define MIN_SAMPLE_COUNT  1
#define MAX_SAMPLE_COUNT 16
float __global_sample_count = 1.0f;

// for reflection of the data types in data_t
typedef enum 
{
    FLOAT = 0,
    UINT32,
    INT32,
} types;
//
std::vector<types> data_t_types = { FLOAT, 
                                    FLOAT, 
                                    INT32, 
                                    INT32 };

// packets sent in binary on SerialESP32->send()
#define WRITE_INT_MAX 1024
#define __WRITE_INT_MAX_STR__(tok) #tok
#define __WRITE_INT_MAX_STR___(tok) __WRITE_INT_MAX_STR__(tok)
#define WRITE_INT_MAX_STR __WRITE_INT_MAX_STR___(WRITE_INT_MAX)

typedef struct
{
    int32_t ival;
} write_data_t;
//
write_data_t __global_write_data = { .ival = 100 };


#endif // __TYPES_H
