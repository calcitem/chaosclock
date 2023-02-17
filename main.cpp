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
// "Chaos Clock" is a two-player game, with each side referred to as "Player A"
// and "Player B". The board is a circular board numbered clockwise from 1 to
// 12, with 12 random pieces initially distributed on the 12 empty positions.
// (In the program, for convenience of calculation, position 12 is represented
// by the number 0.) It is not allowed to have a piece in the "correct
// position". The order of play is that Player B moves first, and then both
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

#include "types.h"
#include "position.h"
#include "search.h"

using namespace std;

const bool isAi[] = {
    false /* YI */, true /* JIA */
};

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
    start_thinking(&pos);

    cout << "\033[33m";    
    cout << "AI: " << (int)bestMove;
    cout << "\033[0m";

    return bestMove;
}

// In the main function, use a loop to continuously execute the player's move
// operation until the game ends. If the player's input is invalid, give an
// error message.
int main()
{
    Move move = MOVE_NONE;
    Position position;
    position.initBoard();
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
