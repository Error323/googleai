#include <iostream>
#include <sstream>
#include <vector>
#include <utility>

#include <cstdlib>
#include <ctime>

#define LOGD(msg)                       \
	do {                                \
        for (int i = 0; i < depth; i++) \
            std::cout << "\t";          \
        std::cout << msg << std::endl;  \
	}                                   \
	while (0)


struct State {
	State() {
		planets.push_back(1);
		planets.push_back(2);
	}

	// change ownership
	State(std::vector<int>& p, std::vector<std::pair<int,int> >& f): planets(p), fleets(f) {
		for (size_t i = 0; i < planets.size(); i++)
		{
			planets[i] = (planets[i] % 2) + 1;
		}
		for (size_t i = 0; i < fleets.size(); i++)
		{
			fleets[i].first = (fleets[i].first % 2) + 1;
		}
	}

	std::vector<int> planets;
	std::vector<std::pair<int,int> > fleets;
	std::vector<std::pair<int,int> > orders;

	friend std::ostream& operator<<(std::ostream& out, const State& s);
};

std::ostream& operator<<(std::ostream& out, const State& s) {
/*
	out << "{p[";
	if (!s.planets.empty())
	{
		for (size_t i = 0; i < s.planets.size()-1; i++)
		{
			out << s.planets[i] << ", ";
		}
		out << s.planets.back();
	}
	out << "]";
	out << "{o[";
	if (!s.orders.empty())
	{
		for (size_t i = 0; i < s.orders.size()-1; i++)
		{
			out << "(" << s.orders[i].first << ":" << s.orders[i].second << "), ";
		}
		out << "(" << s.orders.back().first << ":" << s.orders.back().second << ")";
	}
	out << "]";
*/
	out << " f[";
	if (!s.fleets.empty())
	{
		for (size_t i = 0; i < s.fleets.size()-1; i++)
		{
			out << "(" << s.fleets[i].first << ":" << s.fleets[i].second << "), ";
		}
		out << "(" << s.fleets.back().first << ":" << s.fleets.back().second << ")";
	}
	out << "]";
	return out;
}

struct Node {
	Node(int d, std::vector<State>& S, State& p): 
		depth(d), 
		states(S),
		prev(p)
	{
		curr = State(prev.planets, prev.fleets);
		states.push_back(curr);
	}

	void add(int n) {
		for (int j = 0; j < n; j++)
		{
			curr.orders.push_back(std::make_pair((depth+1)%2+1,rand() % 100));
		}
	}

	void rem() {
		curr.orders.clear();
	}

	void simulate() {
		for (size_t i = 0; i < prev.orders.size(); i++)
		{
			curr.fleets.push_back(prev.orders[i]);
		}
		
		for (size_t i = 0; i < curr.orders.size(); i++)
		{
			curr.fleets.push_back(curr.orders[i]);
		}
	}

	void restore() {
		if (depth > 0)
		curr = states.back();
	}

	int depth;
	std::vector<State> states;
	State  curr;
	State& prev;
};

void print(Node &n, int depth, char c) {
	LOGD(c<<n.curr);
}

static const int D = 4;

void rec(int depth, Node& n) {
	if (depth > 0 && depth % 2 == 0)
	{
		n.simulate();
	}

	print(n, depth, '>');

	if (depth == D)
	{
		print(n, depth, '<');
		n.restore();
		return;
	}

	Node c(depth+1, n.states, n.curr);
	for (size_t i = 1; i <= 2; i++)
	{
		c.add(1);
		rec(depth+1, c);
		c.rem();
	}

	if (depth > 0 && depth % 2 == 0)
	{
		n.restore();
	}

	print(n, depth, '<');
};

int main(void) {
	srand(time(NULL));
	State s;
	std::vector<State> states;
	states.push_back(s);
	Node n(0, states, s);
	rec(0, n);
	return 0;
}
