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

bool Position::reset()
{
    gamePly = 0;
    st.rule50 = 0;

    inHand.clear();
    lastMove = -2;
    step = 0;
    sideToMove = YI;
    result = GameResult::none;
    moveList.clear();
    jiaHasWon = yiHasWon = false;

    return true;
}

void Position::initBoard()
{
    // 初始时手上无棋子
    inHand.clear();

    // 清空历史
    moveList.clear();

    // 初始化棋子
    for (int i = 0; i < 12; ++i) {
        board[i] = i;
    }

    // 初始状态下不允许任何棋子处于“正位”。如果存在，
    // 则需要重新随机分配直到没有棋子处于“正位”为止。
    // 此处实现为，随机交换棋子位置，直到没有棋子处于正位。
    std::mt19937 rng(std::random_device {}());
    std::uniform_int_distribution<> dist(0, 12 - 1);
    while (hasFixedPiece()) {
        int i = dist(rng);
        int j = dist(rng);
        std::swap(board[i], board[j]);
    }

#ifdef TEST_MODE
    int testBoard[] = {
        4, 5, 3, 8, 6, 9, 0, 11, 10, 1, 7, 2,
    };

    // 载入写死的用于测试的棋盘
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
    // 任何一方吃掉的棋子，如果这个棋子的编号是奇数，就交到甲方的手上，
    // 如果是偶数就交到乙方的手上。这些拿在手上的棋子将用于落子。
    // 因此，落子之前需要先判断奇偶性，不能拿对方的子来落子。
    if (number % 2 != sideToMove) {
        return GameStatus::errCannotPlaceOpponentsPiece;
    }

    // 把手中的己方棋子落在它的正位上。
    // 如果这个空位上有其它棋子，这个棋子就被吃掉了。
    // 如果落子时吃掉了对方的棋子，则己方多一次行棋的机会，否则就换对方继续行棋
    int location = number;
    if (remove(location, number) == false) {
        changeSideToMove();
    }

    lastMove = number;

    remove_first_element_with_value(inHand, number);

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

    // 处于“正位”的棋子，不能被吃掉。
    if (isFixed(newLocation)) {
        return GameStatus::errCannotRemoveFixedPiece;
    }

    remove(newLocation, number);
    lastMove = number;

    board[location] = -1;

    changeSideToMove();

    // Increment ply counters. In particular
    ++gamePly;
    ++st.pliesFromNull;

    return GameStatus::ok;
}


bool Position::remove(int location, int number)
{
    bool ret = false;
    int pc = board[location];

    // 如果目标位置有子，即此次行棋确实是吃子，而不是放在空位上
    if (pc != -1) {
        inHand.push_back(pc);

        // 如果吃掉的不是自己的棋子
        if (pc % 2 != sideToMove) {
            ret = true;
            ++st.pliesFromNull;
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
    // 验证棋子号码范围是否合法
    if (!isOk(number)) {
        return GameStatus::errorOutOfRange;
    }

    // 何一方都可以主动放弃本回合的行棋，轮到对方行棋。
    if (number == -1) {
        // 若甲乙双方接连放弃行棋，则判双方都输棋，即也是“双输”。
        if (lastMove == -1) {
            result = GameResult::bothLost;
            return GameStatus::resultBothLost;
        }

        changeSideToMove();
        lastMove = -1;
        return GameStatus::ok;
    }

    // 对方上一步刚走过的棋子，己方在这一步不能再重复拿来走。
    if (number == lastMove) {
        return GameStatus::errCannotMoveLastMovedPiece;
    }

    // 处于“正位”的棋子，不能再移动。
    if (isFixed(number)) {
        return GameStatus::errCannotMoveFixedPiece;
    }

    // 找到要移动的棋子
    for (int i = 0; i < 12; ++i) {
        if (board[i] == number) {
            return move(i, number);
        }
    }

    return place(number);
}

/// Position::undo_move() unmakes a move. When it returns, the position should
/// be restored to exactly the same state as before the move was made.

void Position::undo_move(ChaosClock::Stack<Position>& ss)
{
    std::memcpy(this, ss.top(), sizeof(Position));
    ss.pop();
}

bool Position::isFixed(int number)
{
    // 编号为 n 的棋子被正好摆放到编号为 n 的空位的这种情况，
    // 称之为编号为 n 的棋子处于“正位”。
    return board[number] == number;
}

bool Position::yiIsFixed()
{
    for (int i = 0; i < 12; i += 2) {
        if (!isFixed(i)) {
            return false;
        }
    }
    return true;
}

bool Position::jiaIsFixed()
{
    for (int i = 1; i < 12; i += 2) {
        if (!isFixed(i)) {
            return false;
        }
    }
    return true;
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
    // TODO: 当前是判断 100
    // 步还未结束棋局就算双输，是否有提前判断双方都不可能赢？
    return step > 100;
}

GameStatus Position::checkIfGameIsOver(GameStatus status)
{
    // TODO: 这段判断棋局结束的方式性能不佳，需优化

    if (bothWin()) {
        result = GameResult::bothWin;
        return GameStatus::resultBothWin;
    }

    if (bothLost()) {
        result = GameResult::bothLost;
        return GameStatus::resultBothLost;
    }

    // TODO: 这段很不简洁，需要重构
    if (jiaIsFixed()) {
        if (jiaHasWon == true) {
            // 如果甲方早就赢了但乙方没能赢，那么就只有甲方赢，不会是双赢
            result = GameResult::jiaWin;
            return GameStatus::resultJiaWin;
        } else {
            jiaHasWon = true;
        }

        if (sideToMove == JIA) {
            // 如甲赢了且接下来还是甲走棋，甲肯定单独赢了，不会是双赢
            result = GameResult::jiaWin;
            return GameStatus::resultJiaWin;
        }
    }

    if (yiIsFixed()) {
        if (yiHasWon == true) {
            // 如果乙方早就赢了但甲方没能赢，那么就只有乙方赢，不会是双赢
            result = GameResult::yiWin;
            return GameStatus::resultYiWin;
        } else {
            yiHasWon = true;
        }

        if (sideToMove == YI) {
            // 如乙赢了且接下来还是乙走棋，乙肯定单独赢了，不会是双赢
            result = GameResult::yiWin;
            return GameStatus::resultYiWin;
        }
    }

    return status;
}

// 输出棋局状态信息
void Position::showGameStatus(GameStatus status)
{
    cout << "\n" << gameStatusStr[int(status)] << endl;
}

void Position::printClock()
{
    // 棋盘是圆形的，长得像钟表的，有从顺时针方向编号 1 到 12 共 12 个空位。
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
    cout << "Pieces on board: ";

    for (int i = 0; i < 12; i++) {
        if (board[i] == i) {
            cout << "[" << board[i] << "] ";
        } else {
            cout << board[i] << " ";
        }
    }

    cout << endl;
}

void Position::printPiecesInHand()
{
    cout << "Pieces on hand: ";

    for (const auto &element : inHand) {
        if (element == -1) {
            cout << "{" << element << "} ";
        } else if (element > 0 && element % 2 == 1) {
            cout << "(" << element << ") ";
        } else if (element >= 0 && element % 2 == 0) {
            cout << "[" << element << "] ";
        } else {
            cout << element << "? ";
        }
    }

    cout << endl;
}

void Position::printMoveList()
{
    cout << "Move list: ";

    for (const auto &element : moveList) {
        cout << element << " ";
    }

    cout << endl;
}

void Position::printSideToMove()
{
    cout << "\n--------------------------------------------------" << endl
         << endl;

    if (sideToMove == JIA) {
        cout << "It's now Player One's turn to play." << endl;
    } else {
        cout << "It's now Player Two's turn to play." << endl;
    }
}

// 输出当前棋局状态
void Position::print()
{
    printClock();
    printPiecesOnBoard();
    printPiecesInHand();
    printMoveList();
    printSideToMove();
}
