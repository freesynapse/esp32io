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

//
class Subplot
{
public:
    //
    Subplot(IVector2 _subplot_dims, IVector2 _offset, size_t _subplot_id, const std::string &_title)
    {
        m_subplot_id = _subplot_id;
        m_subplot_dims = { .x=(float)_subplot_dims.x, .y=(float)_subplot_dims.y };
        m_subplot_offset = { .x = (float)_offset.x, .y = (float)_offset.y };
        m_vertices = new Vector2[_subplot_dims.x - (int)__rc.axes_llim.x];
        
        if (__rc.draw_axes == true)
            m_plotting_area = Vector2Subtract(Vector2Subtract(m_subplot_dims, __rc.axes_llim), __rc.axes_rlim);
        else
            m_plotting_area = m_subplot_dims;

        //
        m_title = _title;

        // calculate axes vertices
        m_axes_vertices[0] = 
        { 
            .x = m_subplot_offset.x + __rc.axes_llim.x - __rc.axes_hang.x,
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
            .y = m_subplot_offset.y + m_subplot_dims.y - __rc.axes_llim.y + __rc.axes_hang.y
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
            Vector2 sz = MeasureTextEx(__rc.font24, m_title.c_str(), __rc.font24_size, __rc.font24_spacing);
            DrawTextEx(__rc.font24,
                       m_title.c_str(),
                        {
                            .x = m_subplot_offset.x + m_subplot_dims.x * 0.5f - sz.x * 0.5f,
                            .y = m_subplot_offset.y + __rc.axes_rlim.y * 0.5f - sz.y * 0.5f
                        },
                       __rc.font24.baseSize,
                       __rc.font24_spacing,
                       __rc.font24_color);
        }

        if (__rc.draw_y_minmax)
        {
            char buf_min[16] = { 0 };
            char buf_max[16] = { 0 };
            sprintf(buf_min, "%.1f", m_lim.x);
            sprintf(buf_max, "%.1f", m_lim.y);
            Vector2 min_sz = MeasureTextEx(__rc.font18, buf_min, __rc.font18_size, __rc.font18_spacing);
            Vector2 max_sz = MeasureTextEx(__rc.font18, buf_min, __rc.font18_size, __rc.font18_spacing);
            DrawTextEx(__rc.font18, 
                       buf_min, 
                       {
                           .x = m_axes_vertices[0].x - __rc.axes_hang.x - min_sz.x,
                           .y = m_axes_vertices[0].y - min_sz.y * 0.5f
                       },
                       (float)__rc.font18.baseSize, 
                       __rc.font18_spacing, 
                       __rc.font18_color);

            DrawTextEx(__rc.font18, 
                       buf_max, 
                       {
                           .x = m_axes_vertices[0].x - __rc.axes_hang.x - min_sz.x,
                           .y = m_axes_vertices[3].y - min_sz.y * 0.5f
                       },
                       (float)__rc.font18.baseSize, 
                       __rc.font18_spacing, 
                       __rc.font18_color);
        }

        if (__rc.draw_current_y)
        {
            Vector2 sz = MeasureTextEx(__rc.font18, m_last_y, __rc.font18_size, __rc.font18_spacing);
            DrawTextEx(__rc.font18, 
                       m_last_y,
                       {
                           .x = m_axes_vertices[1].x - sz.x,
                           .y = m_axes_vertices[3].y - 15.0f - sz.y * 0.5f
                       },
                       (float)__rc.font18.baseSize,
                       __rc.font18_spacing,
                       __rc.font18_color);
        }

    }

    //
    template<typename T>
    void copy_data_to_vertices(data_t *_data, size_t _n)
    {
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
        scale_vertices();

    }

    void scale_vertices()
    {
        // calculate a nice y range
        float range = m_vertex_lim.y - m_vertex_lim.x;
        // float min20 = MIN(m_vertex_lim.x - 0.20f * range, 0);
        // float max20 = m_vertex_lim.y + 0.20f * range;
        // m_lim = calculate_range(min20, max20);
        m_vertex_lim.x = MIN(m_vertex_lim.x, 0.0f);
        m_lim = calculate_range(m_vertex_lim.x, m_vertex_lim.y);
        
        // scaling
        for (int i = 0; i < m_vertex_count; i++)
        {
            float y = m_vertices[i].y;
            // scale to limits
            y = (y - m_lim.x) / (m_lim.y - m_lim.x);
            // from fraction [0..1] to pixels [0..drawable.y]
            m_vertices[i].y = (y * m_plotting_area.y);
            // translate by subplot and axes offset
            m_vertices[i] = 
            {
                .x = m_vertices[i].x + m_subplot_offset.x + __rc.axes_llim.x,
                .y = m_subplot_dims.y + m_subplot_offset.y - __rc.axes_llim.y - m_vertices[i].y
            };
        }
    }

    //
    Vector2 get_offset() { return m_subplot_offset; }
    void set_current_iteration(size_t _it) { m_current_iteration = _it; }

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
    Vector2 m_lim = { 0 };

    size_t m_current_iteration = 0;
};

//
class Plotter
{
public:
    //
    Plotter(IVector2 _win_dims, size_t _n_params, IVector2 _subplots_shape, const std::vector<std::string> &_titles)
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

        m_subplot_dims = 
        {
            .x = _win_dims.x / _subplots_shape.x,
            .y = _win_dims.y / _subplots_shape.y
        };

        // create subplots
        for (int j = 0; j < _subplots_shape.y; j++)
        {
            // printf("j = %d\n", j);
            for (int i = 0; i < _subplots_shape.x; i++)
            {
                // printf("i = %d\n", i);
                int idx = j * _subplots_shape.x + i;
                IVector2 subplot_offset = 
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
            case types::FLOAT:  m_subplots[i]->copy_data_to_vertices<float>(_data, _element_count); break;
            case types::UINT32: m_subplots[i]->copy_data_to_vertices<int>(_data, _element_count); break;
            }
           
            
        }
    }

    //
    void draw()
    {
        static size_t it = 0;

        // draw subplots
        for (auto &subplot : m_subplots)
        {
            subplot->set_current_iteration(it);
            subplot->draw();
        }
        
        // draw subplot dividers
        if (__rc.draw_subplot_div)
        {
            for (int i = 0; i < m_subplots_shape.x; i++)
            {
                DrawLineEx({ .x = (float)m_subplot_dims.x * i, .y = 0.0f },
                            { .x = (float)m_subplot_dims.x * i, .y = (float)m_window_dims.y },
                            __rc.div_line_width,
                            __rc.div_color);
            }

            for (int j = 0; j < m_subplots_shape.y; j++)
            {
                DrawLineEx({ .x = 0.0f, .y = (float)m_subplot_dims.y * j },
                            { .x = (float)m_window_dims.x, .y = (float)m_subplot_dims.y * j },
                            __rc.div_line_width,
                            __rc.div_color);
            }

        }

        it++;
    }

    // Accessors
    //
    Vector2 get_subplot_offset(size_t _subplot_id)  
    {
        return m_subplots[_subplot_id]->get_offset();
    }
    IVector2 get_subplot_dims()
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
    IVector2 m_window_dims = { 0 };
    IVector2 m_subplots_shape = { 0 };
    IVector2 m_subplot_dims = { 0 };
    size_t m_param_count = 0;
    std::vector<Subplot *> m_subplots;

    Font m_font_reg;
    Font m_font_title;

};




#endif //__PLOTTER_H
