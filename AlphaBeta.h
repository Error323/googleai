#ifndef ALPHABETA_H_
#define ALPHABETA_H_

#include "PlanetWars.h"
#include "Simulator.h"
#include "Timer.h"

#include <vector>
#include <limits>
#include <ctime>
#include <queue>
#include <utility>

#define MAX_TURNS 200

class AlphaBeta {
public:
	AlphaBeta(PlanetWars& pw_, double maxTime_):
		pw(pw_),
		bestScore(0),
		nodesVisited(0),
		maxTime(maxTime_ - 0.01) {}

	class Action {
	public:
		Action(): value(-1) {}
		Action(std::vector<Fleet>& o, double v):
			orders(o),
			value(v) {}

		std::vector<Fleet> orders;
		double value;
		bool operator< (const Action& a) const {
			return value < a.value;
		}
	};

	class Node {
	public:
		Node() {}
		Node(std::vector<Planet>& P, std::vector<Fleet>& F):
			AP(P),
			AF(F) {}
		std::vector<Planet> AP;
		std::vector<Fleet>  AF;
		std::vector<int>    planetStack;
		void GenerateActions(int, std::vector<int>&, std::vector<int>&, std::vector<AlphaBeta::Action>&);
	};

	std::vector<Fleet>& GetOrders(int turn, int plies);

private:
	std::vector<Action> GetActions(Node&, int);
	std::vector<Node> nodeStack;
	Action bestAction;

	PlanetWars& pw;
	int bestScore;
	int nodesVisited;

	Simulator sim;
	int turn;
	int maxDepth;
	double maxTime;

	int Search(Node&, std::pair<Action,Action>, int, int, int);
	void Simulate(Node&, std::pair<Action, Action>&, int);
	bool IsTerminal(int);
	int GetScore(Node&, int);

	Timer timer;
};

#endif
