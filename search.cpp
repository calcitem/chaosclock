#include "misc.h"
#include "piece.h"
#include "position.h"
#include "types.h"

using namespace std;

int roll_sum = 0;   // number of positions evaluated
int max_depth = 0;  // maximum depth reached in the search tree
int result_sum = 0; // number of results generated

/**
 * @brief Search function for the alpha-beta pruning algorithm
 *
 * @param pos Pointer to the current position
 * @return Pointer to the current position with updated value and children
 */
Position *search(Position *pos)
{
    pos->value = ifEnd(*pos); // check if the game is over
    for (Position *child : pos->children) {
        delete child;
    }
    pos->children.clear();
    roll_sum++;
    max_depth = max((int)pos->depth, max_depth); // update maximum depth
    if (pos->depth > 30 || roll_sum > 1.2e7) {   // limit the search depth and
                                               // number of positions evaluated
        return pos;
    }

    // generate children
    if (pos->value > 0) { // if the game is over, update the result count
        result_sum++;
    } else {
        vector<int8_t> move = vectorMerge(pos->pieces_data.running,
                                          pos->pieces_data.hand[pos->player]);
        // remove lastmove and (12 if player == 1)
        vectorRemove(move, pos->last_move);
        if (1 == pos->player)
            vectorRemove(move, 12);
        vector<Position *> children;
        for (size_t y = 0; y < move.size(); y++) {
            Position *new_pos = new Position(*pos);
            int c = move[y];
            new_pos->depth = pos->depth + 1;
            new_pos->last_move = c;
            new_pos->player = 1 - pos->player;
            if (y < move.size() - pos->pieces_data.hand[pos->player].size()) {
                int x = vectorIndexOf(new_pos->board, c);
                new_pos->board[x] = 0;
                if (c != 12) {
                    new_pos->board[(x + c) % 12] = c;
                }
                new_pos->pieces_data = piecesValue(*new_pos);
                children.emplace_back(new_pos);
            } else {
                new_pos->board[c - 1] = c;
                int onum = pos->board[c - 1];
                if (onum > 0 && (onum & 1) != pos->player) {
                    new_pos->player = pos->player;
                }
                new_pos->pieces_data = piecesValue(*new_pos);
                children.emplace_back(new_pos);
            }
        }
        // vector<Position *> _children = sortChildren(pos, children);
        for (size_t sa = 0; sa < children.size(); sa++) {
            Position *child = search(children[sa]);
            pos->children.emplace_back(child);
            if ((pos->children[sa]->value == 1 && // if there is a win position
                                                  // for the other player
                 pos->children[sa]->player != pos->player) ||
                (pos->children[sa]->value == 4 && // if there is a tie position
                 pos->children[sa]->player == pos->player)) {
                break;
            }
        }
        if (move.size() == 0 ||      // if there are no legal moves
            (children.size() == 1 && // if there is only one move
             pos->pieces_data.dead[1 - pos->player].size() -
                 children[0]->pieces_data.dead[1 - pos->player].size() >
             0)) {
        if (pos->last_move == 0) {
            pos->value = 2;
            for (Position *child : pos->children) {
                delete child;
            }
            pos->children.clear();
        } else {
            Position *pass_pos = new Position(*pos);
            pass_pos->depth = pos->depth + 1;
                pass_pos->last_move = 0;
                pass_pos->player = 1 - pos->player;
                pos->children.emplace_back(search(pass_pos));
            }
        }

        // value
        int max_value = pos->value;
        for (Position *child : pos->children) {
            int this_value = child->value;
            if ((this_value == 4 || this_value == 1) &&
                child->player != pos->player) {
                this_value = (this_value == 4 ? 1 : 4);
            }
            if (max_value < this_value) {
                max_value = this_value;
            }
        }
        pos->value = max_value;
    }

    return pos;
}
