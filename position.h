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

#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED

#include <algorithm>
#include <iomanip> // std::setw
#include <iostream> // std::cout, std::endl
#include <random>
#include <string>
#include <vector>

#include "stack.h"
#include "types.h"

using namespace std;

const string gameStatusStr[] = {
    "",
    "The input range is incorrect. Please enter 0-11 or -1, where 0 represents 12, and -1 represents giving up this turn.",
    "The opponent just moved a piece in the last step, and you cannot use it again in this step.",
    "You cannot use your opponent's pieces to make a move.",
    "Pieces that are fixed cannot be moved anymore.",
    "Pieces that are fixed cannot be removed anymore.",
    "Player One win!",
    "Player Two win!",
    "Both players have been fixed in turn, resulting in a both win.",
    "Both players have given up their turns consecutively, or because rule 50, resulting in a both lose.",
    "Unknown",
};

/// StateInfo struct stores information needed to restore a Position object to
/// its previous state when we retract a move. Whenever a move is made on the
/// board (by calling Position::do_move), a StateInfo object must be passed.

class StateInfo {
public:
    // Copied when making a move
    unsigned int rule50 { 0 };
    int pliesFromNull;
};

// Used to store the current state of the board.
class Position
{
public:
    // 12 o'clock on the clock
    int board[12] {-1};

    // Pieces in both hands
    ChaosClock::Stack<int> inHand;

    // Last move
    int lastMove {-2};

    // Best move
    Move bestMove {MOVE_NONE};

    // How many steps did the two sides take
    int step {0};

    int gamePly {0};

    // Who should play chess now
    // (Order of moves: Party B moves first, and then both sides take turns.)
    Color sideToMove {BLACK};

    // game result
    GameResult result {GameResult::none};

    // historical moves
    ChaosClock::Stack<int> moveList;

    // game state
    StateInfo st;

    // Whether one side has already won
    bool haveWon[COLOR_NB] {false, false};

    Position()
    {
        reset();
    }

//     ~Position()
//     {
//         reset();
//     }

    Position(const Position &) = delete;

    Position &operator=(const Position &other)
    {
        memcpy(this->board, other.board, sizeof(this->board));
        this->inHand = other.inHand;
        this->lastMove = other.lastMove;
        this->step = other.step;
        this->gamePly = other.gamePly;
        this->sideToMove = other.sideToMove;
        this->result = other.result;
        this->moveList = other.moveList;
        this->st = other.st;
        memcpy(this->haveWon, other.haveWon, sizeof(haveWon));

        return *this;
    }

// exchange chess pieces
    void changeSideToMove();

    // Judge whether the chess piece numbered n is in the "upright position"
    bool isFixed(int number);

    // Determine whether there is a chess piece in the upright position
    bool hasFixedPiece();

    // Pieces are all placed in the "right position"
    bool isAllFixed(Color c);

    // In the initial state, chess pieces numbered 1 to 12 are randomly assigned
    // and placed on 12 empty positions on the board.
    void initBoard();

    PieceStatus getStatus(int number);

    // "Pull down" means that one side puts its own piece in its upright
    // position, For example, put the chess piece numbered n on the empty space
    // numbered n. If there is another piece in this space, eat it too.
    GameStatus place(int number);

    // Verify that the chess piece number is legal
    bool isOk(int number);

    // "Moving piece" means that one side picks up any piece on the board, and
    // the number of this piece is n, then walk n steps clockwise, If there are
    // other chess pieces in the space left after n steps, eat them.
    GameStatus move(int location, int number);

    // Use number's own piece to capture the opponent's piece at location, and
    // return true if the opponent's piece is indeed captured
    bool remove(int location, int number);

    // Used to execute the player's chess operations, including move, drop, and
    // give up.
    GameStatus do_move(int number);

    void undo_move();

    // The pieces of both sides are in the upright position, the game is over
    bool bothWin();

    bool bothLost();

    GameStatus checkIfGameIsOver(GameStatus status);

    // Output chess game status information
    void showGameStatus(GameStatus status);

    void printClock();

    void printPiecesOnBoard();

    void printPiecesInHand();

    void printMoveList();

    void printSideToMove();

    // Output the current game state
    void print();

    bool reset();

    bool has_repeat();
};

#endif // #ifndef POSITION_H_INCLUDED
