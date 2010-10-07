#include "AlphaBeta.h"

#include <sstream>
#include <algorithm>
#include <cmath>

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

	turn           = t;
	turnsRemaining = MAX_ROUNDS-turn+1;
	turnsRemaining = std::min<int>(turnsRemaining, 1);

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
		node.Simulate();
	}

	if (depth == turnsRemaining || node.IsTerminal(simulate))
	{
		return node.GetScore();
	}
	
	std::vector<std::vector<Fleet> > actions = node.GetActions();
	Node child(false, node.Planets(), node.Fleets(), file);
	for (unsigned int i = 0, n = actions.size(); i < n; i++)
	{
		child.ApplyAction(simulate, actions[i]);
		alpha = std::max<int>(alpha, -Search(child, depth+1, -beta, -alpha));
		child.RestoreAction(simulate, actions[i]);

		if (beta <= alpha)
		{
			break;
		}
	}
	if (depth == 1)
	{
		if (beta > bestScore)
		{
			bestOrders.clear();
			std::vector<Fleet>& orders = node.GetOrders();
			for (unsigned int i = 0, n = orders.size(); i < n; i++) {
				bestOrders.push_back(orders[i]);
			}
			bestScore = beta;
		}
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

void AlphaBeta::Node::ApplyAction(bool simulate, std::vector<Fleet>& action) {
	for (unsigned int i = 0, n = action.size(); i < n; i++)
	{
		Fleet& order = action[i];
		Planet& src  = AP[order.SourcePlanet()];
		Planet& dst  = AP[order.DestinationPlanet()];
		int ships    = order.NumShips();
		src.Backup();
		dst.Backup();
		src.NumShips(src.NumShips()-ships);
		orders.push_back(order);
		AF.push_back(order);
	}
	// simulation will run, backup the ships
	if (simulate)
	{
		myNumShipsBak    = myNumShips;
		enemyNumShipsBak = enemyNumShips;
	}
}

void AlphaBeta::Node::RestoreAction(bool simulate, std::vector<Fleet>& action) {
	for (unsigned int i = 0, n = action.size(); i < n; i++)
	{
		Fleet& order = action[i];
		Planet& src  = AP[order.SourcePlanet()];
		Planet& dst  = AP[order.DestinationPlanet()];
		src.Restore();
		dst.Restore();
	}
	orders.clear();
	AF.erase(AF.begin()+AF.size()-action.size(), AF.end());
	// simulation has run, restore the ships
	if (simulate)
	{
		myNumShips    = myNumShipsBak;
		enemyNumShips = enemyNumShipsBak;
	}
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

void AlphaBeta::Node::Simulate() {
	sim.Start(1, AP, AF);
	myNumShips    = sim.MyNumShips();
	enemyNumShips = sim.EnemyNumShips();
}


std::vector<Planet>* gAP;
int gTarget;
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
	int distA = ceil(sqrt(pow(t.X()-a.X(),2.0) + pow(t.Y()-a.Y(), 2.0)));
	int distB = ceil(sqrt(pow(t.X()-b.X(),2.0) + pow(t.Y()-b.Y(), 2.0)));
	return distA < distB;
}

// This function contains all the smart stuff
std::vector<std::vector<Fleet> > AlphaBeta::Node::GetActions() {
	gAP = &AP;
	std::vector<std::vector<Fleet> > actions;
	sort(NMP.begin(), NMP.end(), SortOnGrowthShipRatio);
	for (unsigned int i = 0; i < NMP.size(); i++)
	{
		std::vector<Fleet> orders;
		Planet& target = AP[NMP[i]];
		int totalFleet = 0;
		for (unsigned int k = 0, m = MP.size(); k < m; k++)
		{
			Planet& source = AP[MP[k]];
			if (source.NumShips() <= 0)
				continue;
			int fleetSize = std::min<int>(source.NumShips(), target.NumShips()+1);
			orders.push_back(Fleet(1, fleetSize, source.PlanetID(),
				target.PlanetID(), 0, 0));
			totalFleet += fleetSize;
			if (totalFleet == target.NumShips()+1)
				break;
		}
		if (totalFleet == target.NumShips()+1)
			actions.push_back(orders);
	}
	return actions;
}
