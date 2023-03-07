#include "misc.h"
#include "piece.h"
#include "position.h"
#include "types.h"

#include <algorithm> // find(), remove()
#include <array>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

// Function to ensure a number is between 0 and 11 by wrapping it around
int mod12(int x)
{
    if (x >= 12)
        x -= 12;
    return x;
}

// Function to print a vector of integers
void vectorCout(const vector<int8_t> &v, const string &v_name = "ejsooon")
{
    ostringstream oss;
    oss << v_name << ": ";
    for (const auto &elem : v) {
        oss << (int)elem << ", ";
    }
    cout << oss.str() << endl;
}

// Function to print a vector of integers
void vectorCout(const vector<vector<int8_t>> &v, const string &v_name = "ejsoo"
                                                                        "n")
{
    cout << v_name << ":" << endl;
    for (size_t x = 0; x < v.size(); x++) {
        cout << x << ": ";
        for (const auto &elem : v[x]) {
            cout << elem << ", ";
        }
        cout << endl;
    }
}

// Function to print a fixed-size array of integers
void boardCout(const int8_t (&v)[12])
{
    ostringstream oss;
    for (const auto &elem : v) {
        oss << (int)elem << ", ";
    }
    cout << oss.str() << endl;
}

// Function to remove an element from a vector
void vectorRemove(vector<int8_t> &v, int i)
{
    v.erase(remove(v.begin(), v.end(), i), v.end());
}

// Function to find the index of an element in a vector
int vectorIndexOf(const vector<int8_t> &v, int i)
{
    auto iter = find(v.begin(), v.end(), i);
    if (iter == v.end())
        return -1;
    return distance(v.begin(), iter);
}

// Function to find the index of an element in a fixed-size array
int vectorIndexOf(const int8_t (&v)[12], int8_t i)
{
    auto iter = find(begin(v), end(v), i);
    if (iter == end(v))
        return -1;
    return distance(begin(v), iter);
}

// Function to find the index of an element in a fixed-size array
int vectorIndexOf(const int (&v)[12], int i)
{
    auto iter = find(begin(v), end(v), i);
    if (iter == end(v))
        return -1;
    return distance(begin(v), iter);
}

// Function to merge two vectors
vector<int8_t> vectorMerge(const vector<int8_t> hand, const vector<int8_t> free)
{
    vector<int8_t> make_free(hand);
    make_free.insert(make_free.end(), free.begin(), free.end());
    return make_free;
}
