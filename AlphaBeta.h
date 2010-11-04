#ifndef ALPHABETA_H_
#define ALPHABETA_H_

#include "PlanetWars.h"
#include "Simulator.h"
#include "Timer.h"

#include <vector>
#include <limits>
#include <ctime>
#include <queue>

#define MAX_TURNS 200

class AlphaBeta {
public:
	AlphaBeta(PlanetWars& pw_, double maxTime_):
		pw(pw_),
		bestScore(0),
		nodesVisited(0),
		maxTime(maxTime_ - 0.01) {}

	class Node {
	public:
		Node() {}
		Node(std::vector<Planet>& P, std::vector<Fleet>& F):
			AP(P),
			AF(F) {}
		std::vector<Planet> AP;
		std::vector<Fleet>  AF;
		friend std::ostream& operator<<(std::ostream&, const Node&);
	};

	class Action {
	public:
		Action(): value(-1), id(-666) {}
		Action(std::vector<Fleet>& o, double v, int i):
			orders(o),
			value(v),
			id(i) {}

		std::vector<Fleet> orders;
		double value;
		int id;
		bool operator< (const Action& a) const {
			return value < a.value;
		}
	};

	std::vector<Fleet>& GetOrders(int turn, int plies);

private:
	std::vector<Action> GetActions(Node&, int);
	std::vector<Node> nodeStack;
	std::vector<Action> actionStack;
	Action bestAction;

	PlanetWars& pw;
	int bestScore;
	int nodesVisited;

	Simulator sim;
	int turn;
	int maxDepth;
	double maxTime;

	int Search(Node&, int, int, int);
	void Simulate(Node&, int);
	bool IsTerminal(int);
	int GetScore(Node&, int);

	Timer timer;
};

#endif
