#include "AlphaBeta.h"
#include "Logger.h"
#include "vec3.h"
#include "Map.h"
#include "KnapSack.h"

#include <sstream>
#include <algorithm>
#include <cmath>
#include <queue>
#include <iostream>
#include <algorithm>
#include <limits>

#define SIMULATION_STEPS 1 

std::vector<Fleet>& AlphaBeta::GetOrders(int t, int plies) {
	timer.Tick();
	std::vector<Planet> AP = pw.Planets();
	std::vector<Fleet>  AF = pw.Fleets();
	nodesVisited = 0;
	bestScore    = std::numeric_limits<int>::min();
	turn         = t;

	maxDepth = plies*2;
	maxDepth = std::min<int>(maxDepth, (MAX_TURNS-turn)*2);
	nodeStack.resize(plies);

	Node origin(AP, AF);
	Action a;
	std::pair<Action,Action> actionPair(a, a);
	sim.Start(0, AP, AF);
	int alpha = Search(origin, actionPair, 0, std::numeric_limits<int>::min(),
		std::numeric_limits<int>::max());

	ASSERT_MSG(bestScore == alpha, "bestscore: "<<bestScore<<"\talpha: "<<alpha);
	LOG("VISITED: "<<nodesVisited<<"\tSCORE: "<<bestScore);
	return bestAction.orders;
}

int AlphaBeta::Search(Node& node, std::pair<Action, Action> actionPair, int depth, int alpha, int beta) {
	nodesVisited++;
	if (IsTerminal(depth))
	{
		return GetScore(node, depth);
	}

	if (depth % 2 == 1)
	{
		nodeStack[depth/2] = node;
		Simulate(node, actionPair, depth);
	}

	std::vector<Action> actions = GetActions(node, depth);
	for (int i = 0, n = actions.size(); i < n; i++)
	{
		if (depth % 2 == 0)
			actionPair.first = actions[i];
		else
			actionPair.second = actions[i];

		alpha = std::max<int>(alpha, -Search(node, actionPair, depth + 1, -beta, -alpha));

		if (depth == 0 && alpha > bestScore)
		{
			bestAction = actions[i];
			bestScore = alpha;
		}

		if (beta <= alpha)
		{
			break;
		}
	}

	if (depth % 2 == 1)
	{
		node = nodeStack[depth/2];
	}

	return alpha;
}

void AlphaBeta::Simulate(Node& node, std::pair<Action, Action>& actionPair, int depth) {
	for (int i = 0, n = actionPair.first.orders.size(); i < n; i++)
	{
		Fleet& order = actionPair.first.orders[i];
		Planet& source = node.AP[order.SourcePlanet()];
		ASSERTD(order.Owner() == source.Owner());
		source.RemoveShips(order.NumShips());
		node.AF.push_back(order);
	}
	for (int i = 0, n = actionPair.second.orders.size(); i < n; i++)
	{
		Fleet& order = actionPair.second.orders[i];
		Planet& source = node.AP[order.SourcePlanet()];
		ASSERTD(order.Owner() == source.Owner());
		source.RemoveShips(order.NumShips());
		node.AF.push_back(order);
	}
	sim.Start(1, node.AP, node.AF);
}

int AlphaBeta::GetScore(Node& node, int depth) {
	if (sim.EnemyNumShips() <= 0)
		return std::numeric_limits<int>::max();
	
	if (sim.MyNumShips() <= 0)
		return std::numeric_limits<int>::min();

	sim.Start(MAX_TURNS-turn-depth/2, node.AP, node.AF);
	return sim.GetScore();
}

bool AlphaBeta::IsTerminal(int depth) {
	timer.Tock();

	if (timer.Time() >= maxTime)
		return true;

	if (depth >= maxDepth)
		return true;

	if (depth/2 + turn >= MAX_TURNS-1)
		return true;

	return sim.MyNumShips() <= 0 || sim.EnemyNumShips() <= 0;
}




// Everything below this line is for gathering good actions
namespace bot {
	#include "Helper.inl"
}

bool Defend(int turnsRemaining, int tid, std::vector<Planet>& AP, std::vector<Fleet>& AF,
				std::vector<int>& NTPIDX, std::vector<int>& EFIDX,
				std::vector<Fleet>& orders, bool restore) {

	Simulator end, sim;
	end.Start(turnsRemaining, AP, AF, false, true);
	Simulator::PlanetOwner& enemy = end.GetFirstEnemyOwner(tid);
	Planet& target = AP[tid];
	bot::gTarget = tid;
	sort(NTPIDX.begin(), NTPIDX.end(), bot::SortOnDistanceToTarget);
	bool success = false;
	for (unsigned int j = 0, m = NTPIDX.size(); j < m; j++)
	{
		Planet& source = AP[NTPIDX[j]];
		const int sid = source.PlanetID();
		if (source.NumShips() <= 0)
			continue;

		const int dist = target.Distance(source);
		if (dist > enemy.time)
			continue;

		source.Backup();
		sim.Start(enemy.time, AP, AF, false, true);
		int numShips = sim.GetPlanet(tid).NumShips();
		numShips = std::min<int>(numShips, source.NumShips()-bot::GetIncommingFleets(sid, EFIDX));

		if (numShips <= 0)
			continue;

		source.RemoveShips(numShips);
		Fleet order(1, numShips, sid, tid, dist, dist);
		AF.push_back(order);
		orders.push_back(order);
		sim.Start(enemy.time, AP, AF, false, true);
		if (sim.IsMyPlanet(tid))
		{
			success = true;
			break;
		}
	}
	if (!success || restore)
	{
		for (unsigned int j = 0, m = orders.size(); j < m; j++)
			AP[orders[j].SourcePlanet()].Restore();

		AF.erase(AF.begin() + AF.size() - orders.size(), AF.end());
	}
	return success;
}

bool Attack(int sid, int tid, std::vector<Planet>& AP, std::vector<Fleet>& AF,
				std::vector<int>& EFIDX, std::vector<Fleet>& orders, bool restore) {

	Simulator sim;
	bool canAttack = false;
	Planet& source = AP[sid];
	Planet& target = AP[tid];
	const int dist = source.Distance(target);
	sim.Start(dist, AP, AF, false, true);
	int numShipsRequired = sim.GetPlanet(tid).NumShips();
	int numShips = source.NumShips()-bot::GetIncommingFleets(sid, EFIDX);
	canAttack = numShips > numShipsRequired;

	if (canAttack)
	{
		Fleet order(1, numShips, sid, tid, dist, dist);
		source.Backup();
		source.RemoveShips(numShips);
		orders.push_back(order);
		AF.push_back(order);
	}

	if (canAttack && restore)
	{
		source.Restore();
		AF.pop_back();
	}

	return canAttack;
}

std::vector<AlphaBeta::Action> AlphaBeta::GetActions(Node& n, int depth) {
	std::priority_queue<Action> actions;
	std::vector<Planet>& AP = n.AP;
	std::vector<Fleet> & AF = n.AF;
	std::vector<int> MPIDX;
	std::vector<int> NMPIDX;
	int owner = 0;
	if (depth % 2 == 0)
		owner = 1;
	else
		owner = 2;
	int sid = -1;
	int numShips = 0;
	for (int i = 0, n = AP.size(); i < n; i++)
	{
		Planet& p = AP[i];
		if (p.Owner() == owner)
		{
			if (p.NumShips() > numShips)
			{
				sid = p.PlanetID();
				numShips = p.NumShips();
			}

			MPIDX.push_back(i);
		}
		else NMPIDX.push_back(i);
	}

	std::vector<Fleet> orders;
	actions.push(Action(orders, -1, -1));
	if (sid != -1)
	{
		Planet& source = AP[sid];
		for (int i = 0, n = NMPIDX.size(); i < n; i++)
		{
			Planet& target = AP[NMPIDX[i]];
			int tid = target.PlanetID();
			if (sid == tid)
				continue;
			int dist = source.Distance(target);
			Fleet order(owner, source.NumShips(), sid, tid, dist, dist);
			orders.push_back(order);
			double value = target.GrowthRate()/(target.NumShips()*dist + 1.0);
			actions.push(Action(orders, value, i));
			orders.clear();
		}
	}

	std::vector<Action> A;
	while (!actions.empty())
	{
		A.push_back(actions.top());
		actions.pop();
	}

	return A;
}

std::ostream& operator<<(std::ostream &out, const AlphaBeta::Node& node) {
	for (int i = 0, n = node.AP.size(); i < n; i++)
	{
		out << node.AP[i];
	}
	for (int i = 0, n = node.AF.size(); i < n; i++)
	{
		out << node.AF[i];
	}
	return out;
}
