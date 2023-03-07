#include "piece.h"
#include "misc.h"
#include "position.h"
#include "types.h"
#include <algorithm> // find()
#include <array>

using namespace std;

// Function to update the free pieces of a player based on their hand and stop
// pieces
void makeFree(size_t p, Pieces &new_pieces, const Position &pos)
{
    vector<int8_t> makefree = vectorMerge(new_pieces.hand[p],
                                          new_pieces.free[p]);
    for (long x = 0; x < new_pieces.stop[p].size(); x++) {
        int stopx = new_pieces.stop[p][x];
        int8_t stop_pos = vectorIndexOf(pos.board, stopx);
        if (vectorIndexOf(makefree, stop_pos + 1) > -1) {
            new_pieces.free[p].emplace_back(stopx);
            new_pieces.stop[p].erase(new_pieces.stop[p].begin() + x);
            makefree = vectorMerge(new_pieces.hand[p], new_pieces.free[p]);
            x = -1;
        }
    }
}

// Function to update the stock pieces of a player based on their stop pieces
// and the running pieces on the board
void makeStock(size_t p, Pieces &new_pieces, const Position &pos,
               const vector<int8_t> &run_pos_sum)
{
    vector<int8_t> stop_delete;
    for (size_t x = 0; x < new_pieces.stop[p].size(); x++) {
        int c = new_pieces.stop[p][x];
        int c_pos = vectorIndexOf(pos.board, c);
        if (vectorIndexOf(run_pos_sum, c_pos) == -1) {
            stop_delete.emplace_back(x);
            new_pieces.stock[p].emplace_back(c);
        }
    }
    while (stop_delete.size()) {
        new_pieces.stop[p].erase(new_pieces.stop[p].begin() +
                                 stop_delete.back());
        stop_delete.pop_back();
    }
}

// Function to update the dead pieces of a player based on their stock pieces
// and the state of the board
void makeDead(size_t p, Pieces &new_pieces, const Position &pos)
{
    vector<int8_t> stock_delete;
    for (size_t x = 0; x < new_pieces.stock[p].size(); x++) {
        int c = new_pieces.stock[p][x];
        size_t c_pos = vectorIndexOf(pos.board, c);
        // if in other player
        if ((c_pos & 1) == p) {
            stock_delete.emplace_back(x);
            new_pieces.dead[p].emplace_back(c);
        }
        // if multiple stock
        else {
            int ms = c_pos + 1;
            int ms_pos = vectorIndexOf(pos.board, ms);
            int ts = ms_pos + 1;
            int ts_pos = vectorIndexOf(pos.board, ts);
            if (vectorIndexOf(new_pieces.stock[p], ms) > -1 &&
                (ms_pos + 1 == c ||
                 (vectorIndexOf(new_pieces.stock[p], ts) > -1 &&
                  ts_pos + 1 == c))) {
                stock_delete.emplace_back(x);
                new_pieces.dead[p].emplace_back(c);
            }
        }
    }
    while (stock_delete.size()) {
        new_pieces.stock[p].erase(new_pieces.stock[p].begin() +
                                  stock_delete.back());
        stock_delete.pop_back();
    }
}

// Function to compute the values of each type of piece for both players based
// on the current state of the board
Pieces piecesValue(const Position &pos)
{
    Pieces new_pieces;
    vector<int8_t> run_pos_sum = {};
    for (int c = 1; c <= 12; c++) {
        // stick, hand, run, stop
        if (c == pos.board[c - 1]) {
            new_pieces.stick[c & 1].emplace_back(c);
        } else if (vectorIndexOf(pos.board, c) == -1) {
            new_pieces.hand[c & 1].emplace_back(c);
        } else {
            vector<int8_t> c_run_pos = getRunPos(pos.board, c);
            if (c_run_pos.size() == 0) {
                new_pieces.stop[c & 1].emplace_back(c);
            } else {
                new_pieces.running.emplace_back(c);
                run_pos_sum.insert(run_pos_sum.end(), c_run_pos.begin(),
                                   c_run_pos.end());
            }
        }
    }

    // free
    makeFree(0, new_pieces, pos);
    makeFree(1, new_pieces, pos);

    // stock
    makeStock(0, new_pieces, pos, run_pos_sum);
    makeStock(1, new_pieces, pos, run_pos_sum);

    // dead
    makeDead(0, new_pieces, pos);
    makeDead(1, new_pieces, pos);

    return new_pieces;
}
