#ifndef ALPHABETA_H_
#define ALPHABETA_H_

#include "PlanetWars.h"
#include "Simulator.h"

#include <vector>
#include <limits>

class AlphaBeta {
public:
	AlphaBeta(PlanetWars& pw_, std::ofstream& file_):
		pw(pw_),
		file(file_),
		nodesVisited(0),
		bestScore(std::numeric_limits<int>::min())
	{
	}

	class Node {
	public:
		Node(bool, std::vector<Planet>&, std::vector<Fleet>&, std::ofstream&);

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
		void                             RemoveAction(bool,int);

	private:
		friend class AlphaBeta;
		std::ofstream& file;

		// NOTE: All members below are wrt the node-turn, 
		//       e.g. enemy planets could be OUR planets
		//       this depends on the isMe boolean value
		std::vector<Fleet>	orders;	// the action
		std::vector<Planet> AP;		// all planets
		std::vector<Fleet>  AF;		// all fleets
		std::vector<int> NP;		// neutral planets
		std::vector<int> EP;		// enemy planets
		std::vector<int> MP;		// my planets
		std::vector<int> NMP;		// not my planets
		std::vector<int> EF;		// enemy fleets
		std::vector<int> MF;		// my fleets
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
