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

#include "evaluate.h"
#include "movepick.h"
#include "stack.h"
#include "search.h"
#include <string>

using Eval::evaluate;

Position rootPos;

Depth originDepth {12};
Move bestMove {MOVE_NONE};
Value bestvalue {VALUE_ZERO};
Value lastvalue {VALUE_ZERO};

Value minimax(Position *pos, Depth depth);

Value qsearch(Position *pos, Depth depth,
              Value alpha, Value beta);

void go(Position *pos)
{
#ifdef UCI_AUTO_RE_GO
begin:
#endif

    start_thinking(pos);

    //     if (pos->get_phase() == Phase::gameOver) {
    // #ifdef UCI_AUTO_RESTART
    //         // TODO(calcitem)
    //         while (true) {
    //             if (Threads.main()->searching == true) {
    //                 continue;
    //             }
    //
    //             pos->set(StartFEN, Threads.main());
    //             Threads.main()->us = ODD; // WAR
    //             break;
    //         }
    // #else
    //         return;
    // #endif
    //    }

#ifdef UCI_AUTO_RE_GO
    goto begin;
#endif
}

ChaosClock::Stack<Position> ss;
int algorithm;

/// start_thinking() is the main iterative deepening loop. It calls search()
/// repeatedly with increasing depth until the allocated thinking time has been
/// consumed, the user stops the search, or the maximum search depth is reached.

Move start_thinking(const Position *pos)
{
    // rootPos = pos;
    std::memcpy(&rootPos, pos, sizeof(Position));

    Value value = VALUE_ZERO;
    const Depth d = originDepth;

    if (algorithm == 1) {
        value = minimax(&rootPos, d);
    } else if (algorithm == 2) {
        Value alpha = VALUE_NONE;
        Value beta = VALUE_NONE;

        value = qsearch(&rootPos, d, alpha, beta);
    } else {
        assert(0);
    }

    lastvalue = bestvalue;
    bestvalue = value;

    ss.clear();

    return bestMove;
}

Value qsearch(Position *pos, Depth depth,
              Value alpha, Value beta)
{
    Value value;
    Value bestValue = -VALUE_INFINITE;

    if (pos->st.rule50 > BOTH_LOSE_THRESHOLD) {
        alpha = VALUE_BOTH_LOSE;
        if (alpha >= beta) {
            return alpha;
        }
    }

    // TODO: Recover || depth <= 0 
    if (pos->result != GameResult::none || depth <= 0) {
        bestValue = Eval::evaluate(*pos);

        // For win quickly
        // TODO: Apply
#if 0
        if (bestValue > 0) {
            bestValue += depth;
        } else {
            bestValue -= depth;
        }
#endif

        return bestValue;
    }

    // Initialize a MovePicker object for the current position, and prepare
    // to search the moves.
    MovePicker mp(*pos);
    const Move nextMove = mp.next_move();
    const int moveCount = mp.move_count();

    if (moveCount == 1 && depth == originDepth) {
        bestMove = nextMove;
        bestValue = VALUE_UNIQUE;
        return bestValue;
    }

    if (depth == originDepth) {
        cout << "\nMove count = " << moveCount << endl;
    }

    // Loop through the moves until no moves remain or a beta cutoff occurs
    for (int i = 0; i < moveCount; i++) {
        //ss.push(*pos);
        Position backupPosition = *pos;
        const Color before = pos->sideToMove;
        const Move move = mp.moves[i].move;

        // Make and search the move
        switch (pos->do_move(move)) {
        case GameStatus::errorOutOfRange:
        case GameStatus::errCannotMoveLastMovedPiece:
        case GameStatus::errCannotPlaceOpponentsPiece:
        case GameStatus::errCannotMoveFixedPiece:
        case GameStatus::errCannotRemoveFixedPiece:
            continue;
        default:
            break;
        }

        const Color after = pos->sideToMove;

        if (after != before) {
            value = -qsearch(pos, depth - 1, -beta, -alpha);
        } else {
            value = qsearch(pos, depth - 1, alpha, beta);
        }

        if (value == -VALUE_BOTH_WIN) {
            value = VALUE_BOTH_WIN;
        }

        if (value == -VALUE_BOTH_LOSE) {
            value = VALUE_BOTH_LOSE;
        }

        //pos->undo_move();
        *pos = backupPosition;

        if (value >= bestValue) {
            bestValue = value;

            if (value > alpha) {
                if (depth == originDepth) {
                    bestMove = move;
                }

                if (value < beta) {
                    // Update alpha! Always alpha < beta
                    alpha = value;
                } else {
                    assert(value >= beta); // Fail high
                    break;                 // Fail high
                }
            }
        }

        if (depth == originDepth) {
            cout << "Move: " << (int)move << ",  Value = " << (int)value;

            if (value == bestvalue) {
                cout << " *";
            }

            cout << endl;
        }
    }

    return bestValue;
}

Value minimax(Position *pos, Depth depth)
{
    Value value;
    Value bestValue = -VALUE_INFINITE;

    //cout << "depth = " << (int)depth <<  endl;

    if (pos->st.rule50 > BOTH_LOSE_THRESHOLD) {
        return VALUE_BOTH_LOSE;
    }

    if (pos->result != GameResult::none || depth <= 0) {
        bestValue = Eval::evaluate(*pos);
        return bestValue;
    }

    // Initialize a MovePicker object for the current position, and prepare
    // to search the moves.
    MovePicker mp(*pos);
    const Move nextMove = mp.next_move();
    const int moveCount = mp.move_count();

    if (moveCount == 1 && depth == originDepth) {
        bestMove = nextMove;
        bestValue = VALUE_UNIQUE;
        return bestValue;
    }

    // Loop through the moves and recursively evaluate the position after each
    // move.
    if (depth == originDepth) {
        cout << "\nMove count = " << moveCount << endl;
    }

    for (int i = 0; i < moveCount; i++) {
        //ss.push(*pos);
        Position backupPosition = *pos;

        const Color before = pos->sideToMove;
        const Move move = mp.moves[i].move;

        // Make and evaluate the move
        switch (pos->do_move(move)) {
        case GameStatus::errorOutOfRange:
        case GameStatus::errCannotMoveLastMovedPiece:
        case GameStatus::errCannotPlaceOpponentsPiece:
        case GameStatus::errCannotMoveFixedPiece:
        case GameStatus::errCannotRemoveFixedPiece:
            continue;
        default:
            break;
        }

        const Color after = pos->sideToMove;

        if (after != before) {
            value = -minimax(pos, depth - 1);
        } else {
            value = minimax(pos, depth - 1);
        }

        if (value == -VALUE_BOTH_WIN) {
            value = VALUE_BOTH_WIN;
        }

        if (value == -VALUE_BOTH_LOSE) {
            value = VALUE_BOTH_LOSE;
        }

        //pos->undo_move();
        *pos = backupPosition;

        if (value > bestValue) {
            bestValue = value;
            if (depth == originDepth) {
                bestMove = move;
            }
        }

        if (depth == originDepth) {
            cout << "Move: " << (int)move << ",  Value = " << (int)value;

            if (value == bestvalue) {
                cout <<  " *";
            }

            cout << endl;
        }
    }

    return bestValue;
}
