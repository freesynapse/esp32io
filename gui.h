#ifndef __GUI_H
#define __GUI_H

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include <raylib.h>

#include "plotter_utils.h"
#include "thread.h"
#include "serial.h"
#include "types.h"


// globals
bool __global_is_showing_popup = false;

// create a raylib 'window'
float draw_window(float _xpos, float _ypos, float _w, float _h, const char *_title)
{
    Rectangle r = { .x = _xpos, .y = _ypos, .width = _w, .height = _h };
    DrawRectangleRec(r, __rc.background_popup_color);
    DrawRectangleLinesEx(r, 3.0f, __rc.edge_popup_color);

    static Vector2 title_sz = measure_text(_title);
    static float yoffset = 5.0f;
    draw_text(_title, { .x = _xpos + _w / 2 - title_sz.x / 2, .y = _ypos + yoffset });
    float ylo = _ypos + title_sz.y + yoffset * 2;
    DrawLineEx({ .x = _xpos, .y = ylo }, { .x = _xpos + _w, .y = ylo }, 3.0f, __rc.edge_popup_color);

    // return ypos of drawing area after title and sugar was added
    return ylo;
}

// display help
void draw_help_popup(float _xpos, float _ypos, float _w, float _h, const char *_title)
{
    float new_y = draw_window(_xpos, _ypos, _w, _h, _title);

    const char *info = "<UP>/<DOWN> : change delay by 1\n"
                       "<+CTRL>     : change delay by 10\n"
                       "<+SHIFT>    : change delay by 100\n"
                       "<RIGHT>     : increase sampling rate\n"
                       "<LEFT>      : decrease sampling rate\n"
                       "<ESC>       : close help window";

    static Vector2 info_sz = measure_text(info);

    draw_text(info, { .x = _xpos + _w / 2 - info_sz.x / 2, .y = new_y + 10 });

}

#endif // __GUI_H
