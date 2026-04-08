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
//#define MAX_INPUT_CHARS 16
//char __global_text_input_buffer[MAX_INPUT_CHARS + 1] = { '\0' };
//int __global_text_input_sz = 0;
bool __global_is_showing_popup = false;

// helper function to get value from text box. Begging for templated classes this thing is...
/*
uint32_t get_text_box_val()
{
    uint32_t val = 0;
    if (strlen(__global_text_input_buffer) > 0)
    {
        sscanf(__global_text_input_buffer, "%d", &val);

        if (val > WRITE_INT_MAX)
        {
            fprintf(stderr, "WARNING: write_data_t.ival > WRITE_INT_MAX (%s)\n", WRITE_INT_MAX_STR);
            val = CLAMP(val, 0, 1024);
            memset(__global_text_input_buffer, '\0', MAX_INPUT_CHARS + 1);
            strcpy(__global_text_input_buffer, WRITE_INT_MAX_STR);
            __global_text_input_sz = strlen(WRITE_INT_MAX_STR);

        }
    }

    return val;
}
*/

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

    const char *info = "<UP>     : increase delay by 1\n"
                       "<DOWN>   : decrease delay by 1\n"
                       "<+CTRL>  : adjust change by 10\n"
                       "<+SHIFT> : adjust change by 100\n"
                       "<ESC>    : close help window";

    Vector2 info_sz = measure_text(info);
    draw_text(info, { .x = _xpos + 10, .y = new_y + 10 });
}



// Handles key events in active window/popup, uses the global 'input_buffer' (above). 
// N.B.: redirects here from main loop in main.cpp
/*
void handle_popup_input()
{
    // check if any chars in raylib's input buffer
    int key = GetCharPressed();
    while (key > 0)
    {
        // only characters ['0' .. '9'] allowed
        if (key >= 48 && key <= 57 && __global_text_input_sz < MAX_INPUT_CHARS)
        {
            __global_text_input_buffer[__global_text_input_sz] = (char)key;
            __global_text_input_buffer[__global_text_input_sz + 1] = '\0';
            __global_text_input_sz++;
        }

        key = GetCharPressed(); // next char in buffer
    }

    // erase on backspace
    if (IsKeyPressed(KEY_BACKSPACE))
    {
        if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
        {
            memset(__global_text_input_buffer, '\0', MAX_INPUT_CHARS + 1);
            __global_text_input_sz = 0;
        }
        else
        {
            __global_text_input_sz--;
            if (__global_text_input_sz < 0)
                __global_text_input_sz = 0;
            __global_text_input_buffer[__global_text_input_sz] = '\0';
        }
    }

    // adjust value on arrow keys
    else if (IsKeyPressed(KEY_UP))
    {
        uint32_t val = get_text_box_val();
        if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
            val += 10;
        else
            val += 1;
        
        val = CLAMP(val, 0, WRITE_INT_MAX);
        memset(__global_text_input_buffer, '\0', MAX_INPUT_CHARS + 1);
        __global_text_input_sz = sprintf(__global_text_input_buffer, "%d", val);
    }
    else if (IsKeyPressed(KEY_DOWN))
    {
        uint32_t val = get_text_box_val();
        if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
            val -= 10;
        else
            val -= 1;
        
        val = CLAMP(val, 0, WRITE_INT_MAX);
        memset(__global_text_input_buffer, '\0', MAX_INPUT_CHARS + 1);
        __global_text_input_sz = sprintf(__global_text_input_buffer, "%d", val);
    }

    // write on enter
    else if (IsKeyPressed(KEY_ENTER))
    {
        write_data_t d;
        d.ival = get_text_box_val();

        if (strlen(__global_text_input_buffer) > 0)
        {
            pthread_mutex_lock(&__global_serial_mtx);
            __global_serial_esp32->send(&d, sizeof(d));
            pthread_mutex_unlock(&__global_serial_mtx);
            printf("INFO : sent %ld byte(s)\n", sizeof(d));
        }
        else
            printf("WARNING : buffer empty.\n");

        __global_is_showing_popup = false;
    }

    // close popup on <ESCAPE>
    else if (IsKeyPressed(KEY_ESCAPE))
        __global_is_showing_popup = false;    
}
*/

// input text box for delay adjustment
/*
void write_to_serial_popup(float _xpos, float _ypos, float _w, float _h)
{
    static int it = 0;

    const char *ex_t = "0";
    static Vector2 font_sz = measure_text(ex_t);
    static int xoffset = 10;
    static int yoffset = 10;
    static int text_xoffset = 5;

    float ypos = draw_window(_xpos, _ypos, _w, _h, "WRITE TO ESP32");
    float h = _h - ypos;

    // input textbox
    static Vector2 label_sz = measure_text("Delay [0..1024] ");
    draw_text("Delay [0..1024] ", { .x = _xpos + xoffset, .y = ypos + yoffset + 5});
    Rectangle textbox = { .x = _xpos + xoffset + label_sz.x, .y = ypos + yoffset, .width = (MAX_INPUT_CHARS + 1) * (font_sz.x +  + __rc.font18_spacing) + text_xoffset, .height = font_sz.y + 10 };
    DrawRectangleLinesEx(textbox, 2.0f, __rc.edge_popup_color);

    // draw input
    static Vector2 text_pos = { .x = textbox.x + text_xoffset, .y = textbox.y + 5 };
    draw_text(__global_text_input_buffer, { .x = text_pos.x, .y = text_pos.y });

    // draw cursor
    if (((it / 20) % 2) == 0)
    {
        Vector2 input_text_sz = measure_text(__global_text_input_buffer);
        draw_text("|", { .x = text_pos.x + input_text_sz.x, .y = text_pos.y });
    }

    const char *info = "<ENTER>         : write to serial\n"
                       "<ESC>           : exit\n"
                       "<UP> (<CTRL>)   : increase delay\n"
                       "<DOWN> (<CTRL>) : decrease delay";
    Vector2 info_sz = measure_text(info);
    draw_text(info, { .x = _xpos + xoffset, .y = _ypos + _h - info_sz.y - 5});

    it++;
}
*/


#endif // __GUI_H
