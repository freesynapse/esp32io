#ifndef __PLOTTER_H
#define __PLOTTER_H

#include <vector>
#include <string.h>
#include <string>
#include <assert.h>

#include <raylib.h>
#include <raymath.h>

#include "types.h"
#include "plotter_utils.h"
#include "thread.h"
#include "timer.h"


//
class Subplot
{
public:
    //
    Subplot(Vector2 _subplot_dims, Vector2 _offset, size_t _subplot_id, const std::string &_title)
    {
        m_subplot_id = _subplot_id;
        m_subplot_dims = { .x=_subplot_dims.x, .y=_subplot_dims.y };
        m_subplot_offset = { .x = _offset.x, .y = _offset.y };
        m_vertices = new Vector2[(int)_subplot_dims.x - (int)__rc.axes_llim.x];
        
        if (__rc.draw_axes == true)
            m_plotting_area = Vector2Subtract(Vector2Subtract(m_subplot_dims, __rc.axes_llim), __rc.axes_rlim);
        else
            m_plotting_area = m_subplot_dims;

        //
        m_title = _title;

        // calculate axes vertices
        m_axes_vertices[0] = 
        { 
            .x = m_subplot_offset.x + __rc.axes_llim.x - __rc.axes_ticklength.x,
            .y = m_subplot_offset.y + m_subplot_dims.y - __rc.axes_llim.y
        };

        m_axes_vertices[1] = 
        { 
            .x = m_subplot_offset.x + m_subplot_dims.x - __rc.axes_rlim.x,
            .y = m_subplot_offset.y + m_subplot_dims.y - __rc.axes_llim.y
        };

        m_axes_vertices[2] = 
        {
            .x = m_subplot_offset.x + __rc.axes_llim.x,
            .y = m_subplot_offset.y + m_subplot_dims.y - __rc.axes_llim.y + __rc.axes_ticklength.y
        };

        m_axes_vertices[3] = 
        {
            .x = m_subplot_offset.x + __rc.axes_llim.x,
            .y = m_subplot_offset.y + __rc.axes_rlim.y
        };

    }

    //
    ~Subplot()
    {
        delete m_vertices;
    }

    //
    void draw()
    {
        // draw axes
        if (__rc.draw_axes)
        {
            DrawLineEx(m_axes_vertices[0], m_axes_vertices[1], __rc.axes_line_width, __rc.axes_color);
            DrawLineEx(m_axes_vertices[2], m_axes_vertices[3], __rc.axes_line_width, __rc.axes_color);
        }

        // draw the data
        DrawLineStrip(m_vertices, m_vertex_count, __rc.colors[m_subplot_id]);

        if (__rc.draw_title)
        {
            Vector2 sz = measure_text_large(m_title.c_str());
            draw_text_large(m_title.c_str(), { .x = m_subplot_offset.x + m_subplot_dims.x * 0.5f - sz.x * 0.5f, 
                                               .y = m_subplot_offset.y + __rc.axes_rlim.y * 0.5f - sz.y * 0.5f });
        }

        //
        if (__rc.draw_yticks)
        {
            for (size_t i = 0; i < m_yticks.size(); i += 2)
            {
                DrawLineEx(m_yticks[i+0], m_yticks[i+1], __rc.axes_line_width, __rc.axes_color);
                const char *ytlbl = m_yticklabels[i / 2].c_str();
                Vector2 sz = measure_text(ytlbl);
                draw_text(ytlbl, { .x = m_yticks[i+1].x - __rc.axes_ticklength.x - sz.x,
                                   .y = m_yticks[i+1].y - sz.y * 0.5f });
            }

            // minimum ticklabel
            char buf_min[16] = { 0 };
            sprintf(buf_min, "%.1f", m_lim.x);
            Vector2 min_sz = measure_text(buf_min);
            draw_text(buf_min, { .x = m_axes_vertices[0].x - __rc.axes_ticklength.x - min_sz.x,
                                 .y = m_axes_vertices[0].y - min_sz.y * 0.5f });

        }

        // draw the last value of y (i.e. signal)
        if (__rc.draw_current_y)
        {
            Vector2 sz = measure_text(m_last_y);
            draw_text(m_last_y, { .x = m_axes_vertices[1].x - sz.x,
                                  .y = m_axes_vertices[3].y - 15.0f - sz.y * 0.5f });
        }

    }

    //
    template<typename T>
    void copy_data_to_vertices(data_t *_data, size_t _n)
    {
        static double scale_timer0 = GetTime();
        double scale_timer1 = GetTime();
        // reset vertex limits every 2 seconds
        if (scale_timer1 - scale_timer0 >= 2.0f)
        {
            scale_timer0 = GetTime();
            m_vertex_lim = { 1e6, -1e6 };
            m_force_rescaling = true;
        }

        // copy data
        m_vertex_count = _n;
        for (size_t i = 0; i < m_vertex_count; i++)
        {
            m_vertices[i].x = (float)i;
            
            // address of the i:th element of the _data array
            data_t *data_at_idx = _data + i;
            
            // pointer tricks
            m_vertices[i].y = *(T *)((char *)(data_at_idx) + 4 + m_subplot_id * 4);
            //                             ^            ^    ^         ^
            // treat data_t as char for pointer math    |    |         |
            //              address of i:th array element    |         |
            //                    skip the first 4 bytes (char[4])     |
            // since all elements are 4 bytes (float and uint32_t), id * 4 will 
            // correspond to the id:th number in data_t

            // store range
            m_vertex_lim.x = MIN(m_vertex_lim.x, m_vertices[i].y);
            m_vertex_lim.y = MAX(m_vertex_lim.y, m_vertices[i].y);
        }

        // update current y value
        if (__rc.draw_current_y)
        {
            float val = m_vertices[m_vertex_count - 1].y;
            memset(m_last_y, 0, 16);
            sprintf(m_last_y, "%.2f", val);
        }

        // scale and translate to subplot offset 
        // -- only update range if values have changed
        scale_vertices();

    }

    void scale_vertices()
    {
        m_vertex_lim.x = MIN(m_vertex_lim.x, 0.0f);
        float vertex_range = m_vertex_lim.y - m_vertex_lim.x;

        // Only update range/limits and ticks etc if vertex range has changed 
        // OR every 5 seconds
        if (vertex_range != m_prev_vertex_range || m_force_rescaling)
        {
            m_force_rescaling = false;

            // calculate a nice y range
            m_lim = calculate_range(m_vertex_lim.x, m_vertex_lim.y, &m_ytick_spacing);
            float range = m_lim.y - m_lim.x;
            m_ytick_count = range / m_ytick_spacing;
            
            if (!isnan(range))
            {
                m_yticks.clear();
                m_yticklabels.clear();

                //
                float y = m_lim.x;
                do
                {
                    y += m_ytick_spacing;
                    Vector2 ytick0 = 
                    { 
                        .x = 0.0f,
                        .y = y
                    };
                    Vector2 ytick1 = ytick0;
                    ytick1.x -= __rc.axes_ticklength.x;

                    char buffer[16] = { 0 };
                    sprintf(buffer, "%.1f", ytick0.y);
                    m_yticklabels.push_back(std::string(buffer));

                    // scale and transform to plotting area at subplot offset
                    scale_and_translate(&ytick0);
                    scale_and_translate(&ytick1);
                    m_yticks.push_back(ytick0);
                    m_yticks.push_back(ytick1);

                } while (y < m_lim.y);
            }

        }

        m_prev_vertex_range = vertex_range;

        // scaling and translating vertices
        for (int i = 0; i < m_vertex_count; i++)
        {
            // scale to [0..1] and translate by subplot and axes offset
            scale_and_translate(&m_vertices[i]);
        }
    }

    //
    void scale_to_plotting_area(Vector2 *_v)
    {
        // scale to limits
        float y = (_v->y - m_lim.x) / (m_lim.y - m_lim.x);
        // from fraction [0..1] to pixels [0..drawable.y]
        float y_frac = (y * m_plotting_area.y);
        
        _v->y = y_frac;
        // return y_frac;
    }

    //
    void translate_to_subplot(Vector2 *_v)
    {
        Vector2 v = 
        {
            .x = _v->x + m_subplot_offset.x + __rc.axes_llim.x,
            .y = m_subplot_dims.y + m_subplot_offset.y - __rc.axes_llim.y - _v->y
        };

        *_v = v; 
        // return v;
    }

    //
    void scale_and_translate(Vector2 *_v)
    {
        scale_to_plotting_area(_v);
        translate_to_subplot(_v);
    }

    //
    Vector2 get_offset() { return m_subplot_offset; }
    void set_frame_count(size_t _fc) { m_frame_count = _fc; }

private:
    Vector2 m_subplot_dims = { 0 };
    Vector2 m_plotting_area = { 0 };
    Vector2 m_subplot_offset = { 0 };
    
    Vector2 *m_vertices = nullptr;
    char m_last_y[16] = { 0 };

    std::string m_title = "";

    Vector2 m_axes_vertices[4];

    size_t m_vertex_count = 0;
    size_t m_subplot_id = 0;
    
    Vector2 m_vertex_lim = { 1e6, -1e6 };
    bool m_force_rescaling = false;
    float m_prev_vertex_range = -1.0f;    
    Vector2 m_lim = { 0 };
    float m_ytick_spacing = 0.0f;
    size_t m_ytick_count = 0;
    std::vector<Vector2> m_yticks;
    std::vector<std::string> m_yticklabels;

    size_t m_frame_count = 0;
};

//
class Plotter
{
public:
    //
    Plotter(Vector2 _win_dims, size_t _n_params, Vector2 _subplots_shape, const std::vector<std::string> &_titles)
    {
        m_window_dims = _win_dims;
        m_param_count = _n_params;
        m_subplots_shape = _subplots_shape;

        std::vector<std::string> titles = _titles;
        if (titles.size() != _n_params)
        {
            printf("WARNING: invalid size of std::vector<std::string> &_titles, omitting.\n");
            titles.clear();
            for (size_t i = 0; i < m_param_count; i++)
                titles.push_back("");
            __rc.draw_title = false;
        }

        // make room for menu bar
        m_subplot_dims = 
        {
            .x = _win_dims.x / _subplots_shape.x,
            .y = (_win_dims.y - __rc.menu_bar_height) / _subplots_shape.y
        };

        // create subplots
        for (int j = 0; j < _subplots_shape.y; j++)
        {
            // printf("j = %d\n", j);
            for (int i = 0; i < _subplots_shape.x; i++)
            {
                // printf("i = %d\n", i);
                int idx = j * _subplots_shape.x + i;
                Vector2 subplot_offset = 
                {
                    .x = m_subplot_dims.x * i,
                    .y = m_subplot_dims.y * j
                };

                m_subplots.push_back(new Subplot(m_subplot_dims, subplot_offset, (size_t)idx, titles[idx]));
            }
        }
    }
    
    //
    ~Plotter()
    {
        for (auto &subplot : m_subplots)
            delete subplot;
    }

    //
    void copy_data_to_vertices(data_t *_data, size_t _element_count)
    {
        // assumes the data pointer are thread-safe
        for (size_t i = 0; i < m_subplots.size(); i++)
        {
            switch (data_t_types[i])
            {
            case FLOAT:  m_subplots[i]->copy_data_to_vertices<float>(_data, _element_count); break;
            case UINT32: m_subplots[i]->copy_data_to_vertices<uint32_t>(_data, _element_count); break;
            case INT32:  m_subplots[i]->copy_data_to_vertices<int32_t>(_data, _element_count); break;
            }
           
            
        }
    }

    //
    void draw()
    {
        static size_t frame_count = 0;

        // draw subplots
        for (auto &subplot : m_subplots)
        {
            subplot->set_frame_count(frame_count);
            subplot->draw();
        }
        
        // draw subplot dividers
        if (__rc.draw_subplot_div)
        {
            for (int i = 0; i < m_subplots_shape.x; i++)
            {
                DrawLineEx({ .x = m_subplot_dims.x * i, .y = 0.0f },
                           { .x = m_subplot_dims.x * i, .y = m_window_dims.y - __rc.menu_bar_height },
                           __rc.div_line_width,
                           __rc.div_color);
            }

            for (int j = 0; j < m_subplots_shape.y; j++)
            {
                DrawLineEx({ .x = 0.0f, .y = m_subplot_dims.y * j },
                           { .x = m_window_dims.x, .y = m_subplot_dims.y * j },
                           __rc.div_line_width,
                           __rc.div_color);
            }

        }

        // draw menu bar divider and basic info
        if (__rc.menu_bar_height > 0.0f)
        {
            DrawLineEx({ .x = 0.0f, .y = m_window_dims.y - __rc.menu_bar_height },
                       { .x = m_window_dims.x, .y = m_window_dims.y - __rc.menu_bar_height },
                       __rc.div_line_width,
                       __rc.div_color);
            const char *info_text = "F1: help";
            // Vector2 info_sz = measure_text(info_text);
            draw_text(info_text, { .x = 5.0f, .y = m_window_dims.y - __rc.menu_bar_height + 5.0f });

            char buffer[64] = { 0 };
            pthread_mutex_lock(&__global_esp32_signal_freq_mtx);
            sprintf(buffer, "signal: %.2f Hz", __global_esp32_signal_freq);
            pthread_mutex_unlock(&__global_esp32_signal_freq_mtx);
            Vector2 sz = measure_text(buffer);
            draw_text(buffer, { .x = m_window_dims.x - sz.x - 5.0f, .y = m_window_dims.y - __rc.menu_bar_height + 5.0f });
        }

        frame_count++;
    }

    // Accessors
    //
    Vector2 get_subplot_offset(size_t _subplot_id)  
    {
        return m_subplots[_subplot_id]->get_offset();
    }
    Vector2 get_subplot_dims()
    {
        return m_subplot_dims;
    }
    size_t get_subplot_vertex_count()
    {   
        return m_subplot_dims.x - __rc.axes_llim.x - __rc.axes_rlim.x;
    }
    Font *get_font_reg()
    {
        return &m_font_reg;
    }
    Font *get_font_title()
    {
        return &m_font_title;
    }

private:
    Vector2 m_window_dims = { 0 };
    Vector2 m_subplots_shape = { 0 };
    Vector2 m_subplot_dims = { 0 };
    size_t m_param_count = 0;
    std::vector<Subplot *> m_subplots;

    Font m_font_reg;
    Font m_font_title;

};




#endif //__PLOTTER_H
