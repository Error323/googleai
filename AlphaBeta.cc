#include "AlphaBeta.h"

#include <sstream>
#include <algorithm>
#include <cmath>
#include <cassert>

#include "Moves.inl"


Simulator AlphaBeta::sim;
int       AlphaBeta::turn;

std::vector<Fleet>& AlphaBeta::GetOrders(int t) {
	std::vector<Planet> AP = pw.Planets();
	std::vector<Fleet>  AF = pw.Fleets();
	nodesVisited = 0;
	bestScore    = std::numeric_limits<int>::min();

	turn     = t;
	// NOTE: maxDepth should ALWAYS be equal
	maxDepth = (MAX_ROUNDS-turn+1)*2;
	maxDepth = std::min<int>(maxDepth, 4);

	Node origin(true, AP, AF, file);
	int score = Search(origin, 0, std::numeric_limits<int>::min(),
		std::numeric_limits<int>::max());

	LOG("NODES VISITED: " << nodesVisited << "\tSCORE: " << score);
	return bestOrders;
}

int AlphaBeta::Search(Node& node, int depth, int alpha, int beta)
{
	nodesVisited++;
	bool simulate = (depth > 0 && depth % 2 == 0);
	if (simulate)
	{
		node.ApplySimulation();
	}

	if (depth == maxDepth || node.IsTerminal(simulate))
	{
		return node.GetScore();
	}
	
	std::vector<std::vector<Fleet> > actions = node.GetActions();
	Node child(false, node.Planets(), node.Fleets(), file);
	for (unsigned int i = 0, n = actions.size(); i < n; i++)
	{
		child.AddAction(actions[i]);
		alpha = std::max<int>(alpha, -Search(child, depth+1, -beta, -alpha));
		if (depth == 0 && alpha > bestScore)
		{
			bestOrders.clear();
			std::vector<Fleet>& orders = child.GetOrders();
			for (unsigned int i = 0, n = orders.size(); i < n; i++)
			{
				bestOrders.push_back(orders[i]);
			}
			bestScore = alpha;
		}
		child.RemoveAction(simulate, actions[i].size());

		if (beta <= alpha)
		{
			break;
		}
	}

	if (simulate)
	{
		node.RestoreSimulation();
	}
	return alpha;
}

AlphaBeta::Node::Node(bool init, std::vector<Planet>& p, std::vector<Fleet>& f, std::ofstream& fi):
	AP(p),
	AF(f),
	file(fi) {

	myNumShips       = 0;
	enemyNumShips    = 0;
	myNumShipsBak    = 0;
	enemyNumShipsBak = 0;

	// switch ownership if not the first node
	for (unsigned int i = 0, n = AP.size(); i < n; i++)
	{
		Planet& p = AP[i];
		if (!init)
		{
			if (p.Owner() == 1)
				p.Owner(2);
			else
			if (p.Owner() == 2)
				p.Owner(1);
		}

		switch (p.Owner())
		{
			case 0: {
				NPIDX.push_back(i);
				NMPIDX.push_back(i); 
			} break;

			case 1: {
				MPIDX.push_back(i);
				myNumShips += p.NumShips();
			} break;

			default: {
				NMPIDX.push_back(i); 
				EPIDX.push_back(i);
				enemyNumShips += p.NumShips();
			} break;
		}
		APIDX.push_back(i);
	}

	for (unsigned int i = 0, n = AF.size(); i < n; i++)
	{
		Fleet& f = AF[i];
		if (!init)
		{
			if (f.Owner() == 1)
				f.Owner(2);
			else
			if (f.Owner() == 2)
				f.Owner(1);
		}

		if (f.Owner() == 1)
		{
			MFIDX.push_back(i);
			myNumShips += f.NumShips();
		}
		else
		{
			EFIDX.push_back(i);
			enemyNumShips += f.NumShips();
		}
		AFIDX.push_back(i);
	}
}

void AlphaBeta::Node::AddAction(std::vector<Fleet>& action) {
	for (unsigned int i = 0, n = action.size(); i < n; i++)
	{
		orders.push_back(action[i]);
	}
}

void AlphaBeta::Node::RemoveAction(bool isMe, int size) {
	if (isMe)
		orders.erase(orders.begin(), orders.end()-size);
	else
		orders.erase(orders.begin()+orders.size()-size, orders.end());
}

void AlphaBeta::Node::ApplySimulation() {
	for (unsigned int i = 0, n = orders.size(); i < n; i++)
	{
		Fleet& order = orders[i];
		Planet& src  = AP[order.SourcePlanet()];
		Planet& dst  = AP[order.DestinationPlanet()];
		int ships    = order.NumShips();
		src.Backup();
		dst.Backup();
		src.NumShips(src.NumShips()-ships);
		AF.push_back(order);
	}
	sim.Start(1, AP, AF);
	myNumShipsBak    = myNumShips;
	enemyNumShipsBak = enemyNumShips;
	myNumShips       = sim.MyNumShips();
	enemyNumShips    = sim.EnemyNumShips();
}

void AlphaBeta::Node::RestoreSimulation() {
	for (unsigned int i = 0, n = orders.size(); i < n; i++)
	{
		Fleet& order = orders[i];
		Planet& src  = AP[order.SourcePlanet()];
		Planet& dst  = AP[order.DestinationPlanet()];
		src.Restore();
		dst.Restore();
	}
	AF.erase(AF.begin()+AF.size()-orders.size(), AF.end());
	orders.clear();
	myNumShips    = myNumShipsBak;
	enemyNumShips = enemyNumShipsBak;
}

int AlphaBeta::Node::GetScore() {
	sim.Start(MAX_ROUNDS-turn, AP, AF);
	return sim.GetScore();
}

bool AlphaBeta::Node::IsTerminal(bool simulate) {
	if (simulate)
		return myNumShips <= 0 || enemyNumShips <= 0;
	else
		return false;
}
