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
    "Both players have given up their turns consecutively, resulting in a both lose.",
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

// 用于存储棋盘的当前状态。
class Position {
public:
    // 时钟上 12 个点
    int board[12] { -1 };

    // 在双方手上的棋子
    ChaosClock::Stack<int> inHand;

    // 上次的着法
    int lastMove { -2 };

    // 双方走了多少步
    int step { 0 };

    int gamePly {0};

    // 当前该谁下棋
    // (行棋顺序: 先由乙方先行棋，之后双方轮流行棋。)
    Color sideToMove { YI };

    // 游戏结果
    GameResult result { GameResult::none };

    // 历史着法
    ChaosClock::Stack<int> moveList;

    // 棋局状态
    StateInfo st;

    // 是否有一方已经赢了
    bool jiaHasWon { false };
    bool yiHasWon { false };

    Position()
    {
        reset();
    }

//     ~Position()
//     {
//         reset();
//     }

    Position(const Position &) = delete;
    Position &operator=(const Position &) = delete;

    // 交换行棋方
    void changeSideToMove();

    // 判断编号为 n 的棋子是否处于 “正位”
    bool isFixed(int number);

    // 判断是否存在棋子处于正位
    bool hasFixedPiece();

    // 乙方棋子全部放到“正位”
    bool yiIsFixed();

    // 甲方棋子全部放到“正位”
    bool jiaIsFixed();

    // 初始状态下，有编号为 1 到 12 的棋子随机地分配并摆放到棋盘上的 12 个空位上。
    void initBoard();

    PieceStatus getStatus(int number);

    // “落子” 是指，一方把手中的己方棋子落在它的正位上，
    // 比如将编号为n的棋子放在编号为n的空位上。
    // 如果这个空位上有其它棋子，也要把它吃掉。
    GameStatus place(int number);

    // 验证棋子号码是否合法
    bool isOk(int number);

    // "走子" 是指，一方拿起棋盘上的任意一枚棋子，此棋子的编号是 n，则顺时针走 n 步，
    // 若走 n 步后停留的空位有其它棋子，则要把它吃掉。
    GameStatus move(int location, int number);

    // 使用 number 的己方棋子吃掉对方位于 location 的棋子, 如确实吃掉了对方子，返回 true
    bool remove(int location, int number);

    // 用于执行玩家的行棋操作，包括走子、落子、放弃。
    GameStatus do_move(int number);

    void undo_move(ChaosClock::Stack<Position> &ss);

    // 双方的棋子都在正位上，游戏结束
    bool bothWin();

    bool bothLost();

    GameStatus checkIfGameIsOver(GameStatus status);

    // 输出棋局状态信息
    void showGameStatus(GameStatus status);

    void printClock();

    void printPiecesOnBoard();

    void printPiecesInHand();

    void printMoveList();

    void printSideToMove();

    // 输出当前棋局状态
    void print();

    bool reset();
};

#endif // #ifndef POSITION_H_INCLUDED
