#ifndef PIECE_H
#define PIECE_H

#include <string>
#include <vector>

#include "types.h"

class Position;

using namespace std;

struct Pieces
{
    vector<int8_t> stick[2] = {};
    vector<int8_t> hand[2] = {};
    vector<int8_t> running = {};
    vector<int8_t> stop[2] = {};
    vector<int8_t> stock[2] = {};
    vector<int8_t> dead[2] = {};
    vector<int8_t> free[2] = {};
};

void makeFree(size_t p, Pieces &new_pieces, const Position &pos);

void makeStock(size_t p, Pieces &new_pieces, const Position &pos,
               const std::vector<int8_t> &run_pos_sum);

void makeDead(size_t p, Pieces &new_pieces, const Position &pos);

Pieces piecesValue(const Position &pos);

#endif // PIECE_H
