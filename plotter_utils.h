#ifndef __PLOTTER_UTILS_H
#define __PLOTTER_UTILS_H

#include <math.h>
#include <raylib.h>

//
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi) (MIN((hi), MAX((lo), (x))))

# define __force_inline __attribute__((always_inline)) inline

//
void __debug_Vector2(Vector2 _v, const char *_prefix="")
{
    if (strcmp(_prefix, "") != 0)
        printf("%s: [ %.2f  %.2f ]\n", _prefix, _v.x, _v.y);
    else
        printf("Vector2: [ %.2f  %.2f ]\n", _v.x, _v.y);
}

// figure parameters
class rc_t
{
public:
    float menu_bar_height   = 30.0f;

    bool draw_subplot_div   = true;
    Color div_color         = { 100, 100, 100, 255 };
    float div_line_width    = 2.0f;

    bool draw_axes          = true;
    Vector2 axes_llim       = { 120.0f,  50.0f };
    Vector2 axes_rlim       = {  70.0f, 100.0f };
    float max_ticks         = 5.0f;
    Vector2 axes_ticklength = {  20.0f,  20.0f };
    Color axes_color        = { 255, 255, 255, 255 };
    float axes_line_width   = 1.0f;

    bool draw_y_minmax_vals = true;
    bool draw_yticks        = true;
    bool draw_title         = true;
    bool draw_current_y     = true;

    Color colors[6] = {
        { 255,   0,   0, 255 },
        {   0, 255,   0, 255 },
        { 255,   0, 255, 255 },
        {   0, 255, 255, 255 },
        { 255, 255, 255, 255 },
        {   0,   0, 255, 255 },
    };

    Color background_main_color   = {60, 60, 60, 255};
    Color background_popup_color  = {30, 30, 30, 255};
    Color edge_popup_color        = {90, 90, 90, 255};
    // regular font
    Font font18;
    float font18_size       = 18;
    float font18_spacing    = 2;
    Color font18_color      = { 255, 255, 255, 255 };

    // title font
    Font font24;
    float font24_size       = 24;
    float font24_spacing    = 2;
    Color font24_color      = { 255, 255, 255, 255 };

};
// global instance
rc_t __rc;

// wrappers for very verbose raylib function calls
__force_inline void draw_text(const char *_text, Vector2 _pos) { DrawTextEx(__rc.font18, _text, _pos, __rc.font18.baseSize, __rc.font18_spacing, __rc.font18_color); }
__force_inline void draw_text_large(const char *_text, Vector2 _pos) { DrawTextEx(__rc.font24, _text, _pos, __rc.font24.baseSize, __rc.font24_spacing, __rc.font24_color); }
__force_inline Vector2 measure_text(const char *_text) { return MeasureTextEx(__rc.font18, _text, __rc.font18_size, __rc.font18_spacing); }
__force_inline Vector2 measure_text_large(const char *_text) { return MeasureTextEx(__rc.font24, _text, __rc.font24_size, __rc.font24_spacing); }


// Functions for getting nice scale and ticks on subplot axes
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
Vector2 calculate_range(float _min, float _max, float *_tick_spacing)
{
    float range = nice_num(_max - _min, false);

    float tick_spacing = nice_num(range / (__rc.max_ticks - 1.0f), true);
    Vector2 nice_lim = { 0 };
    nice_lim.x = floor(_min / tick_spacing) * tick_spacing;
    nice_lim.y = ceil(_max / tick_spacing) * tick_spacing;
    
    // update parameters
    *_tick_spacing = tick_spacing;
    return nice_lim;
}


#endif // __PLOTTER_UTILS_H
