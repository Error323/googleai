#include "AlphaBeta.h"
#include "Logger.h"

#include <sstream>
#include <algorithm>
#include <cmath>
#include <cassert>

GameState::GameState(int d, std::vector<Planet>& ap, std::vector<Fleet>& af): 
	AP(ap),
	AF(af),
	depth(d) {
	// switch ownership if not the first node
	myNumShips = enemyNumShips = 0;
	for (unsigned int i = 0, n = AP.size(); i < n; i++)
	{
		Planet& p = AP[i];
		int owner = p.Owner();
		// at depth = 0, we don't want to swap owner and neutral planets never
		// swap owner
		owner = (depth > 0 && owner != 0) ? (owner % 2) + 1 : owner;
		p.Owner(owner);

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
		int owner = f.Owner();
		owner = (depth > 0) ? (owner % 2) + 1 : owner;
		f.Owner(owner);

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

Simulator AlphaBeta::sim;
int       AlphaBeta::turn;

std::vector<Fleet>& AlphaBeta::GetOrders(int t, int plies) {
	std::vector<Planet> AP = pw.Planets();
	std::vector<Fleet>  AF = pw.Fleets();
	nodesVisited = 0;
	bestScore    = std::numeric_limits<int>::min();
	turn         = t;

	maxDepth = plies*2;
	maxDepth = std::min<int>(maxDepth, (MAX_ROUNDS-turn)*2);

	GameState gs(0, AP, AF);
	std::vector<GameState> history;
	history.push_back(gs);
	Node origin(0, history, gs);

	int score = Search(origin, 0, std::numeric_limits<int>::min(),
		std::numeric_limits<int>::max());

	LOG("NODES VISITED: " << nodesVisited << "\tSCORE: " << score);
	return bestOrders;
}

int AlphaBeta::Search(Node& node, int depth, int alpha, int beta) {
	nodesVisited++;
	bool simulate = (depth > 0 && depth % 2 == 0);
	if (simulate)
	{
		node.ApplySimulation();
	}
	
	if (depth == maxDepth || node.IsTerminal(simulate))
	{
		int score = node.GetScore();
		node.RestoreSimulation();
		return score;
	}
	
	Node child(depth+1, node.history, node.curr);
	std::vector<std::vector<Fleet> > actions = node.GetActions();
	for (unsigned int i = 0, n = actions.size(); i < n; i++)
	{
		child.AddOrders(actions[i]);
		alpha = std::max<int>(alpha, -Search(child, depth+1, -beta, -alpha));

		// TODO: Verify if this is correct
		if (beta <= alpha)
		{
			//break;
		}

		if (depth == 0 && alpha > bestScore)
		{
			bestOrders = actions[i];
			bestScore = alpha;
		}
		
		child.RemoveOrders();
	}

	if (simulate)
	{
		node.RestoreSimulation();
	}

	return alpha;
}

AlphaBeta::Node::Node(int d, std::vector<GameState>& h, GameState& p):
	depth(d),
	history(h),
	prev(p) {

	curr = GameState(depth, prev.AP, prev.AF);
	history.push_back(curr);
}

void AlphaBeta::Node::AddOrders(std::vector<Fleet>& action) {
	curr.orders = action;
}

void AlphaBeta::Node::RemoveOrders() {
	curr.orders.clear();
}

void AlphaBeta::Node::ApplySimulation() {
	for (unsigned int i = 0, n = prev.orders.size(); i < n; i++)
	{
		Fleet& order = prev.orders[i];
		Planet& src  = curr.AP[order.SourcePlanet()];
		src.RemoveShips(order.NumShips());
		int index = curr.AF.size();
		curr.EFIDX.push_back(index);
		curr.AFIDX.push_back(index);
		curr.AF.push_back(order);
	}
	for (unsigned int i = 0, n = curr.orders.size(); i < n; i++)
	{
		Fleet& order = curr.orders[i];
		Planet& src  = curr.AP[order.SourcePlanet()];
		src.RemoveShips(order.NumShips());
		int index = curr.AF.size();
		curr.MFIDX.push_back(index);
		curr.AFIDX.push_back(index);
		curr.AF.push_back(order);
	}
	sim.Start(1, curr.AP, curr.AF);
	curr.myNumShips = sim.MyNumShips();
	curr.enemyNumShips = sim.EnemyNumShips();
	for (unsigned int i = 0; i < curr.AF.size(); i++)
	{
		if (curr.AF[i].TurnsRemaining() <= 0)
			LOGD(">"<<curr.AF[i]);
	}
}

void AlphaBeta::Node::RestoreSimulation() {
	curr = history.back();
	for (unsigned int i = 0; i < curr.AF.size(); i++)
	{
		if (curr.AF[i].TurnsRemaining() <= 0)
			LOGD(">"<<curr.AF[i]);
	}
}

int AlphaBeta::Node::GetScore() {
	if (curr.enemyNumShips <= 0 && curr.myNumShips > 0)
		return std::numeric_limits<int>::max();

	if (curr.enemyNumShips <= 0 && curr.enemyNumShips > 0)
		return std::numeric_limits<int>::min();

	sim.Start(MAX_ROUNDS-turn-(depth/2), curr.AP, curr.AF);
	return sim.GetScore();
}

bool AlphaBeta::Node::IsTerminal(bool s) {
	if (s)
		return curr.myNumShips <= 0 || curr.enemyNumShips <= 0;
	else
		return false;
}

// Everything below this line is for gathering good actions
// globals
std::vector<Planet>* gAP;
int gTarget;

inline int Distance(const Planet& a, const Planet& b) {
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
	Planet& t = gAP->at(gTarget);
	Planet& a = gAP->at(pidA);
	Planet& b = gAP->at(pidB);
	int distA = Distance(a, t);
	int distB = Distance(b, t);
	return distA < distB;
}

// This function contains all the smart stuff
std::vector<std::vector<Fleet> > AlphaBeta::Node::GetActions() {
	gAP = &curr.AP;
	int owner = depth % 2 + 1;
	std::vector<std::vector<Fleet> > actions;
	sort(curr.NMPIDX.begin(), curr.NMPIDX.end(), SortOnGrowthShipRatio);
	for (unsigned int i = 0, n = curr.NMPIDX.size(); i < n; i++)
	{
		std::vector<Fleet> orders;
		Planet& target = curr.AP[curr.NMPIDX[i]];
		int totalFleet = 0;
		gTarget = target.PlanetID();
		sort(curr.MPIDX.begin(), curr.MPIDX.end(), SortOnDistanceToTarget);
		for (unsigned int k = 0, m = curr.MPIDX.size(); k < m; k++)
		{
			Planet& source = curr.AP[curr.MPIDX[k]];
			if (source.NumShips() <= 0 || target.PlanetID() == source.PlanetID())
				continue;
			const int distance = Distance(target, source);
			int fleetSize = std::min<int>(source.NumShips(), target.NumShips()+1);
			orders.push_back(Fleet(owner, fleetSize, source.PlanetID(),
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
