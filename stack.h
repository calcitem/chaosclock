// This file is part of ChaosClock.
// Copyright (C) 2023 The ChaosClock developers (see AUTHORS file)
//
// ChaosClock is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// ChaosClock is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef STACK_H_INCLUDED
#define STACK_H_INCLUDED

#include <cstring>

namespace ChaosClock {

template <typename T, size_t capacity = 512>
class Stack
{
public:
    Stack() { arr = new T[capacity]; }

    Stack(const Stack &other) { *this = other; }

    ~Stack() {
        //delete[] arr;
        check();
    }

    Stack &operator=(const Stack &other)
    {
        memcpy(arr, other.arr, length());
        p = other.p;
        check();
        return *this;
    }

    bool operator==(const T &other) const
    {
        check();
        return p == other.p && memcmp(arr, other.arr, size());
    }

    T &operator[](int i)
    {
        check();
        return arr[i];
    }

    const T &operator[](int i) const
    {
        check();

        return arr[i];
    }

    void push(const T &obj)
    {
        p++;
        std::memcpy(arr + p, &obj, sizeof(T));
        check();
    }

    void push_back(const T &obj)
    {
        p++;
        arr[p] = obj;
        check();
    }

    void pop() { p--;
        check();
    }

    T *top()
    {
        check();
        return &arr[p];
    }

    [[nodiscard]] int size() const
    {
        check();
        return p + 1;
    }

    [[nodiscard]] size_t length() const
    {
        check();
        return sizeof(T) * size();
    }

    T *begin()
    {
        check();
        return &arr[0];
    }

    T *end()
    {
        check();
        return &arr[p + 1];
    }

    [[nodiscard]] bool empty() const
    {
        check();
        return p < 0;
    }

    void clear()
    {
        check();
        p = -1;
    }

    void erase(int index)
    {
        for (int i = index; i < capacity - 1; i++) {
            arr[i] = arr[i + 1];
        }

        p--;
        check();
    }

    void remove(T entry)
    {
        for (int i = 0; i < size(); i++) {
            if (arr[i] == entry) {
                erase(i);
                return;
            }
        }
        check();
    }

    bool check() const {
        int test = 0;
        for (int i = 0; i < p+1; i++) {
            memcpy(&test, (const void*)(arr+i), 4);
            if (test < -512) {
                return false;
            }
        }
        return true;
    }

private:
    T *arr;
    int p {-1};
};

} // namespace ChaosClock

#endif // STACK_H_INCLUDED
