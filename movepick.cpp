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

#include "movepick.h"

// partial_insertion_sort() sorts moves in descending order up to and including
// a given limit. The order of moves smaller than the limit is left unspecified.
void partial_insertion_sort(ExtMove *begin, const ExtMove *end, int limit)
{
    for (ExtMove *sortedEnd = begin, *p = begin + 1; p < end; ++p)
        if (p->value >= limit) {
            ExtMove tmp = *p, *q;
            *p = *++sortedEnd;
            for (q = sortedEnd; q != begin && *(q - 1) < tmp; --q)
                *q = *(q - 1);
            *q = tmp;
        }
}

/// Constructors of the MovePicker class.

/// MovePicker constructor for the main search
MovePicker::MovePicker(Position &p) noexcept
    : pos(p)
{ }

/// MovePicker::score() assigns a numerical value to each move in a list, used
/// for sorting.
void MovePicker::score()
{
    cur = moves;

    while (cur++->move != MOVE_NONE) {
        cur->value = 0; // TODO: Implement
    }
}

/// MovePicker::next_move() is the most important method of the MovePicker
/// class. It returns a new pseudo legal move every time it is called until
/// there are no more moves left, picking the move with the highest score from a
/// list of generated moves.
Move MovePicker::next_move()
{
    endMoves = generate(pos, moves);
    moveCount = static_cast<int>(endMoves - moves);

    score();
    partial_insertion_sort(moves, endMoves, INT_MIN);

    return *moves;
}
