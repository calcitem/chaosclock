#include "piece.h"
#include "position.h"
#include "types.h"
#include "misc.h"

using namespace std;

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
