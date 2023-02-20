// This file is part of ChaosClock.
// Copyright (C) 2023 The ChaosClock developers (see AUTHORS file)
//
// ChaosClock is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// ChaosClock is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// This is a C++ implementation of the "Chaos Clock" game.
// In this game, two players duel by moving pieces on a board with 12 positions.
// "Chaos Clock" is a two-player game, with each side referred to as "Even"
// and "Odd". The board is a circular board numbered clockwise from 1 to
// 12, with 12 random pieces initially distributed on the 12 empty positions.
// (In the program, for convenience of calculation, position 12 is represented
// by the number 0.) It is not allowed to have a piece in the "correct
// position". The order of play is that Even moves first, and then both
// players take turns moving. There are three types of moves: move a piece, drop
// a piece, or do not move. "Moving a piece" means picking up any piece on the
// board, moving it n steps clockwise, and eating the piece on the landing
// position, with the eaten piece going to Player A if it is odd, and to Player
// B if it is even. "Dropping a piece" means putting a piece in its correct
// position. If a piece is eaten, the player gets an extra turn. If both sides
// cannot drop a piece in the correct position or consecutively give up moving,
// it is a "double loss"; if one player manages to put all of their pieces in
// the correct position, it is a "win"; if the other player manages to do the
// same, it is a "double win"; if the last move is a drop and eat, the player
// who made that move "wins".

#include "config.h"

#include <iostream>     // std::cout, std::endl
#include <iomanip>      // std::setw
#include <random>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <vector>

#include "types.h"
#include "position.h"
#include "search.h"

using namespace std;

extern Depth originDepth;
extern int algorithm;
extern int init_board[12];
static int last_move;

static string player_even_str, player_odd_str, algorithm_str, board_str;
static int side_to_move, depth, player_even, player_odd, even_has_won, odd_has_won;

static bool isAi[] = {
    false, true
};

int readConfig()
{
    ifstream cfg_file("chaosclock.cfg");

    string line;
    while (getline(cfg_file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        string variable, value;
        size_t equals_pos = line.find('=');
        if (equals_pos != string::npos) {
            variable = line.substr(0, equals_pos);
            value = line.substr(equals_pos + 1);
            value.erase(0, value.find_first_not_of(" \t\n\r\f\v"));
            value.erase(value.find_last_not_of(" \t\n\r\f\v") + 1);
        }

        if (variable == "player-even") {
            player_even_str = value;
            player_even = (player_even_str == "ai") ? 1 : 2;
        } else if (variable == "player-odd") {
            player_odd_str = value;
            player_odd = (player_odd_str == "ai") ? 1 : 2;
        } else if (variable == "side-to-move") {
            side_to_move = stoi(value);
        } else if (variable == "depth") {
            depth = stoi(value);
        } else if (variable == "even-has-won") {
            even_has_won = stoi(value);
        } else if (variable == "odd-has-won") {
            odd_has_won = stoi(value);
        } else if (variable == "algorithm") {
            algorithm_str = value;
            algorithm = (algorithm_str == "minimax") ? 1 : 2;
        } else if (variable == "board") {
            board_str = value;  
            vector<string> tokens;
            size_t pos = 0;
            while ((pos = board_str.find(",")) != string::npos) {
                string token = board_str.substr(0, pos);
                tokens.push_back(token);
                board_str.erase(0, pos + 1);
            }
            tokens.push_back(board_str);

            for (int i = 0; i < tokens.size(); i++) {
                init_board[i] = stoi(tokens[i]);
            }
        } else if (variable == "last-move") {
            last_move = stoi(value);
        }
    }

    cout << "player-even: " << player_even_str << ", player_even: " << player_even << endl;
    cout << "player-odd: " << player_odd_str << ", player_odd: " << player_odd << endl;
    cout << "side-to-move: " << side_to_move << endl;
    cout << "even-has-won: " << even_has_won << endl;
    cout << "odd-to-move: " << odd_has_won << endl;
    cout << "depth: " << depth << endl;
    cout << "algorithm_str: " << algorithm_str << ", algorithm: " << algorithm << endl;
    cout << "init_board: ";
    for (int i = 0; i < 12; i++) {
        cout << init_board[i] << " ";
    }

    cout << "\n----------------------------------------------------------";
    cout << endl;

   cfg_file.close();

    return 0;
}

Move humanToGo() {
    int number;

begin:
    cout << "\033[31mHuman: ";
    cin >> number;
    cout << " \033[0m";

    // handle invalid input
    if (cin.fail()) {
        cout << "\nInput format error!" << endl;
        cin.clear();
        // Ignore the first 10000 characters of the input stream when reading,
        // until a newline character is encountered. This function is usually
        // used to clear the input buffer after reading, so that the program can
        // continue to execute if the reading fails. For example, if a string is
        // read in while an integer is read, it may cause the string to still be
        // in the cache, And subsequent read operations will still fail. in this
        // case, It is useful to use the cin.ignore function to clear data in
        // the cache.
        cin.ignore(10000, '\n');
        goto begin;
    }

    return (Move)number;
}

Move engineToGo(const Position &pos)
{
    cout << "Thinking...\n";
    Move best = start_thinking(&pos);

    cout << "\033[33m";    
    cout << "AI: " << (int)best << "\tValue: " << (int)bestvalue;
    cout << "\033[0m";

    return best;
}

// In the main function, use a loop to continuously execute the player's move
// operation until the game ends. If the player's input is invalid, give an
// error message.
#ifdef BRUTE_FORCE_ALGORITHM
int tmain()
#else
int main()
#endif
{
    Move move = MOVE_NONE;
    Position position;
    Position pos;

    cout << "  _____ _                        _____ _            _    " << endl;
    cout << " / ____| |                      / ____| |          | |   " << endl;
    cout << "| |    | |__   __ _  ___  ___  | |    | | ___   ___| | __" << endl;
    cout << "| |    | '_ \\ / _` |/ _ \\/ __| | |    | |/ _ \\ / __| |/ /"
         << endl;
    cout << "| |____| | | | (_| | (_) \\__ \\ | |____| | (_) | (__|   < "
         << endl;
    cout << " \\_____|_| |_|\\__,_|\\___/|___/  \\_____|_|\\___/ \\___|_|\\_\\"
         << "\n\n----------------------------------------------------------"
         << endl
         << endl
         << endl;

    readConfig();

    position.initBoard();

    if (player_even == 1) {
        isAi[0] = true;
    } else {
        isAi[0] = false;
    }

    if (player_odd == 1) {
        isAi[1] = true;
    } else {
        isAi[1] = false;
    }

    if (side_to_move == 1) {
        position.sideToMove = ODD;
    } else if (side_to_move == 0) {
        position.sideToMove = EVEN;
    } else {
        assert(0);
    }

    originDepth = (Depth)depth;

    position.lastMove = last_move;
    position.haveWon[EVEN] = even_has_won;
    position.haveWon[ODD] = odd_has_won;

    position.print();
    position.step = 0;

    for (;;) {
        position.step++;

        if (isAi[position.sideToMove]) {
            //std::memcpy(&pos, &position, sizeof(Position));
            pos = position;
            move = engineToGo(pos);
        } else {
            move = humanToGo();
        }

        GameStatus status = position.do_move(move);

        status = position.checkIfGameIsOver(status);

        if (status != GameStatus::ok) {
            position.showGameStatus(status);

            if (status == GameStatus::errorOutOfRange ||
                status == GameStatus::errCannotMoveLastMovedPiece ||
                status == GameStatus::errCannotPlaceOpponentsPiece ||
                status == GameStatus::errCannotMoveFixedPiece ||
                status == GameStatus::errCannotRemoveFixedPiece) {
                continue;
            }
        }

        position.moveList.push_back(move);

        if (position.result != GameResult::none) {
            break;
        }

        cout << endl
             << endl;

        position.print();
    }

    cout << "Congratulations on completing the game!" << endl;

    system("pause");
    return 0;
}
