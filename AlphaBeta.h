#ifndef ALPHABETA_H_
#define ALPHABETA_H_

#include "PlanetWars.h"
#include "Simulator.h"

#include <vector>

class AlphaBeta {
public:
	AlphaBeta(PlanetWars& pw_, std::ofstream& file_):
		pw(pw_),
		file(file_),
		bestScore(-10000000)
	{
	}

	class Node {
	public:
		Node(){}
		Node(std::vector<Planet>& p, std::vector<Fleet>& f):
			AP(p),
			AF(f)
		{}

		std::vector<Planet>&             Planets()				{ return AP; }
		std::vector<Fleet>&              Fleets()				{ return AF; }
		std::vector<Fleet>&              GetOrders()			{ return orders; }
		std::vector<std::vector<Fleet> > GetActions();
		std::vector<Node>                GetChildren();
		int                              GetScore(bool isMe);
		bool                             IsTerminal();
		void                             Simulate();
		void ApplyAction(std::vector<Fleet>& action);
		void RestoreAction(std::vector<Fleet>& action);

	private:
		std::vector<Fleet>	orders;	// the action
		std::vector<Planet> AP;		// all planets
		std::vector<Fleet>  AF;     // all fleets
	};

	std::vector<Fleet> GetOrders(int turn);

private:
	int Search(Node& node, int depth, int alpha, int beta);

	PlanetWars& pw;
	std::ofstream& file;
	int bestScore;
	int turnsRemaining;
	std::vector<Fleet> bestOrders;
	static std::vector<Node>  stack;
};

#endif
