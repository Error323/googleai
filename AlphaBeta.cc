#include "AlphaBeta.h"

#include <sstream>
#include <algorithm>
#include <cmath>
#include <cassert>

#define MAX_ROUNDS 200

Simulator AlphaBeta::sim;
int       AlphaBeta::turn;

#define LOG2(depth, msg)                      \
	if (file.good())                          \
	{                                         \
		std::stringstream ss;                 \
		for (int i = 0; i < depth; i++)       \
			ss << "\t";                       \
		file << ss.str() << msg << std::endl; \
	}

std::vector<Fleet>& AlphaBeta::GetOrders(int t) {
	std::vector<Planet> AP = pw.Planets();
	std::vector<Fleet>  AF = pw.Fleets();
	nodesVisited = 0;

	turn     = t;
	// NOTE: maxDepth should ALWAYS be unequal
	maxDepth = (MAX_ROUNDS-turn+1)*2;
	maxDepth = std::min<int>(maxDepth, 2);

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

	LOG2(depth, "D("<<depth<<") M("<<node.myNumShips<<") E("<<node.enemyNumShips<<")");
	if (depth == maxDepth || node.IsTerminal(simulate))
	{
		if (maxDepth % 2 != 0)
		{
			LOG("ERROR: maxDepth should be an even number - "<<maxDepth);
		}
		int score = node.GetScore();
		return score;
	}
	
	std::vector<std::vector<Fleet> > actions = node.GetActions();
	Node child(false, node.Planets(), node.Fleets(), file);
	for (unsigned int i = 0, n = actions.size(); i < n; i++)
	{
		child.AddAction(actions[i]);
		alpha = std::max<int>(alpha, -Search(child, depth+1, -beta, -alpha));
		child.RemoveAction(simulate, actions[i].size());

		if (beta <= alpha)
		{
			break;
		}
	}
	if (depth == 1)
	{
		if (alpha > bestScore)
		{
			bestOrders.clear();
			std::vector<Fleet>& orders = node.GetOrders();
			for (unsigned int i = 0, n = orders.size(); i < n; i++)
			{
				bestOrders.push_back(orders[i]);
			}
			bestScore = alpha;
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
				NP.push_back(i);
				NMP.push_back(i); 
			} break;

			case 1: {
				MP.push_back(i);
				myNumShips += p.NumShips();
			} break;

			default: {
				NMP.push_back(i); 
				EP.push_back(i);
				enemyNumShips += p.NumShips();
			} break;
		}
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
			MF.push_back(i);
			myNumShips += f.NumShips();
		}
		else
		{
			EF.push_back(i);
			enemyNumShips += f.NumShips();
		}
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

// Below this line is very messy atm

std::vector<Planet>* gAP;
int gTarget;

int Distance(const Planet& a, const Planet& b) {
	double x = a.X()-b.X();
	double y = a.Y()-b.Y();
	return int(ceil(sqrt(x*x + y*y)));
}

bool SortOnGrowthShipRatio(const int pidA, const int pidB) {
	const Planet& a = gAP->at(pidA);
	const Planet& b = gAP->at(pidB);
	
	double growA = a.GrowthRate() / (1.0*a.NumShips() + 1.0);
	double growB = b.GrowthRate() / (1.0*b.NumShips() + 1.0);

	return growA > growB;
}

bool SortOnDistanceToTarget(const int pidA, const int pidB) {
	Planet& t      = gAP->at(gTarget);
	Planet& a      = gAP->at(pidA);
	Planet& b      = gAP->at(pidB);
	int distA = Distance(a, t);
	int distB = Distance(b, t);
	return distA < distB;
}

// This function contains all the smart stuff
std::vector<std::vector<Fleet> > AlphaBeta::Node::GetActions() {
	gAP = &AP;
	std::vector<std::vector<Fleet> > actions;
	sort(NMP.begin(), NMP.end(), SortOnGrowthShipRatio);
	for (unsigned int i = 0, n = NMP.size(); i < n; i++)
	{
		std::vector<Fleet> orders;
		Planet& target = AP[NMP[i]];
		int totalFleet = 0;
		gTarget = target.PlanetID();
		sort(MP.begin(), MP.end(), SortOnDistanceToTarget);
		for (unsigned int k = 0, m = MP.size(); k < m; k++)
		{
			Planet& source = AP[MP[k]];
			if (source.NumShips() <= 0)
				continue;
			const int distance = Distance(target, source);
			int fleetSize = std::min<int>(source.NumShips(), target.NumShips()+1);
			orders.push_back(Fleet(1, fleetSize, source.PlanetID(),
				target.PlanetID(), distance, distance));
			totalFleet += fleetSize;
			if (totalFleet >= target.NumShips()+1)
				break;
		}
		if (totalFleet >= target.NumShips()+1)
			actions.push_back(orders);
	}
	return actions;
}
