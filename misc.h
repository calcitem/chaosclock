#ifndef MISC_H
#define MISC_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "position.h"
#include "types.h"

using namespace std;

int mod12(int x);

void vectorCout(const std::vector<int8_t> &v,
                       const std::string &v_name);

void vectorCout(const std::vector<std::vector<int8_t>> &v,
                       const std::string &v_name);

void boardCout(const int8_t (&v)[12]);

void vectorRemove(std::vector<int8_t> &v, int i);

int vectorIndexOf(const std::vector<int8_t> &v, int i);

int vectorIndexOf(const int8_t (&v)[12], int8_t i);

int vectorIndexOf(const int (&v)[12], int i);

std::vector<int8_t> vectorMerge(const std::vector<int8_t> hand,
                                       const std::vector<int8_t> free);

#endif // MISC_H
