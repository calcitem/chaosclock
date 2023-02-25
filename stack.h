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

template <typename T, size_t capacity = 16384>
class Stack
{
public:
    Stack() { arr = new T[capacity]; }

    Stack(const Stack &other) { *this = other; }

    ~Stack() {
        if (arr != nullptr) {
            delete[] arr;
        } 
        
        arr = nullptr;
    }

    Stack &operator=(const Stack &other)
    {
        memcpy(arr, other.arr, sizeof(T) * other.size());
        p = other.p;
        return *this;
    }

    bool operator==(const T &other) const
    {
        return p == other.p && memcmp(arr, other.arr, size());
    }

    T &operator[](int i) { return arr[i]; }

    const T &operator[](int i) const { return arr[i]; }

    void push(const T &obj)
    {
        p++;
        std::memcpy(arr + p, &obj, sizeof(T));
    }

    void push_back(const T &obj)
    {
        p++;
        arr[p] = obj;
    }

    void pop() { p--; }

    T *top() { return &arr[p]; }

    [[nodiscard]] int size() const { return p + 1; }

    [[nodiscard]] size_t length() const { return sizeof(T) * size(); }

    T *begin() { return &arr[0]; }

    T *end() { return &arr[p + 1]; }

    [[nodiscard]] bool empty() const { return p < 0; }

    void clear() { p = -1; }

    void erase(int index)
    {
        // TODO: Performance
        for (int i = index; i < capacity - 1; i++) {
            arr[i] = arr[i + 1];
        }

        p--;
    }

    void remove(T entry)
    {
        for (int i = 0; i <= p; i++) {
            if (arr[i] == entry) {
                erase(i);
                return;
            }
        }
    }

    int indexOf(T entry) {
        for (int i = 0; i <= p; i++) {
            if (!std::memcmp(arr[i], entry, sizeof(T))) {
                return i;
            }
        }
        return -1;
    }

private:
    T *arr {nullptr};
    int p {-1};
};

} // namespace ChaosClock

#endif // STACK_H_INCLUDED
