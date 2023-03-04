// This file is part of Sanmill.
// Copyright (C) 2019-2023 The Sanmill developers (see AUTHORS file)
//
// Sanmill is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Sanmill is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "movegen.h"
#include "mills.h"
#include "position.h"

/// generate<LEGAL> generates all the legal moves in the given position

template <>
ExtMove *generate<LEGAL>(Position &pos, ExtMove *moveList)
{
    ExtMove *cur = moveList;

    // TODO: performance

    Piece pc = NO_PIECE;

    for (int i = 0; i < 12; i++) {
        pc = pos.board[i];

        if (pc > -1 && pc != i && pc != pos.lastMove) {
            int newLocation = (i + pc) % 12;

            if (!pos.isFixed(newLocation)) {
                *cur++ = (Move)pc;
            }
        }
    }

#if 0
    if (pos.sideToMove == BLACK) {
        for (int i = 0; i < pos.inHand.size(); i++) {
            if (pos.inHand[i] % 2 == 1) {
                *cur++ = (Move)pos.inHand[i];
            }
        }
    } else {
        for (int i = 0; i < pos.inHand.size(); i++) {
            if (pos.inHand[i] % 2 == 0) {
                *cur++ = (Move)pos.inHand[i];
            }
        }
    }
#else
    // TODO: Performance
    if (pos.sideToMove == BLACK) {
        for (int i = SQ_BEGIN; i < SQ_END; i++) {
            if (pos.inHand[i] != NO_PIECE && pos.inHand[i] % 2 == 1) {
                *cur++ = (Move)pos.inHand[i];
            }
        }
    } else {
        for (int i = SQ_BEGIN; i < SQ_END; i++) {
            if (pos.inHand[i] != NO_PIECE && pos.inHand[i] % 2 == 0) {
                *cur++ = (Move)pos.inHand[i];
            }
        }
    }
#endif

    // Pass only when no move to do
    if (cur == moveList) {
        *cur++ = MOVE_PASS;
    }

    return cur;
}

template <>
void MoveList<LEGAL>::create()
{

}

template <>
void MoveList<LEGAL>::shuffle()
{
    Mills::move_priority_list_shuffle();
}
