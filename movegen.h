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

#ifndef MOVEGEN_H_INCLUDED
#define MOVEGEN_H_INCLUDED

#include <algorithm>
#include <array>

#include "types.h"

class Position;

struct ExtMove
{
    Move move;
    int value;

    operator Move() const noexcept { return move; }

    void operator=(Move m) noexcept { move = m; }

    // Inhibit unwanted implicit conversions to Move
    // with an ambiguity that yields to a compile error.
    operator float() const = delete;
};

inline bool operator<(const ExtMove &f, const ExtMove &s) noexcept
{
    return f.value < s.value;
}

ExtMove *generate(Position &pos, ExtMove *moveList);

/// The MoveList struct is a simple wrapper around generate(). It sometimes
/// comes in handy to use this class instead of the low level generate()
/// function.
struct MoveList
{
    explicit MoveList(Position &pos)
        : last(generate(pos, moveList))
    { }

    [[nodiscard]] const ExtMove *begin() const { return moveList; }

    [[nodiscard]] const ExtMove *end() const { return last; }

    [[nodiscard]] size_t size() const { return last - moveList; }

    [[nodiscard]] bool contains(Move move) const
    {
        return std::find(begin(), end(), move) != end();
    }

    static void create();
    static void shuffle();

private:
    ExtMove moveList[MAX_MOVES] {{MOVE_NONE, 0}};
    ExtMove *last {nullptr};
};

#endif // #ifndef MOVEGEN_H_INCLUDED
