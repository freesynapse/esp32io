#ifndef __TYPES_H
#define __TYPES_H

#include <stdio.h>
#include <stdint.h>
#include <vector>

//
typedef struct 
{
    int x;
    int y;

    void __debug_print(const char *_prefix="") 
    {
        printf("IVector2 %s { %d, %d }\n", _prefix, x, y);
    }

} IVector2;

//----------------------------------------------------------------------------------------
// ESP32-related data structures
// the data_t struct is extendable, matching the data sent in binary from the ESP32
typedef struct
{
    char id[4];
    float fval[2];
    uint32_t ival[2];
    void __debug_print()
    {
        printf("data_t: '%s', %.2f, %.2f, %d, %d\n", id, fval[0], fval[1], ival[0], ival[1]);
    }
} data_t;

enum class types
{
    FLOAT = 0,
    UINT32 = 1,
};

std::vector<types> data_t_types = { types::FLOAT, types::FLOAT, types::UINT32, types::UINT32 };

// timestamped, for comparisons and sorting
typedef struct
{
    data_t data;
    float time;
} data_point_t;


#endif // __TYPES_H
