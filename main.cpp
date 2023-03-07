#include <algorithm> // find()
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator> // begin(), end()
#include <sstream>
#include <string>
#include <vector>

#include "misc.h"
#include "piece.h"
#include "position.h"
#include "search.h"
#include "types.h"

using namespace std;

static string pick_child;

// Read position from file
Position readPositionFromFile()
{
    string pos_start;
    fstream my_file("ccpos.txt");
    getline(my_file, pos_start);
    my_file.close();
    return getValue(pos_start);
}

// Print information about the current position
void printPositionInfo(Position *pos)
{
    cout << "board: ";
    boardCout(pos->board);
    cout << "depth:" << (int)pos->depth << endl;
    cout << "player: " << (int)pos->player << endl;
    cout << "value:" << (int)pos->value << endl;
    cout << "available move:" << pos->children.size() << endl;

    for (size_t lm = 0; lm < pos->children.size(); lm++) {
        cout << "  " << lm << ": " << (int)pos->children[lm]->last_move;
        cout << " (value: " << (int)pos->children[lm]->value << ") ";
        cout << endl;
    }
}

// Let user pick a move and update the current position
void pickMove(Position *pos)
{
    cin >> pick_child;
    if (pick_child != "-3") {
        Position *new_pos_child = pos->children[stoi(pick_child)];
        pos = new_pos_child;
    }
}

// Main function
int main()
{
    auto start_time = std::chrono::high_resolution_clock::now();

    // Read position from file
    Position pos = readPositionFromFile();

    // Search for best move
    pos.pieces_data = piecesValue(pos);
    Position *new_pos = search(&pos);

    // Print some search statistics
    cout << "roll_sum:" << roll_sum << endl;
    cout << "max_depth:" << max_depth << endl;
    cout << "result_sum:" << result_sum << endl;
    cout << endl;

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    std::cout << "Stage 1 took " << duration.count() << " ms" << std::endl;

    do {
        // Print information about current position
        printPositionInfo(new_pos);

        start_time = std::chrono::high_resolution_clock::now();

        // Let user pick a move
        pickMove(new_pos);

        end_time = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        std::cout << "Stage 2 took " << duration.count() << " ms" << std::endl;

    } while (pick_child != "-3");

    // Deallocate memory
    delete new_pos;

    return 0;
}
