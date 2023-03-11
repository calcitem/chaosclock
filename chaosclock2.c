#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Define a struct called "Pieces"
struct Pieces
{
    uint16_t stick; // A bitmask of the player's sticks on the board
    uint16_t hand;  // A bitmask of the player's pieces in hand
    uint16_t free;  // A bitmask of the player's pieces that are not blocked
    // by the opponent
    uint16_t running; // A bitmask of the player's pieces that are part of a
    // run
    uint16_t stop; // A bitmask of the player's pieces that are blocked by
    // the opponent
    uint16_t stock; // A bitmask of the player's pieces that are still in
    // the stock
    uint16_t dead; // A bitmask of the player's pieces that have been
    // captured by the opponent
    uint8_t stick_size; // The number of sticks the player has on the board
    uint8_t hand_size;  // The number of pieces the player has in hand
    uint8_t free_size;  // The number of pieces the player has that are not
    // blocked by the opponent
    uint8_t running_size; // The number of pieces the player has that are
    // part of a run
    uint8_t dead_size; // The number of pieces the player has that have been
    // captured by the opponent
};

typedef struct _Position
{
    uint64_t board;
    struct _Position **children;
    uint16_t num_children;
    uint16_t size_children;
} Position;

// An array representing the movement directions of the pieces on the board
const uint8_t pos24[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                         0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

char *strndup(const char *s, size_t n)
{
    size_t len = strnlen(s, n);
    char *new_s = (char *)malloc(len + 1);
    if (new_s == NULL)
        return NULL;
    memcpy(new_s, s, len);
    new_s[len] = '\0';
    return new_s;
}

// Function to print the current game board
void coutBoard(uint64_t board, char *c_name, bool display_board)
{
    printf("%s->", c_name);
    printf("value: %d ; player: %d ; lastmove: %d ; deep: %d",
           (int)(board >> 60), (int)((board >> 48) & 1),
           (int)((board >> 49) & 0xf), (int)((board >> 54) & 0xf));
    if (display_board) {
        printf("\nboard: ");
        for (int i = 0; i < 12; ++i) {
            printf("%d, ", (int)((board >> (i << 2)) & 0xf));
        }
    }
    printf("\n");
}

// Function to print the current pieces
void coutPieces(uint16_t piece, char *c_name)
{
    printf("%s: %d %d %d %d\n", c_name, (int)(piece >> 12),
           (int)(piece >> 8 & 0xf), (int)(piece >> 4 & 0xf),
           (int)(piece & 0xf));
}

// Function to find the index of a piece on the game board
int8_t iob(uint64_t board, uint8_t c)
{
    for (uint8_t i = 0; i < 12; ++i) {
        if (c == (board >> (i << 2) & 0xf)) {
            return i;
        }
    }
    return -1;
}

// Function to get the piece at a specific position on the board
uint8_t pob(uint64_t board, int8_t c_pos)
{
    return board >> (c_pos << 2) & 0xf;
}

// Function to get the bitmask of the positions a piece can move to in a run
uint16_t getRunPos(uint64_t board, uint8_t c, int8_t c_pos)
{
    uint16_t run_pos = 0;
    uint16_t next_pos = pos24[c_pos + c];
    while (pob(board, next_pos) != next_pos + 1) {
        run_pos |= (1 << next_pos);
        if (next_pos == c_pos || c == next_pos + 1) {
            break;
        }
        next_pos = pos24[next_pos + c];
    }
    return run_pos;
}

// Define some macros for use in the piecesValue function
#define CB ((i ^ player) & 1)
#define CBI ((~(i ^ player)) & 1)
#define PCB (((i | p) ^ player) & 1)
#define PCBI ((~((i | p) ^ player)) & 1)
struct Pieces piecesValue(uint64_t board)
{
    struct Pieces new_pieces;
    uint8_t player = (board >> 48) & 1; // 0 is odd, 1 is even
    uint16_t run_pos_sum = 0;
    for (uint8_t i = 0; i < 12; ++i) {
        uint8_t c = i + 1;
        int8_t c_pos = iob(board, c);
        // stick
        if (c == pob(board, i)) {
            new_pieces.stick |= (1 << i);
            new_pieces.stick_size =
                (new_pieces.stick_size & (0xf << (CBI << 2))) |
                (((new_pieces.stick_size >> (CB << 2)) & 0xf) + 1) << (CB << 2);
        }
        // hand
        else if (c_pos == -1) {
            new_pieces.hand |= (1 << i);
            new_pieces.hand_size =
                (new_pieces.hand_size & (0xf << (CBI << 2))) |
                (((new_pieces.hand_size >> (CB << 2)) & 0xf) + 1) << (CB << 2);
        }
        // run, stop
        else {
            uint16_t c_run_pos = getRunPos(board, c, c_pos);
            if (c_run_pos == 0) {
                new_pieces.stop |= (1 << i);
            } else {
                new_pieces.running |= (1 << i);
                new_pieces.running_size++;
                run_pos_sum |= c_run_pos;
            }
        }
    }
    // free
    uint16_t mergehandfree = new_pieces.hand | new_pieces.free;
    for (uint8_t p = 0; p < 2; ++p) {
        for (int8_t i = 0; i < 12; i += 2) {
            if ((new_pieces.stop >> (i | p)) & 1) {
                uint8_t c = (i | p) + 1;
                int8_t c_pos = iob(board, c);
                if (!((p ^ c_pos) & 1) && (mergehandfree >> c_pos) & 1) {
                    new_pieces.free |= (1 << c) >> 1;
                    new_pieces.free_size =
                        (new_pieces.free_size & (0xf << (PCBI << 2))) |
                        (((new_pieces.free_size >> (PCB << 2)) & 0xf) + 1)
                            << (PCB << 2);
                    new_pieces.stop &= ~((1 << c) >> 1);
                    mergehandfree = new_pieces.hand | new_pieces.free;
                    i = -2;
                }
            }
        }
    }
    // stock
    for (uint8_t i = 0; i < 12; ++i) {
        if ((new_pieces.stop >> i) & 1) {
            uint8_t c = i + 1;
            int8_t c_pos = iob(board, c);
            if (!((run_pos_sum >> c_pos) & 1)) {
                new_pieces.stock |= (1 << c) >> 1;
                new_pieces.stop &= ~((1 << c) >> 1);
            }
        }
    }
    // dead
    for (int8_t i = 0; i < 12; ++i) {
        if ((new_pieces.stock >> i) & 1) {
            uint8_t c = i + 1;
            int8_t c_pos = iob(board, c);
            // if in other player, or in itself but dead
            if ((c_pos ^ i) & 1 || (new_pieces.dead >> c_pos) & 1) {
                new_pieces.dead |= (1 << c) >> 1;
                new_pieces.dead_size = (new_pieces.dead_size &
                                        (0xf << (CBI << 2))) |
                                       (((new_pieces.dead_size >> (CB << 2)) &
                                         0xf) +
                                        1) << (CB << 2);
                new_pieces.stock &= ~((1 << c) >> 1);
                i = -1;
            }
        }
    }
    // if multiple stock
    for (uint8_t i = 0; i < 12; ++i) {
        if ((new_pieces.stock >> i) & 1) {
            uint8_t c = i + 1;
            int8_t c_pos = iob(board, c);
            uint8_t ms = c_pos + 1;
            int8_t ms_pos = iob(board, ms);
            uint8_t ts = ms_pos + 1;
            int8_t ts_pos = iob(board, ts);
            if ((new_pieces.stock << 1 >> ms) & 1 &&
                (ms_pos + 1 == c ||
                 ((new_pieces.stock << 1 >> ts) & 1 && ts_pos + 1 == c))) {
                new_pieces.dead |= (1 << c) >> 1;
                new_pieces.dead_size = (new_pieces.dead_size &
                                        (0xf << (CBI << 2))) |
                                       (((new_pieces.dead_size >> (CB << 2)) &
                                         0xf) +
                                        1) << (CB << 2);
            }
        }
    }
    new_pieces.stock &= ~new_pieces.dead;
    return new_pieces;
}

// Function to compute the value of the current position
uint8_t posValue(uint64_t board, const struct Pieces *pieces_value)
{
    uint8_t my_stick = pieces_value->stick_size & 0xf;
    uint8_t your_stick = pieces_value->stick_size >> 4;
    uint8_t my_handle = (pieces_value->hand_size & 0xf) +
                        (pieces_value->free_size & 0xf);
    uint8_t your_handle = (pieces_value->hand_size >> 4) +
                          (pieces_value->free_size >> 4);
    uint8_t my_dead = pieces_value->dead_size & 0xf;
    uint8_t your_dead = pieces_value->dead_size >> 4;
    // two win
    const bool two_win = my_stick + my_handle == 6 &&
                         your_stick + your_handle == 6 &&
                         (my_stick - your_stick <= 0 &&
                          my_stick - your_stick >= -1);
    // I win
    const bool i_win = (my_stick == 6 && your_stick < 6) ||
                       (my_stick + my_handle == 6 && your_dead > 0) ||
                       (my_stick + my_handle == 6 &&
                        your_stick + your_handle <= 6 &&
                        my_stick - your_stick > 0);
    // I lose
    const bool i_lose = (my_stick < 5 && your_stick == 6) ||
                        (your_stick + your_handle == 6 && my_dead > 0) ||
                        (my_stick + my_handle <= 6 &&
                         your_stick + your_handle == 6 &&
                         your_stick - my_stick > 1);
    // two lose
    const bool two_lose = my_dead > 0 && your_dead > 0;
    if (two_win) {
        return 3;
    } else if (i_win) {
        return 4;
    } else if (i_lose) {
        return 1;
    } else if (two_lose) {
        return 2;
    }
    return 0;
}

unsigned int roll_sum = 0;
unsigned int result_sum = 0;
int8_t max_depth = 0;

Position *roll(Position *pos, int8_t depth)
{
    roll_sum++;
    // pos value
    struct Pieces pieces_value = piecesValue(pos->board);
    uint64_t pos_value = posValue(pos->board, &pieces_value);
    pos->board |= pos_value << 60;
    free(pos->children);
    pos->children = NULL;
    pos->num_children = 0;
    pos->size_children = 0;
    // max depth
    max_depth = MAX(depth, max_depth);
    uint64_t deep = depth;
    pos->board &= ~(0xfll << 54);
    pos->board |= deep << 54;
    // end if too much
    if (depth > 36 || roll_sum > 1.2e7) {
        return pos;
    }
    // end if has a value
    if (pos_value > 0) {
        result_sum++;
    }
    // roll if value is 0
    else {
        uint8_t player = (pos->board >> 48) & 1;
        uint8_t lastmove = (pos->board >> 49) & 0xf;
        uint8_t hand_size = pieces_value.hand_size & 0xf;
        uint8_t running_size = pieces_value.running_size;
        // remove lastmove
        if ((pieces_value.running << 1 >> lastmove) & 1) {
            running_size--;
            pieces_value.running &= ~(1 << lastmove >> 1);
        }
        // remove 12 when player == 0
        if (!player && (pieces_value.running << 1 >> 12) & 1) {
            running_size--;
            pieces_value.running &= ~(1 << 12 >> 1);
        }
        // children size
        uint8_t children_size = hand_size + running_size;

        if (children_size <= 1) {
            pos->children = (Position **)realloc(
                pos->children, sizeof(Position *) * (children_size + 1));
        } else {
            pos->children = (Position **)realloc(
                pos->children, sizeof(Position *) * children_size);
        }
        uint8_t x = 0;
        bool if_win = false;
        // hand's children
        for (uint8_t i = 0; i < 12 && !if_win; i += 2) {
            uint64_t c = (i | player) + 1;
            if ((pieces_value.hand << 1 >> c) & 1) {
                Position *new_pos = (Position *)malloc(sizeof(Position));
                memset(new_pos, 0, sizeof(Position));
                new_pos->board = pos->board;
                uint8_t outc = (new_pos->board << 4 >> (c << 2)) & 0xf;
                // player
                uint64_t next_player = outc > 0 ? outc & 1 : ~player & 1;
                new_pos->board &= ~(1ll << 48);
                new_pos->board |= next_player << 48;
                // hand on stick
                new_pos->board &= ~(0xfll << (c << 2) >> 4);
                new_pos->board |= c << (c << 2) >> 4;
                // lastmove
                new_pos->board &= ~(0xfll << 49);
                new_pos->board |= c << 49;
                // roll
                pos->children[x] = roll(new_pos, depth + 1);
                if (pos->children[x]->board >> 60 == 4 &&
                        next_player == player ||
                    pos->children[x]->board >> 60 == 1 &&
                        next_player != player) {
                    if_win = true;
                }
                x++;
            }
        }
        // running's children
        for (uint8_t i = 0; i < 12 && !if_win; ++i) {
            uint64_t c = i + 1;
            if ((pieces_value.running << 1 >> c) & 1) {
                Position *new_pos = (Position *)malloc(sizeof(Position));
                memset(new_pos, 0, sizeof(Position));
                new_pos->board = pos->board;
                // player
                uint64_t next_player = ~player & 1;
                new_pos->board &= ~(1ll << 48);
                new_pos->board |= next_player << 48;
                // run on position
                int8_t c_pos = iob(pos->board, c);
                int8_t c_newpos = pos24[c_pos + c];
                new_pos->board &= ~(0xfll << (c_pos << 2));
                if (12 != c) {
                    new_pos->board &= ~(0xfll << (c_newpos << 2));
                    new_pos->board |= c << (c_newpos << 2);
                }
                // lastmove
                new_pos->board &= ~(0xfll << 49);
                new_pos->board |= c << 49;
                // roll
                pos->children[x] = roll(new_pos, depth + 1);
                if (pos->children[x]->board >> 60 == 1) {
                    if_win = true;
                }
                x++;
            }
        }
        if (if_win) {
            pos->num_children = x;
            pos->size_children = x;
        } else if (children_size <= 1) {
            if (0 == lastmove) {
                // two lose
                pos->board &= ~(0xfll << 60);
                pos->board |= 2ll << 60;
                return pos;
            } else {
                Position *new_pos = malloc(sizeof(Position));
                new_pos->board = pos->board;
                // player
                uint64_t next_player = ~player & 1;
                new_pos->board &= ~(1ll << 48);
                new_pos->board |= next_player << 48;
                // lastmove is 0
                new_pos->board &= ~(0xfll << 49);
                // roll
                pos->children[children_size] = roll(new_pos, depth + 1);
            }
        }
        // value
        uint64_t max_value = pos->board >> 60;
        for (uint16_t i = 0; i < pos->num_children; i++) {
            Position *child = pos->children[i];
            int this_value = child->board >> 60;
            if ((this_value == 4 || this_value == 1) &&
                ((child->board >> 48) & 1) != player) {
                this_value = this_value == 4 ? 1 : 4;
            }
            if (max_value < this_value) {
                max_value = this_value;
            }
        }
        pos->board &= ~(0xfll << 60);
        pos->board |= max_value << 60;
        // children deep
        for (uint16_t i = 0; i < pos->num_children; i++) {
            Position *child = pos->children[i];
            int this_deep = (child->board >> 54) & 0xf;
            if (deep < this_deep) {
                deep = this_deep;
            }
        }
        pos->board &= ~(0xfll << 54);
        pos->board |= deep << 54;
    }
    return pos;
}

// 1,2,0,4,0,6,7,3,9,10,12,11;1;6
// 1,2,0,4,0,6,7,3,9,10,12,11;1
// 1,2,0,4,0,6,7,3,9,10,12,11
Position *initBoard(char *pos_start)
{
    Position *new_position = (Position *)malloc(sizeof(Position));
    memset(new_position, 0, sizeof(Position));
    size_t pos_find, last_pos_find, substr_len;
    size_t pos_split[2] = {0};
    size_t pos_split_index= 0;
    size_t pos_split_size = 0;
    char *pos_string;
    uint64_t player, last_move;
    pos_find = 0;
    while (strchr(pos_start + pos_find, ';') != NULL) {
        pos_find = strchr(pos_start + pos_find, ';') - pos_start;
        pos_split[pos_split_index] = pos_find;
        pos_split_index++;
        pos_split_size++;
        pos_find++;
    }
    if (pos_split_size == 2) {
        strncpy(pos_string, pos_start, pos_split[0]);
        pos_string[pos_split[0]] = '\0';

        char player_str[2];
        strncpy(player_str, pos_start + pos_split[0] + 1, 1);
        player_str[1] = '\0';
        player = atoi(player_str);

        char last_move_str[2];
        strncpy(last_move_str, pos_start + pos_split[1] + 1, 1);
        last_move_str[1] = '\0';
        last_move = atoi(last_move_str);
    } else if (pos_split_size == 1) {
        strncpy(pos_string, pos_start, pos_split[0]);
        pos_string[pos_split[0]] = '\0';

        player = atoi(pos_start + pos_split[0] + 1);
        printf("player: %d\n", player);

        last_move = -1;

        free(pos_string);
    } else {
        player = 0;
        last_move = -1;
    }
    printf("board: ");
    pos_find = 0;
    uint64_t c = 0;
    for (uint8_t i = 0; i < 12; i++) {
        if (strchr(pos_start + pos_find, ',') == NULL) {
            c = atoi(pos_start + pos_find);
        } else {
            last_pos_find = pos_find;
            pos_find = strchr(pos_start + pos_find, ',') - pos_start;
            substr_len = pos_find - last_pos_find;
            c = atoi(pos_start + last_pos_find);
        }
        new_position->board |= (c << (i << 2));
        printf("%lld", c);
        pos_find++;
    }
    printf("\n");
    printf("player: %lld\n", player);
    new_position->board |= (player << 48);
    printf("last_move: %lld\n", last_move);
    new_position->board |= ((uint64_t)last_move << 49);
    printf("\n");
    return new_position;
}

int main()
{
    auto start_time = clock();

    // read position
    FILE *fp = fopen("ccpos.txt", "r");
    char pos_start[1024];
    fgets(pos_start, 1024, fp);
    fclose(fp);

    Position *pos = initBoard(pos_start);
    Position *result_pos = roll(pos, 0);
    char pick_child[100];
    printf("roll_sum: %d\n", roll_sum);
    printf("max_depth: %d\n", max_depth);
    printf("result_sum: %d\n\n", result_sum);

    auto end_time = clock();
    auto duration = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("Stage 1 took %f seconds\n", duration);

    do {
        coutBoard(result_pos->board, "ejsoon", true);
        printf("available move: %d\n", result_pos->num_children);
        for (uint16_t lm = 0; lm < result_pos->num_children; lm++) {
            printf("  %d: ", lm);
            coutBoard(result_pos->children[lm]->board, "", false);
        }
        scanf("%s", pick_child);
        if (strcmp(pick_child, "-3") != 0) {
            Position *new_pos_child = result_pos->children[atoi(pick_child)];
            result_pos = new_pos_child;
        }

        end_time = clock();
        duration = (double)(end_time - start_time) / CLOCKS_PER_SEC;
        printf("Stage 2 took %f seconds\n", duration);
    } while (strcmp(pick_child, "-3") != 0);

    return 0;
}
