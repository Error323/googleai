#ifndef ALPHABETA_H_
#define ALPHABETA_H_

#include "PlanetWars.h"
#include "Simulator.h"

#include <vector>
#include <limits>

#define MAX_ROUNDS 200

#define LOGD(msg)                             \
	if (file.good())                          \
	{                                         \
		std::stringstream ss;                 \
		for (int i = 0; i < depth; i++)       \
			ss << "\t";                       \
		file << ss.str() << msg << std::endl; \
	}

class AlphaBeta {
public:
	AlphaBeta(PlanetWars& pw_, std::ofstream& file_):
		pw(pw_),
		file(file_),
		bestScore(std::numeric_limits<int>::min()),
		nodesVisited(0)
	{
	}

	class Node {
	public:
		Node(int, std::vector<Planet>&, std::vector<Fleet>&);

		std::vector<Planet>&             Planets()				{ return AP; }
		std::vector<Fleet>&              Fleets()				{ return AF; }
		std::vector<Fleet>&              GetOrders()			{ return orders; }
		std::vector<std::vector<Fleet> > GetActions();
		std::vector<Node>                GetChildren();
		int                              GetScore();
		bool                             IsTerminal(bool simulate);
		void                             ApplySimulation();
		void                             RestoreSimulation();
		void                             AddAction(std::vector<Fleet>& action);
		void                             RemoveAction(int);

	private:
		friend class AlphaBeta;
		std::vector<Fleet>	orders;	// the action
		int depth;

		// NOTE: All members below are wrt the node-turn, 
		//       e.g. enemy planets could be OUR planets
		//       this depends on the isMe boolean value
		std::vector<Planet> AP;		// all planets
		std::vector<Fleet>  AF;		// all fleets
		std::vector<int> APIDX;		// all planets indices
		std::vector<int> AFIDX;		// all fleets indices
		std::vector<int> NPIDX;		// neutral planets indices
		std::vector<int> EPIDX;		// enemy planets indices
		std::vector<int> MPIDX;		// my planets indices
		std::vector<int> NMPIDX;	// not my planets indices
		std::vector<int> EFIDX;		// enemy fleets indices
		std::vector<int> MFIDX;		// my fleets indices
		int myNumShips;				// my total amount of ships
		int enemyNumShips;			// enemy total amount of ships
		int myNumShipsBak;
		int enemyNumShipsBak;
	};

	std::vector<Fleet>& GetOrders(int turn);

private:
	int Search(Node& node, int depth, int alpha, int beta);

	PlanetWars& pw;
	std::ofstream& file;
	int bestScore;
	int maxDepth;
	int nodesVisited;
	std::vector<Fleet> bestOrders;

	static Simulator sim;
	static int turn;
};

#endif
