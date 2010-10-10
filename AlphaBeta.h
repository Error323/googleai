#ifndef ALPHABETA_H_
#define ALPHABETA_H_

#include "PlanetWars.h"
#include "Simulator.h"

#include <vector>
#include <limits>

#define MAX_ROUNDS 200

class GameState {
public:
	GameState(){}
	GameState(int depth, std::vector<Planet>& ap, std::vector<Fleet>& af);
	
	std::vector<Planet> AP;		// all planets
	std::vector<Fleet>  AF;		// all fleets
	std::vector<Fleet> orders;	// orders

	int myNumShips;				// my total amount of ships
	int enemyNumShips;			// enemy total amount of ships

private:
	int depth;
};

class AlphaBeta {
public:
	AlphaBeta(PlanetWars& pw_):
		pw(pw_),
		bestScore(std::numeric_limits<int>::min()),
		nodesVisited(0)
	{
	}

	class Node {
	friend class AlphaBeta;
	public:
		Node(int, std::vector<GameState>&, GameState&);

		void                             ApplySimulation();
		void                             RestoreSimulation();
		void                             AddOrders(std::vector<Fleet>& orders);
		void                             RemoveOrders();
		int                              GetScore();
		bool                             IsTerminal(bool s);
		std::vector<std::vector<Fleet> > GetActions();

	private:
		int depth;
		std::vector<GameState> history;
		GameState& prev;
		GameState  curr;
	};

	std::vector<Fleet>& GetOrders(int turn, int plies);

private:
	int Search(Node& node, int depth, int alpha, int beta);

	PlanetWars& pw;
	int bestScore;
	int maxDepth;
	int nodesVisited;
	std::vector<Fleet> bestOrders;

	static Simulator sim;
	static int turn;
};

#endif
