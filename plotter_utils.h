#ifndef __PLOTTER_UTILS_H
#define __PLOTTER_UTILS_H

#include <math.h>

#include <raylib.h>

//
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

//
void __debug_Vector2(Vector2 _v, const char *_prefix="")
{
    printf("Vector2 %s: [ %.2f  %.2f ]\n", _prefix, _v.x, _v.y);
}

// figure parameters
class rc_t
{
public:
    bool draw_subplot_div   = false;
    Color div_color         = { 100, 100, 100, 255 };
    float div_line_width    = 2.0f;

    bool draw_axes          = true;
    Vector2 axes_llim       = {  70.0f,  50.0f };
    Vector2 axes_rlim       = {  70.0f,  50.0f };
    Vector2 axes_hang       = {  20.0f,  20.0f };
    Color axes_color        = { 255, 255, 255, 255 };
    float axes_line_width   = 1.0f;

    bool draw_ticks         = false;

    

};
// global instance
rc_t __global_rc;

//
float nice_num(float _range, bool _round)
{
    float exponent;
    float fraction;
    float nice_fraction;

    exponent = floor(log10(_range));
    fraction = _range / pow(10.f, exponent);

    if (_round) 
    {   if (fraction < 1.5)
            nice_fraction = 1;
        else if (fraction < 3)
            nice_fraction = 2;
        else if (fraction < 7)
            nice_fraction = 5;
        else
            nice_fraction = 10;
    } 
    else 
    {   if (fraction <= 1)
            nice_fraction = 1;
        else if (fraction <= 2)
            nice_fraction = 2;
        else if (fraction <= 5)
            nice_fraction = 5;
        else
            nice_fraction = 10;
    }
    return nice_fraction * pow(10, exponent);                
}

//
Vector2 calculate_range(float _min, float _max)
{
    static float max_ticks = 10.0f;
    float range = nice_num(_max - _min, false);

    float tick_spacing = nice_num(range / (max_ticks - 1.0f), true);
    Vector2 nice_lim = { 0 };
    nice_lim.x = floor(_min / tick_spacing) * tick_spacing;
    nice_lim.y = ceil(_max / tick_spacing) * tick_spacing;
    
    // update parameters
    // range = nice_lim.y - nice_lim.x;
    return nice_lim;
}


#endif // __PLOTTER_UTILS_H
