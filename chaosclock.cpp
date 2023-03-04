#include <algorithm> // find()
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator> // begin(), end()
#include <sstream>
#include <string>
#include <vector>

#include "config.h"

#pragma warning(disable : 4189)
#pragma warning(disable : 4267)
#pragma warning(disable : 4244)

using namespace std;

struct Pieces
{
    vector<vector<int>> stick = {{}, {}};
    vector<vector<int>> hand = {{}, {}};
    vector<int> running = {};
    vector<vector<int>> stop = {{}, {}};
    vector<vector<int>> stock = {{}, {}};
    vector<vector<int>> dead = {{}, {}};
    vector<vector<int>> free = {{}, {}};
};

struct Position
{
    int board[12];
    int last_move;
    int player;
    int value;
    Pieces pieces_data;
    int depth;
    int sub_value;
    Position **children;
    int child_count;
    int child_capacity;

    Position()
    {
        children = nullptr;
        child_count = 0;
        child_capacity = 0;
    }

    ~Position()
    {
        for (int i = 0; i < child_count; i++) {
            delete children[i];
        }
        free(children);
    }
};

void vectorCout(const std::vector<int> &v, const std::string &v_name = "ejsoon")
{
    std::ostringstream oss;
    oss << v_name << ": ";
    for (const auto &elem : v) {
        oss << elem << ", ";
    }
    std::cout << oss.str() << std::endl;
}

void vectorCout(const std::vector<std::vector<int>> &v,
                const std::string &v_name = "ejsoon")
{
    std::cout << v_name << ":" << std::endl;
    for (size_t x = 0; x < v.size(); x++) {
        std::cout << x << ": ";
        for (const auto &elem : v[x]) {
            std::cout << elem << ", ";
        }
        std::cout << std::endl;
    }
}

void boardCout(const int (&v)[12])
{
    std::ostringstream oss;
    for (const auto &elem : v) {
        oss << elem << ", ";
    }
    std::cout << oss.str() << std::endl;
}

void vectorRemove(std::vector<int> &v, int i)
{
    v.erase(std::remove(v.begin(), v.end(), i), v.end());
}

int vectorIndexOf(const std::vector<int> &v, int i)
{
    auto iter = std::find(v.begin(), v.end(), i);
    if (iter == v.end())
        return -1;
    return std::distance(v.begin(), iter);
}

int vectorIndexOf(int (&v)[12], char i)
{
    int vindex = find(v, v + 12, i) - v;
    if (vindex == 12)
        return -1;
    return vindex;
}

int vectorIndexOf(int (&v)[12], int i)
{
    size_t vindex = find(v, v + 12, i) - v;
    if (vindex == 12)
        return -1;
    return vindex;
}

vector<int> getRunPos(int (&board)[12], int c)
{
    vector<int> running;
    int c_pos = vectorIndexOf(board, c);
    int next_pos = (c_pos + c) % 12;
    while (board[next_pos] != next_pos + 1) {
        if (next_pos == c_pos || c == next_pos + 1) {
            running.emplace_back(next_pos);
            break;
        } else {
            running.emplace_back(next_pos);
        }
        next_pos = (next_pos + c) % 12;
    }
    return running;
}

std::vector<int> vectorMerge(std::vector<int> hand, std::vector<int> free)
{
    std::vector<int> make_free(std::move(hand));
    make_free.insert(make_free.end(), std::make_move_iterator(free.begin()),
                     std::make_move_iterator(free.end()));
    return make_free;
}

Pieces piecesValue(Position pos)
{
    Pieces new_pieces;
    vector<int> run_pos_sum = {};
    for (int c = 1; c <= 12; c++) {
        // stick, hand, run, stop
        if (c == pos.board[c - 1]) {
            new_pieces.stick[c % 2].emplace_back(c);
        } else if (vectorIndexOf(pos.board, c) == -1) {
            new_pieces.hand[c % 2].emplace_back(c);
        } else {
            vector<int> c_run_pos = getRunPos(pos.board, c);
            if (c_run_pos.size() == 0) {
                new_pieces.stop[c % 2].emplace_back(c);
            } else {
                new_pieces.running.emplace_back(c);
                run_pos_sum.insert(run_pos_sum.end(), c_run_pos.begin(),
                                   c_run_pos.end());
            }
        }
    }
    // free
    for (size_t p = 0; p < new_pieces.stop.size(); p++) {
        vector<int> makefree = vectorMerge(new_pieces.hand[p],
                                           new_pieces.free[p]);
        for (size_t x = 0; x < new_pieces.stop[p].size(); x++) {
            int stopx = new_pieces.stop[p][x];
            int stop_pos = vectorIndexOf(pos.board, stopx);
            if (vectorIndexOf(makefree, stop_pos + 1) > -1) {
                new_pieces.free[p].emplace_back(stopx);
                new_pieces.stop[p].erase(new_pieces.stop[p].begin() + x);
                makefree = vectorMerge(new_pieces.hand[p], new_pieces.free[p]);
                x = -1;
            }
        }
    }
    // stock
    for (size_t p = 0; p < new_pieces.stop.size(); p++) {
        vector<int> stop_delete;
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
    // dead
    for (size_t p = 0; p < new_pieces.stock.size(); p++) {
        vector<int> stock_delete;
        for (size_t x = 0; x < new_pieces.stock[p].size(); x++) {
            int c = new_pieces.stock[p][x];
            size_t c_pos = vectorIndexOf(pos.board, c);
            // if in other player
            if (c_pos % 2 == p) {
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
    return new_pieces;
}

/*
pos_start:
        1,2,0,4,0,6,7,3,9,10,12,11;1;6
        1,2,0,4,0,6,7,3,9,10,12,11;1
        1,2,0,4,0,6,7,3,9,10,12,11
 */
Position getValue(string pos_start)
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

int ifEnd(const Position &pos)
{
    const int me = pos.player;
    const int you = 1 - me;
    const Pieces pd = pos.pieces_data;

    const int my_num = pd.stick[me].size();
    const int your_num = pd.stick[you].size();
    const int my_handle = pd.hand[me].size() + pd.free[me].size();
    const int your_handle = pd.hand[you].size() + pd.free[you].size();
    const int my_dead = pd.dead[me].size();
    const int your_dead = pd.dead[you].size();

    // two win
    const bool two_win = my_num + my_handle == 6 &&
                         your_num + your_handle == 6 &&
                         (my_num - your_num <= 0 && my_num - your_num >= -1);
    // I win
    const bool i_win = (my_num == 6 && your_num < 6) ||
                       (my_num + my_handle == 6 && your_dead > 0) ||
                       (my_num + my_handle == 6 &&
                        your_num + your_handle <= 6 && my_num - your_num > 0);
    // I lose
    const bool i_lose = (my_num < 5 && your_num == 6) ||
                        (your_num + your_handle == 6 && my_dead > 0) ||
                        (my_num + my_handle <= 6 &&
                         your_num + your_handle == 6 && your_num - my_num > 1);
    // two lose
    const bool two_lose = my_dead > 0 && your_dead > 0;

    if (two_win) {
        return 3;
    } else if (i_win) {
        return 4;
    } else if (i_lose) {
        return 1;
    } else if (two_lose) {
        return 2;
    } else {
        return 0;
    }
}

int roll_sum = 0;
int max_depth = 0;
int result_sum = 0;

Position *roll(Position *pos)
{
    pos->value = ifEnd(*pos);

    // allocate initial capacity for children array
    pos->child_capacity = 16;
    pos->children = (Position **)malloc(sizeof(Position *) *
                                        pos->child_capacity);
    pos->child_count = 0;

    roll_sum++;
    max_depth = max(pos->depth, max_depth);
    if (pos->depth > 30 || roll_sum > 1.2e7) {
        return pos;
    }
    // children
    if (pos->value > 0) {
        result_sum++;
        // appendResult(pos);
    } else {
        vector<int> move = vectorMerge(pos->pieces_data.running,
                                       pos->pieces_data.hand[pos->player]);
        // remove lastmove and (12 if player == 1)
        vectorRemove(move, pos->last_move);
        if (1 == pos->player)
            vectorRemove(move, 12);

        // reserve enough capacity for children array
        if (pos->child_capacity < move.size()) {
            pos->child_capacity = move.size() * 2;
            pos->children = (Position **)realloc(
                pos->children, sizeof(Position *) * pos->child_capacity);

            // check if realloc() failed
            if (pos->children == NULL) {
                // deallocate previously allocated memory
                for (int i = 0; i < pos->child_count; i++) {
                    delete pos->children[i];
                }
                free(pos->children);

                // throw an exception or exit program
                throw std::bad_alloc();
            }
        }

        // create children nodes
        for (size_t y = 0; y < move.size(); y++) {
            Position *new_pos = new Position();
            memcpy(new_pos->board, pos->board, sizeof(pos->board));
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
                pos->children[pos->child_count++] = roll(new_pos);
            } else {
                new_pos->board[c - 1] = c;
                int onum = pos->board[c - 1];
                if (onum > 0 && onum % 2 != pos->player) {
                    new_pos->player = pos->player;
                    new_pos->pieces_data = piecesValue(*new_pos);
                    pos->children[pos->child_count++] = roll(new_pos);
                }
            }

            // update child count
            pos->child_count = move.size();

            if (pos->child_count == 0 ||
                pos->child_count == 1 &&
                    pos->pieces_data.dead[1 - pos->player].size() -
                            pos->children[0]
                                ->pieces_data.dead[1 - pos->player]
                                .size() >
                        0) {
                if (pos->last_move == 0) {
                    pos->value = 2;
                    for (int i = 0; i < pos->child_count; i++) {
                        delete pos->children[i];
                    }
                    pos->child_count = 0;
                } else {
                    Position *pass_pos = new Position();
                    memcpy(pass_pos->board, pos->board, sizeof(pos->board));
                    pass_pos->last_move = 0;
                    pass_pos->player = 1 - pos->player;
                    pass_pos->depth = pos->depth + 1;

                    // reserve enough capacity for children array
                    if (pos->child_capacity < 1) {
                        pos->child_capacity = 2;
                        pos->children = (Position **)realloc(
                            pos->children,
                            sizeof(Position *) * pos->child_capacity);

                        // check if realloc() failed
                        if (pos->children == NULL) {
                            // deallocate previously allocated memory
                            for (int i = 0; i < pos->child_count; i++) {
                                delete pos->children[i];
                            }
                            free(pos->children);

                            // throw an exception or exit program
                            throw std::bad_alloc();
                        }
                    }

                    pos->children[pos->child_count++] = roll(pass_pos);
                }
            }

            // update value
            int max_value = pos->value;
            for (int i = 0; i < pos->child_count; i++) {
                int this_value = pos->children[i]->value;
                if ((this_value == 4 || this_value == 1) &&
                    pos->children[i]->player != pos->player) {
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
}


int main()
{
    auto start_time = std::chrono::high_resolution_clock::now();
    // read position
    string pos_start;
    fstream my_file("ccpos.txt");
    getline(my_file, pos_start);
    my_file.close();
    Position pos = getValue(pos_start);
    vector<int> pos_board;
    pos.pieces_data = piecesValue(pos);
    Position *new_pos = roll(&pos);
    string pick_child;
    cout << "roll_sum:" << roll_sum << endl;
    cout << "max_depth:" << max_depth << endl;
    cout << "result_sum:" << result_sum << endl;
    cout << endl;
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    std::cout << "Stage 1 took " << duration.count() << " ms" << std::endl;

    do {
        cout << "board: ";
        boardCout(new_pos->board);
        cout << "depth:" << new_pos->depth << endl;
        cout << "player: " << new_pos->player << endl;
        cout << "value:" << new_pos->value << endl;
        cout << "available move:" << new_pos->child_count << endl;

        start_time = std::chrono::high_resolution_clock::now();

        for (int lm = 0; lm < new_pos->child_count; lm++) {
            cout << "  " << lm << ": " << new_pos->children[lm]->last_move;
            cout << " (value: " << new_pos->children[lm]->value << ") ";
            cout << endl;
        }

        end_time = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        std::cout << "Stage 2 took " << duration.count() << " ms" << std::endl;

        cin >> pick_child;
        if (pick_child != "-3") {
            Position *new_pos_child = new_pos->children[stoi(pick_child)];
            new_pos = new_pos_child;
        }
    } while (pick_child != "-3");

    // deallocate memory
    delete new_pos;

    return 0;
}
