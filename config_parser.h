#ifndef __CONFIG_PARSER_H
#define __CONFIG_PARSER_H


#include <vector>
#include <stdio.h>
#include <string.h>
#include <string>

#include <raylib.h>

#include "types.h"

//
#define KVS_ARG_MAX_BYTES 256
struct kv_t
{
    char k[KVS_ARG_MAX_BYTES];
    char v[KVS_ARG_MAX_BYTES];
    kv_t()
    {
        memset(k, 0, KVS_ARG_MAX_BYTES);
        memset(v, 0, KVS_ARG_MAX_BYTES);
    }
};

// trims ' ', '\r' etc
void trim_whitespaces(char *_str, size_t *_len)
{
    char tmp[KVS_ARG_MAX_BYTES] = { 0 };
    size_t i = 0;
    size_t j = 0;
    size_t len = *_len;
    while (i < len)
    {
        if (_str[i] == '\n')
            break;
        
        if (_str[i] != ' ' && _str[i] != '\t')
            tmp[j++] = _str[i];

        i++;
    }
    memset(_str, 0, len);
    memcpy(_str, tmp, j);
    *_len = strlen(_str);
}

//
class ConfigParser
{
public:
    ConfigParser(const char *_file_path)
    {
        FILE *fp;
        if ((fp = fopen(_file_path, "r")) == NULL)
            fprintf(stderr, "Could not open file '%s'.", _file_path);
        
        size_t lineno = 0;
        char *line = NULL;
        size_t len = 0;
        ssize_t read_bytes = 0;

        while ((read_bytes = getline(&line, &len, fp)) != -1)
        {
            trim_whitespaces(line, &len);
            if (len > 0 && line[0] != '#')
                extract_kvs(line);
        }

        translate_kvs();
    }

    //
    void extract_kvs(char *_line)
    {
        kv_t kv;
        size_t len = strlen(_line);
        size_t i = 0;
        size_t j = 0;
        char *key_or_value = kv.k;
        while (i < len)
        {
            if (_line[i] != '=')
                key_or_value[j++] = _line[i];
            // '=' found
            else
            {
                key_or_value = kv.v;
                j = 0;
            }
            i++;
        }
        kvs.push_back(kv);
    }

    void translate_kvs()
    {
        // assign the different parameters
        for (auto &kv : kvs)
        {
            if (strcmp(kv.k, "SUBPLOTS") == 0)
                sscanf(kv.v, "%ld", &n_params);
            else if (strcmp(kv.k, "SHAPE") == 0)
                sscanf(kv.v, "%f,%f", &subplots_shape.x, &subplots_shape.y);
            else if (strcmp(kv.k, "TITLES") == 0)
            {
                char buffer[KVS_ARG_MAX_BYTES];
                memset(buffer, '\0', KVS_ARG_MAX_BYTES);
                size_t len = strlen(kv.v);
                size_t i = 0;
                size_t j = 0;
                while (i < len)
                {
                    if (kv.v[i] != ',')
                        buffer[j++] = kv.v[i];
                    else
                    {
                        titles.push_back(std::string(buffer));
                        memset(buffer, '\0', KVS_ARG_MAX_BYTES);
                        j = 0;
                    }
                    i++;
                }
                // push last title, after last ','
                if (j > 0)
                    titles.push_back(std::string(buffer));
            }
            else
                LOG_WARNING("Unknown keyword '%s'.\n", kv.k);
        }

    }

    // variables
    std::vector<kv_t> kvs;
    size_t n_params = 0;
    Vector2 subplots_shape = { 0 };
    std::vector<std::string> titles = {};
};

#endif // __CONFIG_PARSER_H
