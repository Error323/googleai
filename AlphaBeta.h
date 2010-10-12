#ifndef ALPHABETA_H_
#define ALPHABETA_H_

#include "PlanetWars.h"
#include "Simulator.h"

#include <vector>
#include <limits>
#include <ctime>

#define MAX_TURNS 200
#define MAX_TIME  0.99999

class GameState {
public:
	GameState(){}
	GameState(int depth, std::vector<Planet>& ap, std::vector<Fleet>& af);
	
	std::vector<Planet> AP;		// all planets
	std::vector<Fleet>  AF;		// all fleets
	std::vector<Fleet> orders;	// orders

	std::vector<int> APIDX;		// all planets indices
	std::vector<int> NPIDX;		// neutral planets indices
	std::vector<int> EPIDX;		// enemy planets indices
	std::vector<int> MPIDX;		// my planets indices
	std::vector<int> TPIDX;		// my planets that are targetted and captured in the future
	std::vector<int> NTPIDX;	// my planets that are not targetted
	std::vector<int> NMPIDX;	// not my planets indices
	std::vector<int> EFIDX;		// enemy fleets indices
	std::vector<int> MFIDX;		// my fleets indices

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
	int nodesVisited;
	std::vector<Fleet> bestOrders;

	static Simulator sim;
	static int turn;
	static int maxDepth;

	time_t start, end;
};

#endif
