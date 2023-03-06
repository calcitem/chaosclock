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
