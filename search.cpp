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

Depth originDepth {SEARCH_DEPTH};
Move bestMove {MOVE_NONE};
Value bestvalue {VALUE_ZERO};
Value lastvalue {VALUE_ZERO};

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
    //             Threads.main()->us = WHITE; // WAR
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

/// start_thinking() is the main iterative deepening loop. It calls search()
/// repeatedly with increasing depth until the allocated thinking time has been
/// consumed, the user stops the search, or the maximum search depth is reached.

int start_thinking(const Position *pos)
{


    //rootPos = pos;
    std::memcpy(&rootPos, pos, sizeof(Position));

    Value value = VALUE_ZERO;
    const Depth d = SEARCH_DEPTH;

    Value alpha = VALUE_NONE;
    Value beta = VALUE_NONE;

    value = qsearch(&rootPos, d, alpha, beta);

    lastvalue = bestvalue;
    bestvalue = value;

    ss.clear();

    return 0;
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

    if (pos->result != GameResult::none /* || depth <= 0 */) {
        bestValue = Eval::evaluate(*pos);

        // For win quickly
        if (bestValue > 0) {
            bestValue += depth;
        } else {
            bestValue -= depth;
        }

        return bestValue;
    }

    // Initialize a MovePicker object for the current position, and prepare
    // to search the moves.
    MovePicker mp(*pos);
    const Move nextMove = mp.next_move();
    const int moveCount = mp.move_count();

    if (moveCount == 1 && depth == SEARCH_DEPTH) {
        bestMove = nextMove;
        bestValue = VALUE_UNIQUE;
        return bestValue;
    }

    // Loop through the moves until no moves remain or a beta cutoff occurs
    for (int i = 0; i < moveCount; i++) {
        ss.push(*pos);
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

        pos->undo_move();

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
    }

    return bestValue;
}
