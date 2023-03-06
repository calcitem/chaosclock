#include <algorithm> // find()
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator> // begin(), end()
#include <sstream>
#include <string>
#include <vector>

#include "position.h"
#include "types.h"
#include "misc.h"
#include "piece.h"
#include "evaluate.h"
#include "search.h"

using namespace std;

int main()
{
    auto start_time = std::chrono::high_resolution_clock::now();
    // read position
    string pos_start;
    fstream my_file("ccpos.txt");
    getline(my_file, pos_start);
    my_file.close();
    Position pos = getValue(pos_start);
    vector<int8_t> pos_board;
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
        cout << "depth:" << (int)new_pos->depth << endl;
        cout << "player: " << (int)new_pos->player << endl;
        cout << "value:" << (int)new_pos->value << endl;
        cout << "available move:" << new_pos->children.size() << endl;

        start_time = std::chrono::high_resolution_clock::now();

        for (size_t lm = 0; lm < new_pos->children.size(); lm++) {
            cout << "  " << lm << ": " << (int)new_pos->children[lm]->last_move;
            cout << " (value: " << (int)new_pos->children[lm]->value << ") ";
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
