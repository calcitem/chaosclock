#include <algorithm> // find()
#include <fstream>
#include <iostream>
#include <iterator> // begin(), end()
#include <stack>
#include <vector>
#include <string>
#include <bitset>

using namespace std;

struct Pieces
{
    uint16_t stick = 0;
    uint16_t hand = 0;
    uint16_t free = 0;
    uint16_t running = 0;
    uint16_t stop = 0;
    uint16_t stock = 0;
    uint16_t dead = 0;
    uint8_t stick_size = 0;
    uint8_t hand_size = 0;
    uint8_t free_size = 0;
    uint8_t running_size = 0;
    uint8_t dead_size = 0;
};

struct Position
{
	uint64_t board = 0;
    vector<Position *> children; // 24 Bytes
};

const uint8_t pos24[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};

uint8_t *board_map = new uint8_t[4ll << 28];

uint32_t random_board[12][0xd];
uint32_t random_player[2];
uint32_t random_lastmove[0xd];

void setRandomBoard()
{
	for (uint32_t i = 0; i < 12; ++i) {
		random_board[i][0] = 0;
		for (uint32_t j = 1; j <= 12; ++j) {
			srand((i << 4) | j);
			random_board[i][j] = rand();
		}
	}
	random_lastmove[0] = 0;
	for (uint32_t i = 1; i <= 12; ++i) {
		srand(i << 4);
		random_lastmove[i] = rand();
	}
	srand(0x0000ffff);
	random_player[0] = rand();
	srand(0xffff0000);
	random_player[1] = rand();
}

uint32_t getBoardMapKey(uint64_t board)
{
	// make the key
	uint32_t key = random_board[0][board & 0xf]
		^ random_board[1][(board >> 4) & 0xf]
		^ random_board[2][(board >> 8) & 0xf]
		^ random_board[3][(board >> 12) & 0xf]
		^ random_board[4][(board >> 16) & 0xf]
		^ random_board[5][(board >> 20) & 0xf]
		^ random_board[6][(board >> 24) & 0xf]
		^ random_board[7][(board >> 28) & 0xf]
		^ random_board[8][(board >> 32) & 0xf]
		^ random_board[9][(board >> 36) & 0xf]
		^ random_board[10][(board >> 40) & 0xf]
		^ random_board[11][(board >> 44) & 0xf]
		^ random_player[(board >> 48) & 1]
		^ random_lastmove[(board >> 49) & 0xf];
	return key & 0xfffffff;
}

uint32_t multi_board = 0;

int8_t getBoardMap(uint64_t board)
{
	uint32_t key = getBoardMapKey(board);
	if (board_map[key] == 0) {
		return -1;
	}
	multi_board++;
	return board_map[key] >> 4;
}

void setBoardMap(uint64_t board)
{
	uint32_t key = getBoardMapKey(board);
	board_map[key] = 1 | (board >> 60 << 4);
}

void coutBoard(uint64_t board, string c_name = "ejsoon", bool display_board = true)
{
	cout << c_name << "->";
	cout <<  "value: ";
	cout << int(board >> 60);
	cout <<  " ; player: ";
	cout << int((board >> 48) & 1);
	cout <<  " ; lastmove: ";
	cout << int((board >> 49) & 0xf);
	cout <<  " ; maxdeep: ";
	cout << int((board >> 53) & 0b1111111);
	if (display_board) {
		cout << endl <<  " ; board: ";
		for (int i = 0; i < 12; ++i) {
			cout << int((board >> (i << 2)) & 0xf);
			cout << ", ";
		}
	}
	cout << endl;
}

void coutPieces(uint16_t piece, string c_name = "piece")
{
	cout << c_name << ": ";
	cout << bitset<4>(piece >> 12);
	cout <<  " ";
	cout << bitset<4>(piece >> 8);
	cout <<  " ";
	cout << bitset<4>(piece >> 4);
	cout <<  " ";
	cout << bitset<4>(piece);
	cout << endl;
}

// indexOfBoard
int8_t iob(uint64_t board, uint8_t c)
{
    for (uint8_t i = 0; i < 12; ++i) {
		if (c == (board >> (i << 2) & 0xf)) {
			return i;
		}
	}
	return -1;
}

// pieceOfBoard
uint8_t pob(uint64_t board, int8_t c_pos)
{
	return board >> (c_pos << 2) & 0xf;
}

uint16_t getRunPos(uint64_t board, uint8_t c, int8_t c_pos)
{
	uint16_t run_pos = 0, run3489pos = 0;
	bool empty_in_run_pos = true;
	// while loop next pos
	uint16_t next_pos = pos24[c_pos + c];
	uint8_t piece_of_next_pos = pob(board, next_pos);
    while (piece_of_next_pos != next_pos + 1) {
		run_pos |= (1 << next_pos);
		// run back to start position
        if (next_pos == c_pos) {
			if (empty_in_run_pos) {
				run3489pos |= 1 << c >> 1;
				// 3 and 4
				run_pos |= ((run3489pos >> 2) & 0b11) << 12;
				// 8 and 9
				run_pos |= ((run3489pos >> 7) & 0b11) << 14;
			}
            break;
        }
		// run into its right position
        if (c == next_pos + 1) {
            break;
        }
		// empty in the run position
		if (piece_of_next_pos > 0) {
			empty_in_run_pos = false;
		}
        next_pos = pos24[next_pos + c];
		piece_of_next_pos = pob(board, next_pos);
    }
	return run_pos;
}

#define CB ((i ^ player) & 1)
#define CBI ((~(i ^ player)) & 1)
#define PCB (((i | p) ^ player) & 1)
#define PCBI ((~((i | p) ^ player)) & 1)
Pieces piecesValue(uint64_t board)
{
    Pieces new_pieces;
	uint8_t player = (board >> 48) & 1; // 0 is odd, 1 is even
    uint16_t run_pos_sum = 0;
    for (uint8_t i = 0; i < 12; ++i) {
		uint8_t c = i + 1;
		int8_t c_pos = iob(board, c);
        // stick
        if (c == pob(board, i)) {
            new_pieces.stick |= (1 << i);
            new_pieces.stick_size = (new_pieces.stick_size & (0xf << (CBI << 2)))
            | (((new_pieces.stick_size >> (CB << 2)) & 0xf) + 1) << (CB << 2);
		}
        // hand
		else if (c_pos == -1) {
            new_pieces.hand |= (1 << i);
            new_pieces.hand_size = (new_pieces.hand_size & (0xf << (CBI << 2)))
            | (((new_pieces.hand_size >> (CB << 2)) & 0xf) + 1) << (CB << 2);
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
		for (int8_t i = 0; i < 12 ; i += 2) {
			if ((new_pieces.stop >> (i | p )) & 1) {
				uint8_t c = (i | p) + 1;
				int8_t c_pos = iob(board, c);
				if (!((p ^ c_pos) & 1) && (mergehandfree >> c_pos) & 1) {
					new_pieces.free |= (1 << c) >> 1;
					new_pieces.free_size = (new_pieces.free_size & (0xf << (PCBI << 2)))
					| (((new_pieces.free_size >> (PCB << 2)) & 0xf) + 1) << (PCB << 2);
					new_pieces.stop &= ~((1 << c) >> 1);
					mergehandfree = new_pieces.hand | new_pieces.free;
					i = -2;
				}
			}
		}
	}
    // stock
	for (uint8_t i = 0; i < 12 ; ++i) {
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
	for (int8_t i = 0; i < 12 ; ++i) {
		if ((new_pieces.stock >> i) & 1) {
			uint8_t c = i + 1;
			int8_t c_pos = iob(board, c);
            // if in other player, or in itself but dead
            if ((c_pos ^ i) & 1 || (new_pieces.dead >> c_pos) & 1) {
				new_pieces.dead |= (1 << c) >> 1;
				new_pieces.dead_size = (new_pieces.dead_size & (0xf << (CBI << 2)))
				| (((new_pieces.dead_size >> (CB << 2)) & 0xf) + 1) << (CB << 2);
				new_pieces.stock &= ~((1 << c) >> 1);
				i = -1;
            }
		}
	}
	// if multiple stock
	for (uint8_t i = 0; i < 12 ; ++i) {
		if ((new_pieces.stock >> i) & 1) {
			uint8_t c = i + 1;
			int8_t c_pos = iob(board, c);
			uint8_t ms = c_pos + 1;
			int8_t ms_pos = iob(board, ms);
			uint8_t ts = ms_pos + 1;
			int8_t ts_pos = iob(board, ts);
			if (
				(new_pieces.stock << 1 >> ms) & 1
				&& (
					ms_pos + 1 == c
					|| (
						(new_pieces.stock << 1 >> ts) & 1
						&& ts_pos + 1 == c
						)
					)
				) {
				new_pieces.dead |= (1 << c) >> 1;
				new_pieces.dead_size = (new_pieces.dead_size & (0xf << (CBI << 2)))
				| (((new_pieces.dead_size >> (CB << 2)) & 0xf) + 1) << (CB << 2);
			}
		}
	}
	new_pieces.stock &= ~new_pieces.dead;
	// remove 3489 empty loop
	if ((run_pos_sum >> 12) & 1) {
		new_pieces.running &= ~(1 << 2);
		new_pieces.running_size--;
	}
	if ((run_pos_sum >> 0xd) & 1) {
		new_pieces.running &= ~(1 << 3);
		new_pieces.running_size--;
	}
	if ((run_pos_sum >> 14) & 1) {
		new_pieces.running &= ~(1 << 7);
		new_pieces.running_size--;
	}
	if ((run_pos_sum >> 15) & 1) {
		new_pieces.running &= ~(1 << 8);
		new_pieces.running_size--;
	}
	return new_pieces;
}

uint8_t posValue(uint64_t board, const Pieces &pieces_value)
{
    uint8_t my_stick = pieces_value.stick_size & 0xf;
    uint8_t your_stick = pieces_value.stick_size >> 4;
    uint8_t my_handle = (pieces_value.hand_size & 0xf)
		+ (pieces_value.free_size & 0xf);
    uint8_t your_handle = (pieces_value.hand_size >> 4)
		+ (pieces_value.free_size >> 4);
    uint8_t my_dead = pieces_value.dead_size & 0xf;
    uint8_t your_dead = pieces_value.dead_size >> 4;
    // two win
    const bool two_win = my_stick + my_handle == 6 &&
                         your_stick + your_handle == 6 &&
                         (my_stick - your_stick <= 0 && my_stick - your_stick >= -1);
    // I win
    const bool i_win = (my_stick == 6 && your_stick < 6) ||
                       (my_stick + my_handle == 6 && your_dead > 0) ||
                       (my_stick + my_handle == 6 &&
                        your_stick + your_handle <= 6 && my_stick - your_stick > 0);
    // I lose
    const bool i_lose = (my_stick < 5 && your_stick == 6) ||
                        (your_stick + your_handle == 6 && my_dead > 0) ||
                        (my_stick + my_handle <= 6 &&
                         your_stick + your_handle == 6 && your_stick - my_stick > 1);
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
uint64_t max_depth = 0;

Position *roll(Position *pos, int8_t depth)
{
    roll_sum++;
	// pos value
    Pieces pieces_value = piecesValue(pos->board);
	uint64_t pos_value = posValue(pos->board, pieces_value);
	pos->board |= pos_value << 60;
	setBoardMap(pos->board);
    pos->children.clear();
	// top max depth
    max_depth = max(depth, (int8_t)max_depth);
	pos->board &= ~(0b1111111ll << 53);
	pos->board |= (uint64_t)depth << 53;
	// end if too much
    if (depth >= 48 || roll_sum >= 1.2e7) {
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
			pos->children.resize(children_size + 1);
		} else {
			pos->children.resize(children_size);
		}
		uint8_t x = 0;
		bool if_win = false;
		// hand's children
		for (uint8_t i = 0; i < 12 && !if_win; i += 2) {
			uint64_t c = (i | player) + 1;
			if ((pieces_value.hand << 1 >> c) & 1) {
				Position *new_pos = new Position();
				new_pos->board = pos->board;
				uint8_t outc = (new_pos->board << 4 >> (c << 2)) & 0xf;
				// player
				uint64_t next_player = outc > 0 ?
					outc & 1 : ~player & 1;
				new_pos->board &= ~(1ll << 48);
				new_pos->board |= next_player << 48;
				// hand on stick
				new_pos->board &= ~(0xfll << (c << 2) >> 4);
				new_pos->board |= c << (c << 2) >> 4;
				// lastmove
				new_pos->board &= ~(0xfll << 49);
				new_pos->board |= c << 49;
				// push map
				int64_t is_set = getBoardMap(new_pos->board);
				if (is_set == -1) {
					pos->children[x] = roll(new_pos, depth + 1);
				} else {
					new_pos->board |= is_set << 60;
					pos->children[x] = new_pos;
				}
				// if win
				if (pos->children[x]->board >> 60 == 4 && next_player == player
					|| pos->children[x]->board >> 60 == 1 && next_player != player) {
					if_win = true;
				}
				x++;
			}
		}
		// running's children
		for (uint8_t i = 0; i < 12 && !if_win; ++i) {
			uint64_t c = i + 1;
			if ((pieces_value.running << 1 >> c) & 1) {
				Position *new_pos = new Position();
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
				// push map
				int64_t is_set = getBoardMap(new_pos->board);
				if (is_set == -1) {
					pos->children[x] = roll(new_pos, depth + 1);
				} else {
					new_pos->board |= is_set << 60;
					pos->children[x] = new_pos;
				}
				// if win
				if (pos->children[x]->board >> 60 == 1) {
					if_win = true;
				}
				x++;
			}
		}
		if (if_win) {
			pos->children.resize(x);
			pos->board &= ~(0xfll << 60);
			pos->board |= 4ll << 60;
		} else {
			if (children_size <= 1) {
				if (0 == lastmove) {
					// two lose
					pos->board &= ~(0xfll << 60);
					pos->board |= 2ll << 60;
					return pos;
				} else {
					Position *new_pos = new Position();
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
			for (Position *child : pos->children) {
				int child_value = child->board >> 60;
				if ((child_value == 4 || child_value == 1) &&
					((child->board >> 48) & 1) != player) {
					child_value = child_value == 4 ? 1 : 4;
				}
				if (max_value < child_value) {
					max_value = child_value;
				}
			}
			pos->board &= ~(0xfll << 60);
			pos->board |= max_value << 60;
			setBoardMap(pos->board);
		}
		// max depth of this pos
		uint64_t pos_depth = (pos->board >> 53) & 0b1111111;
		for (Position *child : pos->children) {
			int child_depth = (child->board >> 53) & 0b1111111;
			if (pos_depth < child_depth) {
				pos_depth = child_depth;
			}
		}
		pos->board &= ~(0b1111111ll << 53);
		pos->board |= pos_depth << 53;
	}
	return pos;
}

// 1,2,0,4,0,6,7,3,9,10,12,11;1;6
// 1,2,0,4,0,6,7,3,9,10,12,11;1
// 1,2,0,4,0,6,7,3,9,10,12,11
Position initBoard(string pos_start)
{
    Position *new_position = new Position();
    size_t pos_find, last_pos_find, substr_len;
    vector<size_t> pos_split;
    string pos_string;
	uint64_t player, last_move;
    pos_find = 0;
    while (pos_start.find(';', pos_find) != string::npos) {
        pos_find = pos_start.find(';', pos_find);
        pos_split.emplace_back(pos_find);
        pos_find++;
    }
    if (pos_split.size() == 2) {
        pos_string = pos_start.substr(0, pos_split[0]);
        player = stoi(pos_start.substr(pos_split[0] + 1, 1));
        last_move = stoi(pos_start.substr(pos_split[1] + 1));
    } else if (pos_split.size() == 1) {
        pos_string = pos_start.substr(0, pos_split[0]);
        player = stoi(pos_start.substr(pos_split[0] + 1));
        last_move = 0;
    } else {
        pos_string = pos_start;
        player = 1;
        last_move = 0;
    }
	cout << "board: ";
    pos_find = 0;
	uint64_t c = 0;
    for (uint8_t i = 0; i < 12; i++) {
		if (pos_start.find(',', pos_find) == string::npos) {
			c = stoi(pos_start.substr(pos_find));
		} else {
			last_pos_find = pos_find;
			pos_find = pos_start.find(',', pos_find);
			substr_len = pos_find - last_pos_find;
			c = stoi(pos_start.substr(last_pos_find, substr_len));
		}
		new_position->board |= (c << (i << 2));
		cout << (int)c << ", ";
        pos_find++;
    }
	cout << endl;
	cout << "player: " << player << endl;
	new_position->board |= (player << 48);
	cout << "last_move: " << last_move << endl;
	new_position->board |= (last_move << 49);
    cout << endl;
    return *new_position;
}

int main()
{
	setRandomBoard();
    // read position
    string pos_start;
    fstream my_file("ccpos.txt");
    getline(my_file, pos_start);
    my_file.close();
    Position pos = initBoard(pos_start);
    Position *result_pos = roll(&pos, 0);
	delete board_map;
    string pick_child;
    cout << "roll_sum:" << roll_sum << endl;
    cout << "max_depth:" << (int)max_depth << endl;
    cout << "result_sum:" << result_sum << endl;
    cout << "multi_board:" << multi_board << endl;
    cout << endl;
    int this_depth = 0;
    do {
		coutBoard(result_pos->board);
        cout << "this_depth: " << this_depth++;
        cout << " ; available move:" << result_pos->children.size() << endl;
        for (size_t lm = 0; lm < result_pos->children.size(); lm++) {
            cout << "  " << lm << ": ";
			coutBoard(result_pos->children[lm]->board, "", false);
        }
        cout << endl << "select one option: ";
        cin >> pick_child;
        if (pick_child != "-3") {
            Position *new_pos_child = result_pos->children[stoi(pick_child)];
            result_pos = new_pos_child;
        }
    } while (pick_child != "-3");
    return 0;
}
