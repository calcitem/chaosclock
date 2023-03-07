#ifndef POSITION_H
#define POSITION_H

#include <string>
#include <vector>

#include "types.h"
#include "piece.h"

using namespace std;

struct Pieces;

class Position
{
public:
    int8_t board[12];            // 12 Bytes
    int8_t last_move;            // 1 Bytes
    int8_t player;               // 1 Bytes
    int8_t depth;                // 1 Bytes
    int8_t value : 4;            // 0.5 Bytes
    int8_t sub_value : 4;        // 0.5 Bytes
    vector<Position *> children; // 24 Bytes
    Pieces pieces_data;          // 168 Bytes

    ~Position()
    {
        for (Position *child : children) {
            delete child;
        }
    }
};

vector<int8_t> getRunPos(const int8_t (&board)[12], int8_t c);

int ifEnd(const Position &pos);

Position getValue(const string &pos_start);

#endif // POSITION_H
