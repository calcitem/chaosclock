#include "types.h"
#include "position.h"
#include "misc.h"
#include "piece.h"

using namespace std;

vector<int8_t> getRunPos(const int8_t (&board)[12], int8_t c)
{
    vector<int8_t> running;
    int c_pos = vectorIndexOf(board, c);
    int next_pos = mod12(c_pos + c);
    while (board[next_pos] != next_pos + 1) {
        if (next_pos == c_pos || c == next_pos + 1) {
            running.emplace_back(next_pos);
            break;
        } else {
            running.emplace_back(next_pos);
        }
        next_pos = mod12(next_pos + c);
    }
    return running;
}

int ifEnd(const Position &pos)
{
    const int me = pos.player;
    const int you = 1 - me;
    const Pieces &pd = pos.pieces_data;

    const int my_num = pd.stick[me].size();
    const int your_num = pd.stick[you].size();
    const int my_handle = pd.hand[me].size() + pd.free[me].size();
    const int your_handle = pd.hand[you].size() + pd.free[you].size();
    const int my_dead = pd.dead[me].size();
    const int your_dead = pd.dead[you].size();

    // two win
    const bool both_win = my_num + my_handle == 6 &&
                          your_num + your_handle == 6 &&
                          (my_num - your_num <= 0 && my_num - your_num >= -1);
    // I win
    const bool win = (my_num == 6 && your_num < 6) ||
                     (my_num + my_handle == 6 && your_dead > 0) ||
                     (my_num + my_handle == 6 && your_num + your_handle <= 6 &&
                      my_num - your_num > 0);
    // I lose
    const bool lose = (my_num < 5 && your_num == 6) ||
                      (your_num + your_handle == 6 && my_dead > 0) ||
                      (my_num + my_handle <= 6 && your_num + your_handle == 6 &&
                       your_num - my_num > 1);
    // two lose
    const bool both_lose = my_dead > 0 && your_dead > 0;

    if (both_win) {
        return BOTH_WIN;
    } else if (win) {
        return WIN;
    } else if (lose) {
        return LOSE;
    } else if (both_lose) {
        return BOTH_LOSE;
    } else {
        return 0;
    }
}

/*
pos_start:
        1,2,0,4,0,6,7,3,9,10,12,11;1;6
        1,2,0,4,0,6,7,3,9,10,12,11;1
        1,2,0,4,0,6,7,3,9,10,12,11
 */
Position getValue(const string &pos_start)
{
    Position new_position;
    size_t pos_find, last_pos_find, substr_len;
    vector<size_t> pos_split;
    string pos_string;
    pos_find = 0;
    while (pos_start.find(';', pos_find) != string::npos) {
        pos_find = pos_start.find(';', pos_find);
        pos_split.emplace_back(pos_find);
        pos_find++;
    }
    if (pos_split.size() == 2) {
        pos_string = pos_start.substr(0, pos_split[0]);
        new_position.player = stoi(pos_start.substr(pos_split[0] + 1, 1));
        new_position.last_move = stoi(pos_start.substr(pos_split[1] + 1));
    } else if (pos_split.size() == 1) {
        pos_string = pos_start.substr(0, pos_split[0]);
        new_position.player = stoi(pos_start.substr(pos_split[0] + 1));
        new_position.last_move = -1;
    } else {
        pos_string = pos_start;
        new_position.player = 0;
        new_position.last_move = -1;
    }
    pos_find = 0;
    int pos_p = 0;
    while (pos_start.find(',', pos_find) != string::npos) {
        last_pos_find = pos_find;
        pos_find = pos_start.find(',', pos_find);
        substr_len = pos_find - last_pos_find;
        new_position.board[pos_p] = stoi(
            pos_start.substr(last_pos_find, substr_len));
        pos_find++;
        pos_p++;
    }
    new_position
        .board[sizeof(new_position.board) / sizeof(new_position.board[0]) - 1] =
        stoi(pos_start.substr(pos_find));
    new_position.depth = 0;
    return new_position;
}
