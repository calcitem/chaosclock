#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm> // find()
#include <iterator> // begin(), end()

#include "config.h"

using namespace std;

struct pieces {
	vector<vector<int>> stick = {{}, {}};
	vector<vector<int>> hand = {{}, {}};
	vector<int> running = {};
	vector<vector<int>> stop = {{}, {}};
	vector<vector<int>> stock = {{}, {}};
	vector<vector<int>> dead = {{}, {}};
	vector<vector<int>> free = {{}, {}};
};

struct position {
	int board[12];
	int lastmove;
	int player;
	int value;
	pieces pieces_data;
	int deep;
	int sub_value;
	vector<position> children;
};

void vectorCout(vector<int> &v, string v_name = "ejsoon") {
	cout << v_name << ": ";
	for (int x = 0; x < v.size(); x++) {
		cout << v[x] << ", ";
	}
	cout << endl;
}

void vectorCout(vector<vector<int>> &v, string v_name = "ejsoon") {
	cout << v_name << ":" << endl;
	for (int x = 0; x < v.size(); x++) {
		cout << x << ": ";
		for (int y = 0; y < v[x].size(); y++) {
			cout << v[x][y] << ", ";
		}
		cout << endl;
	}
}

void boardCout(int (&v)[12]) {
	for (int x = 0; x < 12; x++) {
		cout << v[x] << ", ";
	}
	cout << endl;
}

void vectorRemove(vector<int> &v, int i) {
	int vindex = find(v.begin(), v.end(), i) - v.begin();
	if (vindex != v.size())
		v.erase(v.begin() + vindex);
}

int vectorIndexOf(vector<int> v, int i) {
	int vindex = find(v.begin(), v.end(), i) - v.begin();
	if (vindex == v.size()) return -1;
	return vindex;
}

int vectorIndexOf(int (&v)[12], char i) {
	int vindex = find(v, v + 12, i) - v;
	if (vindex == 12) return -1;
	return vindex;
}

int vectorIndexOf(int (&v)[12], int i) {
	int vindex = find(v, v + 12, i) - v;
	if (vindex == 12) return -1;
	return vindex;
}

vector<int> getRunPos(int (&board)[12], int c) {
	vector<int> running;
	int c_pos = vectorIndexOf(board, c);
	int next_pos = (c_pos + c) % 12;
	while (board[next_pos] != next_pos + 1) {
		if (next_pos == c_pos || c == next_pos + 1) {
			running.push_back(next_pos);
			break;
		} else {
			running.push_back(next_pos);
		}
		next_pos = (next_pos + c) % 12;
	}
	return running;
}

vector<int> vectorMerge(vector<int> hand, vector<int> free) {
	vector<int> makefree = {};
	makefree.insert(makefree.end(), hand.begin(), hand.end());
	makefree.insert(makefree.end(), free.begin(), free.end());
	return makefree;
}

void appendResult(position pos) {
	int standardInt = pos.board[0] * 1e12 + pos.board[1] * 1e11
		+ pos.board[2] * 1e10 + pos.board[3] * 1e9
		+ pos.board[4] * 1e8 + pos.board[5] * 1e7
		+ pos.board[6] * 1e6 + pos.board[7] * 1e5
		+ pos.board[8] * 1e4 + pos.board[9] * 1e3
		+ pos.board[10] * 1e2 + pos.board[11]
		+ pos.player * 24 + pos.lastmove;
}


pieces piecesValue(position pos) {
	pieces new_pieces;
	vector<int> run_pos_sum = {};
	for (int c = 1; c <= 12; c++) {
		// stick, hand, run, stop
		if (c == pos.board[c - 1]) {
			new_pieces.stick[c % 2].push_back(c);
		} else if (vectorIndexOf(pos.board, c) == -1) {
			new_pieces.hand[c % 2].push_back(c);
		} else {
			vector<int> c_run_pos = getRunPos(pos.board, c);
			if (c_run_pos.size() == 0) {
				new_pieces.stop[c % 2].push_back(c);
			} else {
				new_pieces.running.push_back(c);
				run_pos_sum.insert(run_pos_sum.end(),
					c_run_pos.begin(), c_run_pos.end());
			}
		}
	}
	// free
	for (int p = 0; p < new_pieces.stop.size(); p++) {
		vector<int> makefree = vectorMerge(new_pieces.hand[p], new_pieces.free[p]);
		for (int x = 0; x < new_pieces.stop[p].size(); x++) {
			int stopx = new_pieces.stop[p][x];
			int stop_pos = vectorIndexOf(pos.board, stopx);
			if (vectorIndexOf(makefree, stop_pos + 1) > -1) {
				new_pieces.free[p].push_back(stopx);
				new_pieces.stop[p].erase(new_pieces.stop[p].begin() + x);
				makefree = vectorMerge(new_pieces.hand[p], new_pieces.free[p]);
				x = -1;
			}
		}
	}
	// stock
	for (int p = 0; p < new_pieces.stop.size(); p++) {
		vector<int> stop_delete;
		for (int x = 0; x < new_pieces.stop[p].size(); x++) {
			int c = new_pieces.stop[p][x];
			int c_pos = vectorIndexOf(pos.board, c);
			if (vectorIndexOf(run_pos_sum, c_pos) == -1) {
				stop_delete.push_back(x);
				new_pieces.stock[p].push_back(c);
			}
		}
		while (stop_delete.size()) {
			new_pieces.stop[p].erase(new_pieces.stop[p].begin() + stop_delete.back());
			stop_delete.pop_back();
		}
	}
	// dead
	for (int p = 0; p < new_pieces.stock.size(); p++) {
		vector<int> stock_delete;
		for (int x = 0; x < new_pieces.stock[p].size(); x++) {
			int c = new_pieces.stock[p][x];
			int c_pos = vectorIndexOf(pos.board, c);
			// if in other player
			if (c_pos % 2 == p) {
				stock_delete.push_back(x);
				new_pieces.dead[p].push_back(c);
			}
			// if multiple stock 
			else {
				int ms = c_pos + 1;
				int ms_pos = vectorIndexOf(pos.board, ms);
				int ts = ms_pos + 1;
				int ts_pos = vectorIndexOf(pos.board, ts);
				if (vectorIndexOf(new_pieces.stock[p], ms) > -1
					&& (ms_pos + 1 == c || 
					vectorIndexOf(new_pieces.stock[p], ts) > -1
					&& ts_pos + 1 == c)) {
					stock_delete.push_back(x);
					new_pieces.dead[p].push_back(c);
				}
			}
		}
		while (stock_delete.size()) {
			new_pieces.stock[p].erase(new_pieces.stock[p].begin() + stock_delete.back());
			stock_delete.pop_back();
		}
	}
	return new_pieces;
}

/*
pos_start:
	1,2,0,4,0,6,7,3,9,10,12,11;1;6
	1,2,0,4,0,6,7,3,9,10,12,11;1
	1,2,0,4,0,6,7,3,9,10,12,11
 */
position getValue(string pos_start) {
	position new_position;
	size_t pos_find, last_pos_find, substr_len;
	vector<size_t> pos_split;
	string pos_string;
	pos_find = 0;
	while (pos_start.find(';', pos_find) != string::npos) {
		pos_find = pos_start.find(';', pos_find);
		pos_split.push_back(pos_find);
		pos_find++;
	}
	if (pos_split.size() == 2) {
		pos_string = pos_start.substr(0, pos_split[0]);
		new_position.player = stoi(pos_start.substr(pos_split[0] + 1, 1));
		new_position.lastmove = stoi(pos_start.substr(pos_split[1] + 1));
	} else if (pos_split.size() == 1) {
		pos_string = pos_start.substr(0, pos_split[0]);
		new_position.player = stoi(pos_start.substr(pos_split[0] + 1));
		new_position.lastmove = -1;
	} else {
		pos_string = pos_start;
		new_position.player = 0;
		new_position.lastmove = -1;
	}
	pos_find = 0;
	int pos_p = 0;
	while (pos_start.find(',', pos_find) != string::npos) {
		last_pos_find = pos_find;
		pos_find = pos_start.find(',', pos_find);
		substr_len = pos_find - last_pos_find;
		new_position.board[pos_p] = stoi(pos_start.substr(last_pos_find, substr_len));
		pos_find++;
		pos_p++;
	}
	new_position.board[sizeof(new_position.board) / sizeof(new_position.board[0]) - 1]
		= stoi(pos_start.substr(pos_find));
	new_position.deep = 0;
	return new_position;
}

vector<position> sortChildren(position pos, vector<position> children) {
	vector<position> first;
	vector<position> normal;
	vector<position> bad;
	for (int i = 0; i < children.size(); i++) {
		int me = pos.player;
		int you = 1 - pos.player;
		int myGoodValue = children[i].pieces_data.stick[me].size()
			+ children[i].pieces_data.hand[me].size()
			+ children[i].pieces_data.free[me].size()
			- pos.pieces_data.stick[me].size()
			+ pos.pieces_data.hand[me].size()
			+ pos.pieces_data.free[me].size();
		int myDeathValue = children[i].pieces_data.dead[me].size()
			- pos.pieces_data.dead[me].size();
		int yourGoodValue = children[i].pieces_data.stick[you].size()
			+ children[i].pieces_data.hand[you].size()
			+ children[i].pieces_data.free[you].size()
			- pos.pieces_data.stick[you].size()
			- pos.pieces_data.hand[you].size()
			- pos.pieces_data.free[you].size();
		int yourDeathValue = children[i].pieces_data.dead[you].size()
			- pos.pieces_data.dead[you].size();
		if (myDeathValue == 0 && yourDeathValue > 0) {
			first.push_back(children[i]);
		} else if (myGoodValue - yourGoodValue > 0) {
			first.push_back(children[i]);
		} else if (myGoodValue - yourGoodValue < 0) {
			bad.push_back(children[i]);
		} else {
			normal.push_back(children[i]);
		}
	}
	vector<position> sorted_children;
	sorted_children.insert(sorted_children.end(), first.begin(), first.end());
	sorted_children.insert(sorted_children.end(), normal.begin(), normal.end());
	sorted_children.insert(sorted_children.end(), bad.begin(), bad.end());
	return sorted_children;
}

int ifEnd(position pos) {
	int me = pos.player;
	int you = 1 - me;
	pieces pd = pos.pieces_data;
	int my_num = pd.stick[me].size();
	int your_num = pd.stick[you].size();
	int my_handle = pd.hand[me].size() + pd.free[me].size();
	int your_handle = pd.hand[you].size() + pd.free[you].size();
	int my_dead = pd.dead[me].size();
	int your_dead = pd.dead[you].size();
	// two win
	if (my_num + my_handle == 6 && your_num + your_handle == 6
			&& my_num - your_num <= 0
			&& my_num - your_num >= -1) {
		return 3;
	}
	// I win
	if (my_num == 6 && your_num < 6
			|| my_num + my_handle == 6 && your_dead > 0
			|| my_num + my_handle == 6 && your_num + your_handle <= 6
			&& my_num - your_num > 0) {
		return 4;
	}
	// I lose
	if (my_num < 5 && your_num == 6
			|| your_num + your_handle == 6 && my_dead > 0
			|| my_num + my_handle <= 6 && your_num + your_handle == 6
			&& your_num - my_num > 1) {
		return 1;
	}
	// two lose
	if (my_dead > 0 && your_dead > 0) {
		return 2;
	}
	return 0;
}

int rollsum = 0;
int maxdeep = 0;
int result_sum = 0;

position roll(position pos) {
	pos.value = ifEnd(pos);
	pos.children.clear();
	rollsum++;
	maxdeep = max(pos.deep, maxdeep);
	if (pos.deep > 24 || rollsum > 1.2e5) {
		return pos;
	}
	// children
	if (pos.value > 0) {
		result_sum++;
		// appendResult(pos);
	} else {
		vector<int> move = vectorMerge(pos.pieces_data.running,
			pos.pieces_data.hand[pos.player]);
		// remove lastmove and (12 if player == 1)
		vectorRemove(move, pos.lastmove);
		if (1 == pos.player) vectorRemove(move, 12);
		vector<position> children;
		for (int y = 0; y < move.size(); y++) {
			position new_pos = pos;
			int c = move[y];
			new_pos.deep = pos.deep + 1;
			new_pos.lastmove = c;
			new_pos.player = 1 - pos.player;
			if (y < move.size() - pos.pieces_data.hand[pos.player].size()) {
				int x = vectorIndexOf(new_pos.board, c);
				new_pos.board[x] = 0;
				if (c != 12) {
					new_pos.board[(x + c) % 12] = c;
				}
				new_pos.pieces_data = piecesValue(new_pos);
				children.push_back(new_pos);
			} else {
				new_pos.board[c - 1] = c;
				int onum = pos.board[c - 1];
				if (onum > 0 && onum % 2 != pos.player) {
					new_pos.player = pos.player;
				}
				new_pos.pieces_data = piecesValue(new_pos);
				children.push_back(new_pos);
			}
		}
		vector<position> _children = sortChildren(pos, children);
		for (int sa = 0; sa < _children.size(); sa++) {
			pos.children.push_back(roll(_children[sa]));
			if (pos.children[sa].value == 1
				&& pos.children[sa].player != pos.player
				|| pos.children[sa].value == 4
				&& pos.children[sa].player == pos.player) {
				break;
			}
		}
		if (move.size() == 0) {
			if (pos.lastmove == 0) {
				pos.value = 2;
				pos.children.clear();
			} else {
				position pass_pos = pos;
				pass_pos.deep = pos.deep + 1;
				pass_pos.lastmove = 0;
				pass_pos.player = 1 - pos.player;
				pos.children.push_back(roll(pass_pos));
			}
		}
		// value
		int max_value = pos.value;
		for (int s = 0; s < pos.children.size(); s++) {
			int this_value = pos.children[s].value;
			if ((this_value == 4 || this_value == 1)
				&& pos.children[s].player != pos.player) {
				this_value = (this_value == 4 ? 1 : 4);
			}
			if (max_value < this_value) {
				max_value = this_value;
			}
		}
		pos.value = max_value;
	}
	return pos;
}

#ifdef BRUTE_FORCE_ALGORITHM
int main()
#else
int tmain()
#endif
{
	// read position
	string pos_start;
	fstream MyFile("ccpos.txt");
	getline(MyFile, pos_start);
	MyFile.close();
	position pos = getValue(pos_start);
	vector<int> pos_board;
	pos.pieces_data = piecesValue(pos);
	position new_pos = roll(pos);
	string pick_child;
	cout << "rollsum:" << rollsum << endl;
	cout << "maxdeep:" << maxdeep << endl;
	cout << "result_sum:" << result_sum << endl;
	cout << endl;
	do {
		cout << "board: ";
		boardCout(new_pos.board);
		cout << "deep:" << new_pos.deep << endl;
		cout << "player: " << new_pos.player << endl;
		cout << "value:" << new_pos.value << endl;
		cout << "available move:" << new_pos.children.size() << endl;
		for (int lm = 0; lm < new_pos.children.size(); lm++) {
			cout << "  " << lm << ": " << new_pos.children[lm].lastmove;
			cout << " (value: " << new_pos.children[lm].value << ") ";
			cout << endl;
		}
		cin >> pick_child;
		if (pick_child != "-3") {
			position new_pos_child = new_pos.children[stoi(pick_child)];
			new_pos = new_pos_child;
		}
	} while (pick_child != "-3");
	return 0;
}
