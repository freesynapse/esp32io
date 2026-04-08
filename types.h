#ifndef __TYPES_H
#define __TYPES_H

#include <stdio.h>
#include <stdint.h>
#include <vector>

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
write_data_t __global_write_data = { 0 };

#endif // __TYPES_H
