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

// 这是 “混乱时钟�?游戏�?C++ 实现�?
// 在这个游戏中，两个玩家通过移动棋子在棋盘上�?12 个位置上进行对决�?
// “混乱时钟”游戏是一个二人对弈游戏，双方分别称作甲方和乙方�?
// 棋盘是顺时针编号�?�?2的圆形棋盘，初始状态下�?2颗棋子随机分配在12个空位上�?
// (程序中为了计算方便，用编�?表示编号12)
// 但不允许有棋子处于“正位”�?
// 行棋顺序为乙方先行棋，双方轮流行棋�?
// 行棋有三种类型：走子、落子和不走�?
// “走子”是指从棋盘上拿起任意一颗棋子，顺时针走n步并吃掉停留位置上的棋子�?
// 吃掉的棋子如果是奇数交到甲方手中，是偶数交到乙方手中�?
// “落子”是指把手中的棋子放在它的正位上，吃掉对方棋子则己方多一次行棋机会�?
// 当双方都不可能把棋子落到“正位”或连续放弃行棋时，为“双输”；
// 如果有一方把己方棋子全部放到“正位”，为“赢棋”；
// 如果对方接下来也达成将己方棋子放到“正位”，则为“双赢”；
// 如果某一方最后一步是落子吃棋，则是此方“赢棋”�?

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
    false /* 乙方 */, true /* 甲方 */
};

Move humanToGo() {
    int number;

begin:
    cout << "\033[31mHuman: ";
    cin >> number;
    cout << " \033[0m";

    // 处理输入非法的情�?
    if (cin.fail()) {
        cout << "\nInput format error!" << endl;
        cin.clear();
        // 在读入时忽略掉输入流中的�?10000 个字符，直到遇到换行符为止�?
        // 这个函数通常用于在读入之后清除输入缓存，以便在读入失败的情况下继续执行程序�?
        // 例如，如果在读入整数时读入了一个字符串，则可能会导致该字符串仍然在缓存中，
        // 并且后续读入操作仍然会失败。在这种情况下，
        // 使用 cin.ignore 函数来清除缓存中的数据是很有用的�?
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

// �?main 函数中，使用循环不断执行玩家的落子操作，直到游戏结束�?
// 如果玩家的输入不合法，则给出错误提示�?
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
        position.moveList.push_back(move); // TODO: Do not push back err
        status = position.checkIfGameIsOver(status);

        if (status != GameStatus::ok) {
            position.showGameStatus(status);
        }

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
