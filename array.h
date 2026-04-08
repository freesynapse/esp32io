
#ifndef __ARRAY_H
#define __ARRAY_H

#include <string.h>

//
template<typename T>
class array_t
{
public:

    array_t() {}

    //
    array_t(size_t _capacity)
    {
        data_ptr = new T[_capacity];
        size = 0;
        capacity = _capacity;
        printf("%s(): array memory allocated (%ld bytes).\n", __func__, sizeof(T) * capacity);
    }

    //
    ~array_t()
    {
        delete[] data_ptr;
        capacity = 0;
        size = 0;
    }

    //
    size_t push(const T &_t)
    {
        if (size < capacity)
        {
            data_ptr[size++] = _t;
        }
        else if (size == capacity)
        {
            memmove((void *)&data_ptr[0], (void *)&data_ptr[1], sizeof(T) * (size - 1));
            data_ptr[capacity - 1] = _t;
        }
        // unreachable (!?)
        else
            printf("why!?\n");
        
        return capacity;
    }
    //
    size_t capacity = 0;
    T *data_ptr = nullptr;
    size_t size = 0;

};


#endif // __ARRAY_H