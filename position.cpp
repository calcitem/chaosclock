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

#include <cstring>

#include "position.h"
#include "config.h"
#include "stack.h"

extern ChaosClock::Stack<Position> ss;

bool Position::reset()
{
    gamePly = 0;
    st.rule50 = 0;

    std::memset(board, NO_PIECE, sizeof(board));
    inHand.clear();
    lastMove = -2;
    step = 0;
    sideToMove = YI;
    result = GameResult::none;
    moveList.clear();
    haveWon[JIA] = haveWon[YI] = false;

    return true;
}

void Position::initBoard()
{
    // Initially there are no chess pieces in hand
    inHand.clear();

    // clear history
    moveList.clear();

    // Initialize chess pieces
    for (int i = 0; i < 12; ++i) {
        board[i] = i;
    }

    // In the initial state, no chess piece is allowed to be in "upright
    // position". if it exists, You need to re-allocate randomly until no chess
    // piece is in the "right position". The implementation here is to randomly
    // exchange the positions of the pieces until no pieces are in the upright
    // position.
    std::mt19937 rng(std::random_device {}());
    std::uniform_int_distribution<> dist(0, 12 - 1);
    while (hasFixedPiece()) {
        int i = dist(rng);
        int j = dist(rng);
        std::swap(board[i], board[j]);
    }

#ifdef TEST_MODE
    int testBoard[] = {
        6, 11, 8, 9, 5, 4, 1, 10, 7, 0, 2, 3,
    };

    // Load hard-coded board for testing
    for (int i = 0; i < 12; ++i) {
        board[i] = testBoard[i];
    }
#endif // TEST_MODE
}

void Position::changeSideToMove()
{
    if (sideToMove == YI) {
        sideToMove = JIA;
    } else {
        sideToMove = YI;
    }
}

PieceStatus Position::getStatus(int number)
{
    for (int i = 0; i < 12; ++i) {
        if (board[i] == number) {
            return PieceStatus::onBoard;
        }
    }

    return PieceStatus::inHand;
}

GameStatus Position::place(int number)
{
    // Any piece captured by any party, if the number of the piece is odd, it
    // will be handed over to Party A, If it is an even number, it will be
    // handed over to Party B. These chess pieces held in hand will be used for
    // falling pieces. Therefore, you need to judge the parity before placing a
    // piece, and you cannot use the opponent's piece to place a piece.
    if (number % 2 != sideToMove) {
        return GameStatus::errCannotPlaceOpponentsPiece;
    }

    // Drop the own piece in the hand on its upright position.
    // If there are other pieces in this space, the piece is captured.
    // If the opponent's piece is captured when the piece is placed, the side
    // will have one more chance to move, otherwise the opponent will continue
    // to move
    int location = number;
    if (remove(location, number) == false) {
        changeSideToMove();
    }

    lastMove = number;

    inHand.remove(number);

    return GameStatus::ok;
}

bool Position::isOk(int number)
{
    if (number < -1 || number > 11) {
        return false;
    }

    return true;
}

GameStatus Position::move(int location, int number)
{
    int newLocation = (location + number) % 12;

    // Pieces in the "upright" position cannot be captured.
    if (isFixed(newLocation)) {
        return GameStatus::errCannotRemoveFixedPiece;
    }

    remove(newLocation, number);
    lastMove = number;

    board[location] = -1;

    changeSideToMove();

    // Increment ply counters. In particular
    // TODO: Remove() has it. No need?
    ++gamePly;
    ++st.pliesFromNull;

    return GameStatus::ok;
}


bool Position::remove(int location, int number)
{
    bool ret = false;
    int pc = board[location];

    // If there is a piece at the target position, that is, this move is indeed
    // a capture piece, not placed on the empty space
    if (pc != -1) {
        inHand.push_back(pc);

        // If it is not your own piece that is captured
        if (pc % 2 != sideToMove) {
            ret = true;
            ++st.pliesFromNull;
            st.rule50++;
        }
    }

    board[location] = number;

    ++gamePly;

    return ret;
}

/// Position::do_move() makes a move, and saves all information necessary
/// to a StateInfo object. The move is assumed to be legal. Pseudo-legal
/// moves should be filtered out before this function is called.

GameStatus Position::do_move(int number)
{
    // Verify that the chess piece number range is legal
    if (!isOk(number)) {
        return GameStatus::errorOutOfRange;
    }

    st.rule50++;

    // Either side can voluntarily give up the move of this round, and it is the
    // opponent's turn to move.
    if (number == -1) {
        // If Party A and Party B give up playing chess one after another, both
        // sides will be judged to lose, which is also a "lose-lose".
        if (lastMove == -1) {
            result = GameResult::bothLose;
            return GameStatus::resultBothLose;
        }

        changeSideToMove();
        lastMove = -1;
        return GameStatus::ok;
    }

    // The chess piece that the opponent has just passed in the last step, the
    // own side cannot repeat it in this step.
    if (number == lastMove) {
        return GameStatus::errCannotMoveLastMovedPiece;
    }

    // Pieces in the "upright" position cannot be moved.
    if (isFixed(number)) {
        return GameStatus::errCannotMoveFixedPiece;
    }

    // Find the piece to move
    for (int i = 0; i < 12; ++i) {
        if (board[i] == number) {
            return move(i, number);
        }
    }

    return place(number);
}

/// Position::undo_move() unmakes a move. When it returns, the position should
/// be restored to exactly the same state as before the move was made.

void Position::undo_move()
{
    std::memcpy(this, ss.top(), sizeof(Position));
    ss.pop();
}

bool Position::isFixed(int number)
{
    // The situation where the chess piece numbered n is placed exactly in the
    // empty space numbered n, Call the pawn number n in the "upright position".
    return board[number] == number;
}

bool Position::isAllFixed(Color c)
{
    if (c == YI) {
        for (int i = 0; i < 12; i += 2) {
            if (!isFixed(i)) {
                return false;
            }
        }
        return true;
    } else {
        for (int i = 1; i < 12; i += 2) {
            if (!isFixed(i)) {
                return false;
            }
        }
        return true;
    }
}

bool Position::hasFixedPiece()
{
    for (int i = 0; i < 12; ++i) {
        if (isFixed(i)) {
            return true;
        }
    }
    return false;
}

bool Position::bothWin()
{
    for (int i = 0; i < 12; ++i) {
        if (board[i] != i) {
            return false;
        }
    }

    return true;
}

bool Position::bothLost()
{
    // TODO: The current judgment is BOTH_LOSE_THRESHOLD
    // If the game is not over yet, both sides will lose. Is it possible to
    // judge in advance that neither side can win?
    return st.rule50 > BOTH_LOSE_THRESHOLD;
}

GameStatus Position::checkIfGameIsOver(GameStatus status)
{
    // TODO: The performance of this method of judging the end of the chess game
    // is not good and needs to be optimized

    if (bothWin()) {
        result = GameResult::bothWin;
        return GameStatus::resultBothWin;
    }

    if (bothLost()) {
        result = GameResult::bothLose;
        return GameStatus::resultBothLose;
    }

    // TODO: This paragraph is not concise and needs to be refactored
    if (isAllFixed(sideToMove)) {
        if (haveWon[sideToMove] == true) {
            // If Party A has already won but Party B failed to win, then only
            // Party A wins, it will not be a win-win situation
            result = GameResult::win;
            return GameStatus::resultWin;
        } else {
            haveWon[sideToMove] = true;
        }
    }

    return status;
}

// Output chess game status information
void Position::showGameStatus(GameStatus status)
{
    cout << "\n" << gameStatusStr[int(status)] << endl;
}

void Position::printClock()
{
    // The chessboard is circular, looks like a clock, and has 12 spaces
    // numbered clockwise from 1 to 12.
    cout << "  " << setw(2) << setfill('0') << board[11] << " " << setw(2)
         << setfill('0') << board[0] << " " << setw(2) << setfill('0')
         << board[1] << " " << endl;
    cout << setw(2) << setfill('0') << board[10] << "        " << setw(2)
         << setfill('0') << board[2] << endl;
    cout << setw(2) << setfill('0') << board[9] << "        " << setw(2)
         << setfill('0') << board[3] << endl;
    cout << setw(2) << setfill('0') << board[8] << "        " << setw(2)
         << setfill('0') << board[4] << endl;
    cout << "  " << setw(2) << setfill('0') << board[7] << " " << setw(2)
         << setfill('0') << board[6] << " " << setw(2) << setfill('0')
         << board[5] << "  " << endl;

    cout << endl;
}

void Position::printPiecesOnBoard()
{
    //cout << "On board: ";

    for (int i = 0; i < 12; i++) {
        if (board[i] == i) {
            cout << "[" << board[i] << "] ";
        } else {
            cout << board[i] << " ";
        }
    }

    //cout << endl;
}

void Position::printPiecesInHand()
{
    //cout << "In hand: ";

    for (int i = 0; i < inHand.size(); i++) {
        auto pc = inHand[i];

        if (pc == -1) {
            cout << "{" << pc << "} ";
        } else if (pc > 0 && pc % 2 == 1) {
            cout << "(" << pc << ") ";
        } else if (pc >= 0 && pc % 2 == 0) {
            cout << "<" << pc << "> ";
        } else {
            cout << pc << "? ";
        }
    }

    cout << endl;
}

void Position::printMoveList()
{
    //cout << "\nMove list: ";

    cout << endl;

    int size = moveList.size();

    for (int i = 0; i < size; i++) {
        if (i == size - 1) {
            cout << moveList[i] << endl;
        } else {
            cout << moveList[i] << ", ";
        }
    }
}

void Position::printSideToMove()
{
    cout << "\n----------------------------------------------------------" << endl
         << endl;

    if (sideToMove == JIA) {
        cout << "Player A";
        cout << endl;
    } else {
        cout << "Player B";
        cout << endl;
    }
}

// Output the current game state
void Position::print()
{
    if (sideToMove == YI) {
        //cout << "\033[33m";
    }

    printClock();
    printPiecesOnBoard();
    cout << ": ";
    printPiecesInHand();
    printMoveList();
    printSideToMove();

    if (sideToMove == YI) {
        //cout << "\033[0m";
    }
}

bool Position::has_repeat() {
    // TODO: Performance
    for (int i = 0; i < ss.size() - 1; i++)
    {
        if (!std::memcmp(ss[i].board, this->board, sizeof(this->board))) {
            return true;
        }
    }
    return false;
}
