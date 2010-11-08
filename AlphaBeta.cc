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
	for (int i = actions.size()-1; i >= 0; i--)
	{
		LOGD(actions[i].value);
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
		order.Owner(1);
		Planet& source = node.AP[order.SourcePlanet()];
		ASSERTD(order.Owner() == source.Owner());
		source.RemoveShips(order.NumShips());
		node.AF.push_back(order);
	}
	for (int i = 0, n = actionPair.second.orders.size(); i < n; i++)
	{
		Fleet& order = actionPair.second.orders[i];
		order.Owner(2);
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

void AlphaBeta::Node::GenerateActions(int depth, std::vector<int>& FLPIDX,
	std::vector<int>& PIDX, std::vector<AlphaBeta::Action>& actions) {
	
	int F = FLPIDX.size();
	if (depth == F)
	{
		std::vector<Fleet> orders;
		for (int i = 0; i < F; i++)
		{
			Planet& source = AP[FLPIDX[i]];
			Planet& target = AP[planetStack[i]];
			const int sid  = source.PlanetID();
			const int tid  = target.PlanetID();
			const int dist = source.Distance(target);

			// we want this as an "empty" action
			if (sid == tid)
				continue;

			Fleet order(1, source.NumShips(), sid, tid, dist, dist);
			orders.push_back(order);
		}
		actions.push_back(Action(orders, 0.0));

		return;
	}

	for (int i = 0, n = PIDX.size(); i < n; i++)
	{
		planetStack.push_back(PIDX[i]);
		GenerateActions(depth + 1, FLPIDX, PIDX, actions);
		planetStack.pop_back();
	}
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

std::vector<AlphaBeta::Action> AlphaBeta::GetActions(Node& node, int depth) {
	std::vector<Planet> ap(node.AP);
	std::vector<Fleet>  af(node.AF);

	// swap ownership on the enemy turn
	if (depth % 2 == 1)
	{
		for (int i = 0, n = ap.size(); i < n; i++)
		{
			Planet& p = ap[i];
			if (p.Owner() == 1)
				p.Owner(2);
			else
			if (p.Owner() == 2)
				p.Owner(1);
		}
		for (int i = 0, n = af.size(); i < n; i++)
		{
			Fleet& f = af[i];
			if (f.Owner() == 1)
				f.Owner(2);
			else
				f.Owner(1);
		}
	}

	bot::gAP = &ap;
	bot::gAF = &af;
	std::vector<int> NPIDX;  // neutral planets
	std::vector<int> MPIDX;  // not targetted planets belonging to us
	std::vector<int> EPIDX;  // enemy planets
	std::vector<int> EFIDX;  // enemy fleets
	std::vector<int> MFIDX;  // my fleets
	std::vector<int> NMPIDX; // not my planets
	Simulator end;
	end.Start(MAX_TURNS-turn-depth/2, ap, af, false, true);
	for (int i = 0, n = ap.size(); i < n; i++)
	{
		Planet& p = ap[i];
		switch (p.Owner())
		{
			case 0:
				if (!end.IsMyPlanet(i))
				{
					if (end.IsEnemyPlanet(i))
						EPIDX.push_back(i);
					else
						NPIDX.push_back(i); 
					NMPIDX.push_back(i);  
				}
			break;

			case 1:  
				MPIDX.push_back(i);
			break;

			default:
				if (!end.IsMyPlanet(i))
				{
					EPIDX.push_back(i); 
					NMPIDX.push_back(i);
				}
			break;
		}
	}

	for (int i = 0, n = af.size(); i < n; i++)
	{
		Fleet& f = af[i];
		if (f.Owner() == 1)
			MFIDX.push_back(i);
		else
			EFIDX.push_back(i);
	}

	Map map(ap);
	std::vector<int> FLPIDX = map.GetFrontLine();

	std::vector<Action> actions;
	if (turn == 0)
		node.GenerateActions(0, FLPIDX, NPIDX, actions);
	else
	if (FLPIDX.size() > 2)
		node.GenerateActions(0, FLPIDX, EPIDX, actions);
	else
		node.GenerateActions(0, FLPIDX, NMPIDX, actions);

	for (int i = 0, n = actions.size(); i < n; i++)
	{
		std::vector<Planet> AP(ap);
		std::vector<Fleet>  AF(af);
		Action& a = actions[i];
		std::vector<Fleet>& orders = a.orders;

		for (int j = 0, m = orders.size(); j < m; j++)
		{
			Fleet& order = orders[j];
			AP[order.SourcePlanet()].RemoveShips(order.NumShips());
			AF.push_back(order);
		}

		// ---------------------------------------------------------------------------
		// LOGD("FEED"); // support the frontline through routing
		// ---------------------------------------------------------------------------
		std::vector<Planet> AFP(AP); // all future planets
		std::vector<Fleet>  AFF(AF); // all future fleets
		end.Start(MAX_TURNS-turn, AFP, AFF);
		Map fmap(AFP); // future map, to compute future frontline
		std::vector<int>& FFLPIDX = fmap.GetFrontLine();

		for (unsigned int j = 0, m = MPIDX.size(); j < m; j++)
		{
			Planet& source = AP[MPIDX[j]];
			const int sid = source.PlanetID();
			if (find(FFLPIDX.begin(), FFLPIDX.end(), sid) != FFLPIDX.end())
				continue;

			const int tid = map.GetClosestPlanetIdx(source.Loc(), FFLPIDX);
			if (tid == -1)
				continue;

			Planet& target = AP[tid];
			const int dist = target.Distance(source);
			const int numShips = source.NumShips() - bot::GetIncommingFleets(sid, EFIDX);
			if (numShips <= 0)
				continue;

			const int hid = bot::GetHub(sid, tid);
			Fleet order(1, numShips, sid, hid, dist, dist);
			orders.push_back(order);
			AF.push_back(order);
		}

		// compute the action value
		end.Start(MAX_TURNS, AP, AF, false, true);
		a.value = depth % 2 == 0 ? end.GetScore() : -end.GetScore();
		a.value += i;
	}

	// sort actions for alpha beta efficiency
	sort(actions.begin(), actions.end());
	return actions;
}
