#include "position.h"
#include "config.h"
#include "stack.h"



/// Position::undo_move() unmakes a move. When it returns, the position should
/// be restored to exactly the same state as before the move was made.
void Position::undo_move(Sanmill::Stack<Position>& ss)
{
    memcpy(this, ss.top(), sizeof(Position));
    ss.pop();
}