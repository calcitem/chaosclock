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

#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED

#include <cassert>
#include <cstring>
#include <deque>
#include <memory> // For std::unique_ptr
#include <string>
#include <vector>

#include "rule.h"
#include "stack.h"
#include "types.h"

#ifdef NNUE_GENERATE_TRAINING_DATA
#include <ostream>
#include <sstream>
#include <string>
using std::ostream;
using std::ostringstream;
using std::string;
#endif /* NNUE_GENERATE_TRAINING_DATA */

// TODO: Move code to other place
const std::string gameStatusStr[] = {
    "",
    "The input range is incorrect. Please enter 0-11 or -1, where 0 represents "
    "12, and -1 represents giving up this turn.",
    "The opponent just moved a piece in the last step, and you cannot use it "
    "again in this step.",
    "You cannot use your opponent's pieces to make a move.",
    "Pieces that are fixed cannot be moved anymore.",
    "Pieces that are fixed cannot be removed anymore.",
    "Side to move win!",
    "Opponent of size to move win!",
    "Both players have been fixed in turn, resulting in a both win.",
    "Both players have given up their turns consecutively, or because rule 50, "
    "resulting in a both lose.",
    "Unknown",
};

/// StateInfo struct stores information needed to restore a Position object to
/// its previous state when we retract a move. Whenever a move is made on the
/// board (by calling Position::do_move), a StateInfo object must be passed.

struct StateInfo
{
    // Copied when making a move
    unsigned int rule50 {0};
    int pliesFromNull;

    // Not copied when making a move (will be recomputed anyhow)
    Key key;
};

/// Position class stores information regarding the board representation as
/// pieces, side to move, hash keys, castling info, etc. Important methods are
/// do_move() and undo_move(), used by the search to update node info when
/// traversing the search tree.
class Thread;

class Position
{
public:
    static void init();

    Position();

    Position(const Position &) = delete;
    Position &operator=(const Position &) = delete;

    // FEN string input/output
    Position &set(const std::string &fenStr, Thread *th);
    [[nodiscard]] std::string fen() const;
#ifdef NNUE_GENERATE_TRAINING_DATA
    string Position::nnueGetOpponentGameResult();
    string Position::nnueGetCurSideGameResult(char lastSide, const string &fen);
    void nnueGenerateTrainingFen();
    void nnueWriteTrainingData();
#endif /* NNUE_GENERATE_TRAINING_DATA */

    // Position representation
    [[nodiscard]] Piece piece_on(Square s) const;
    [[nodiscard]] Color color_on(Square s) const;
    [[nodiscard]] bool empty(Square s) const;
    template <PieceType Pt>
    [[nodiscard]] int count(Color c) const;

    // Properties of moves
    [[nodiscard]] bool legal(Move m) const;

    // Doing and undoing moves
    GameStatus do_move(Move m);
    void undo_move(Sanmill::Stack<Position> &ss);

    // Accessing hash keys
    [[nodiscard]] Key key() const noexcept;
    [[nodiscard]] Key key_after(Move m) const;
    void construct_key();
    Key revert_key(Square s);
    Key update_key(Square s);

    // Other properties of the position
    [[nodiscard]] Color side_to_move() const;
    [[nodiscard]] int game_ply() const;
    [[nodiscard]] Thread *this_thread() const;
    [[nodiscard]] bool has_game_cycle() const;
    bool has_repeated(Sanmill::Stack<Position> &ss) const;
    [[nodiscard]] unsigned int rule50_count() const;

    /// Mill Game

    Piece *get_board() noexcept;
    [[nodiscard]] Square current_square() const;
    [[nodiscard]] const char *get_record() const;

    bool reset();
    bool start();
    void update_score();
    GameStatus check_if_game_is_over(GameStatus status);
    void set_side_to_move(Color c);

    void change_side_to_move();
    [[nodiscard]] Color get_winner() const noexcept;
    void set_gameover(Color w, GameOverReason reason);

    void reset_bb();

    void print_board();

    [[nodiscard]] int piece_on_board_count(Color c) const;
    [[nodiscard]] int piece_in_hand_count(Color c) const;

    [[nodiscard]] int get_mobility_diff() const;
    // template <typename Mt> void updateMobility(Square from, Square to);
    int calculate_mobility_diff();

    [[nodiscard]] bool is_three_endgame() const;

    static bool bitboard_is_ok();

    // Other helpers
    GameStatus put_piece(Piece pc);
    GameStatus put_piece_init(Square s, Piece pc);

    bool remove_piece(Square s, Piece pc);

    GameStatus move_piece(Square from, Piece pc);

    // Data members
    Piece board[SQUARE_NB] {NO_PIECE};
    Bitboard byTypeBB[PIECE_TYPE_NB];
    Bitboard byColorBB[COLOR_NB];
    int pieceInHandCount[COLOR_NB] {0, 0};
    int pieceOnBoardCount[COLOR_NB] {6, 6};
    int mobilityDiff {0};
    int gamePly {0};
    Color sideToMove {WHITE};
    Thread *thisThread {nullptr};
    StateInfo st;

    /// Mill Game
    Color them {BLACK};
    Color winner;
    GameOverReason gameOverReason {GameOverReason::none};

    int score[COLOR_NB] {0};
    int score_draw {0};

    int gamesPlayedCount {0};

    static constexpr int RECORD_LEN_MAX = 64;
    char record[RECORD_LEN_MAX] {'\0'};

    Move move {MOVE_NONE};

    // Chaos Clock
    Piece inHand[SQUARE_NB] {NO_PIECE}; // TODO: Performance
    void clearInHand(void) { memset(inHand, NO_PIECE, sizeof(inHand));}
    void pushBackInHand(Piece pc)
    {
        for (int i = SQ_BEGIN; i < SQ_END; i++) {
            if (inHand[i] == NO_PIECE)
            {
                inHand[i] = pc;
                return;
            }
        }
    }
    // TODO: Performance
    void removePieceInHand(Piece pc) {
        for (int i = SQ_BEGIN; i < SQ_END; i++) {
            if (inHand[i] == pc) {
                inHand[i] = NO_PIECE;
                return;
            }
        }
    }
    Move lastMove {MOVE_NONE};
    GameOverReason result {GameOverReason::none};
    bool haveWon[COLOR_NB] {false, false};
    std::vector<Move> historyList;

    PieceStatus getStatus(Square s);
    bool isFixed(int number);
    bool hasFixedPiece();
    bool isAllFixed(Color c);
    void initBoard();
    bool bothWin();
    bool bothLost();
    void showGameStatus(GameStatus status);
    void printClock();
    void printPiecesOnBoard();
    void printPiecesInHand();
    void printMoveList();
    void printLastMove();
    void printSideToMove();
    void print();
};

extern std::ostream &operator<<(std::ostream &os, const Position &pos);

inline Color Position::side_to_move() const
{
    return sideToMove;
}

inline Piece Position::piece_on(Square s) const
{
    assert(is_ok(s));
    return board[s];
}

inline bool Position::empty(Square s) const
{
    return piece_on(s) == NO_PIECE;
}

template <PieceType Pt>
int Position::count(Color c) const
{
    if (Pt == ON_BOARD) {
        return pieceOnBoardCount[c];
    }

    if (Pt == IN_HAND) {
        return pieceInHandCount[c];
    }

    return 0;
}

inline Key Position::key() const noexcept
{
    return st.key;
}

inline void Position::construct_key()
{
    st.key = 0;
}

inline int Position::game_ply() const
{
    return gamePly;
}

inline unsigned int Position::rule50_count() const
{
    return st.rule50;
}

inline Thread *Position::this_thread() const
{
    return thisThread;
}

/// Mill Game

inline Piece *Position::get_board() noexcept
{
    return board;
}

inline const char *Position::get_record() const
{
    return record;
}

inline int Position::piece_on_board_count(Color c) const
{
    return pieceOnBoardCount[c];
}

inline int Position::piece_in_hand_count(Color c) const
{
    return pieceInHandCount[c];
}

inline int Position::get_mobility_diff() const
{
    return mobilityDiff;
}

inline bool Position::is_three_endgame() const
{
    return pieceOnBoardCount[WHITE] == 3 || pieceOnBoardCount[BLACK] == 3;
}

#endif // #ifndef POSITION_H_INCLUDED
