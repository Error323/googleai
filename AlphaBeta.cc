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

namespace bot {
	#include "Helper.inl"
}

std::vector<Fleet>& AlphaBeta::GetOrders(int t, int plies) {
	timer.Tick();
	std::vector<Planet> AP = pw.Planets();
	std::vector<Fleet>  AF = pw.Fleets();
	nodesVisited = 0;
	bestScore    = std::numeric_limits<int>::min();
	turn         = t;

	maxDepth = plies*2;
	maxDepth = std::min<int>(maxDepth, (MAX_TURNS-turn)*2);
	if (turn == 0)
		maxDepth = 2;

	nodeStack.resize(plies);

	Node origin(AP, AF);
	Action a;
	std::pair<Action,Action> actionPair(a, a);
	sim.Start(0, AP, AF);
	end.Start(MAX_TURNS-turn, AP, AF, false, true);
	int curScore = sim.GetScore();
	int alpha = Search(origin, actionPair, 0, std::numeric_limits<int>::min(),
		std::numeric_limits<int>::max());

	LOG(
		"TURN: "<<turn<<
		"\tVISITED: "<<nodesVisited<<
		"\tALPHA: "<<alpha<<
		"\tSCORE: "<<curScore
	);
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

/**
 * The heuristic score is defined as follows:
 *         /                 f_e(p) + 1      \     /                 f_m(p) + 1      \
 * h(n) = |Sum_{p in P_w(n)} ---------- * g(p)| - |Sum_{p in P_l(n)} ---------- * g(p)|,
 *         \                 f_m(p) + 1      /     \                 f_e(p) + 1      /
 *
 * Where P_w(n) is the set of planets won at node n, P_l is the set of
 * planets lost at node n, f_m(p) are the amount of fleets on my side send
 * to this planet, f_e(p) are the amount of enemy fleets send to this
 * planet and g(p) is the planet's growth rate.
 */
int AlphaBeta::GetScore(Node& node, int depth) {
	if (sim.EnemyNumShips() <= 0)
		return std::numeric_limits<int>::max();
	
	if (sim.MyNumShips() <= 0)
		return std::numeric_limits<int>::min();
	
	std::vector<int> MFIDX; // my fleets
	std::vector<int> EFIDX; // enemy fleets

	sim.Start(MAX_TURNS-turn-depth/2, node.AP, node.AF, false, true);

	std::vector<int> PIDX; // affected planets from this action
	for (int i = 0, n = node.AF.size(); i < n; i++)
	{
		Fleet& f = node.AF[i];
		if (f.Owner() == 1)
			MFIDX.push_back(i);
		else
			EFIDX.push_back(i);

		if (f.TotalTripLength() - f.TurnsRemaining() <= depth/2)
		{
			int sid = f.SourcePlanet();
			int tid = f.DestinationPlanet();
			if (find(PIDX.begin(), PIDX.end(), sid) == PIDX.end())
				PIDX.push_back(sid);
			if (find(PIDX.begin(), PIDX.end(), tid) == PIDX.end())
				PIDX.push_back(tid);
		}
	}

	bot::gAP = &node.AP;
	bot::gAF = &node.AF;

	double wins   = 0.0;
	double losses = 0.0;
	for (int i = 0, n = PIDX.size(); i < n; i++)
	{
		int pid = PIDX[i];
		Planet& pNow = (turn < MAX_TURNS - 1) ? nodeStack[0].AP[pid] : node.AP[pid];
		Planet& pSim = sim.GetPlanet(pid);
		Planet& pEnd = end.GetPlanet(pid);
		int pGrow = pNow.GrowthRate();

		bool defense   = pNow.Owner() == 1 && pEnd.Owner() >  1 && pSim.Owner() == 1;
		bool expansion = pNow.Owner() == 0 && pEnd.Owner() != 1 && pSim.Owner() == 1;
		bool attacking = pNow.Owner() >  1 && pEnd.Owner() >  1 && pSim.Owner() == 1;

		bool sniped   = sim.IsSniped(pid) || end.IsSniped(pid);
		bool attacked = pNow.Owner() == 1 && pEnd.Owner() > 1 && pSim.Owner() > 1;

		double fe  = bot::GetIncommingFleets(pid, EFIDX) + 1.0;
		double fm  = bot::GetIncommingFleets(pid, MFIDX) + 1.0;
		if (defense || expansion || attacking)
		{
			double win = (fe / fm) * pGrow;
			if (defense)   { wins += win * DEFEND; }
			else
			if (expansion) { wins += win * EXPAND; }
			else
			if (attacking) { wins += win * ATTACK; }
		}
		else
		if (sniped || attacked)
		{
			double loss = (fm / fe) * pGrow;
			if (sniped)   losses += loss * SNIPE;
			else
			if (attacked) losses += loss * DEFEND;
		}
		else
		{
			if (pSim.Owner() == 1)
				wins += (fe / fm) * pGrow + pNow.NumShips();
			else
			if (pSim.Owner() > 1)
				losses += (fm / fe) * pGrow + pNow.NumShips();
		}
	}
	return round((wins - losses)) + sim.GetScore();
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

// |T+1|^|S|, where T = target planets and S = source planets
void AlphaBeta::Node::GenerateActions(unsigned int depth, std::vector<int>& SPIDX,
		std::vector<int>& TPIDX, std::vector<AlphaBeta::Action>& actions) {
	
	if (depth == SPIDX.size())
	{
		std::vector<Fleet> orders;
		for (int i = 0, n = SPIDX.size(); i < n; i++)
		{
			const int sid  = SPIDX[i];
			const int tid  = planetStack[i];

			// we want this as an "empty" order
			if (sid == tid)
				continue;

			// the specifics will be filled in later
			Fleet order(1, -1, sid, tid, -1, -1);
			orders.push_back(order);
		}
		actions.push_back(Action(orders));
		return;
	}

	for (int i = 0, n = TPIDX.size(); i < n; i++)
	{
		planetStack.push_back(TPIDX[i]);
		GenerateActions(depth + 1, SPIDX, TPIDX, actions);
		planetStack.pop_back();
	}
}

std::vector<std::vector<AlphaBeta::Action> > AlphaBeta::CartesianProduct(unsigned int depth, std::vector<std::vector<AlphaBeta::Action> >& sets) {
	std::vector<std::vector<Action> > product;

	if (depth == sets.size())
	{
		product.push_back(std::vector<Action>());
	}
	else
	{
		for (int i = 0, n = sets[depth].size(); i < n; i++)
		{
			std::vector<std::vector<Action> > S = CartesianProduct(depth + 1, sets);
			for (int j = 0, m = S.size(); j < m; j++)
			{
				S[j].push_back(sets[depth][i]);
				product.push_back(S[j]);
			}
		}
	}
	return product;
}

/**
 * The overall idea is this:
 *   for each frontline planet p in F we generate an action set with targets T
 *   containing the following wrt p:
 *     - The best valued neutral planet
 *     - The best enemy target
 *     - Itself (a null-action)
 *     - Targetted planets in distance
 *
 *   This generates f = |F| action sets A = {A_1,...,A_f}. An action is defined
 *   as a cartesian product A_1 x ... x A_f = {(a_1,...,a_f) : a_i in A_i}.
 *   Giving a total of |A_1| * ... * |A_f| actions per turn.
 *
 *   All planets that belong to us and are not under attack and are not
 *   part of the frontline will be feeders or staging planets
 *
 */
std::vector<AlphaBeta::Action> AlphaBeta::GetActions(Node& node, int depth) {
	std::vector<Planet> ap(node.AP);
	std::vector<Fleet>  af(node.AF);
	int turnsRemaining = MAX_TURNS-turn-depth/2;

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
	std::vector<int> NTPIDX; // not targetted planets belonging to us
	std::vector<int> TAPIDX; // targetted planets belonging to us
	std::vector<int> EPIDX;  // enemy planets
	std::vector<int> EFIDX;  // enemy fleets
	std::vector<int> MFIDX;  // my fleets
	Simulator end;
	end.Start(turnsRemaining, ap, af, false, true);

	for (int i = 0, n = ap.size(); i < n; i++)
	{
		Planet& p = ap[i];
		switch (p.Owner())
		{
			case 0: {
				if (p.GrowthRate() == 0)
					continue;

				if (end.IsSniped(i))
					TAPIDX.push_back(i);
				else if (!end.IsMyPlanet(i))
					NPIDX.push_back(i);
			}
			break;

			case 1: {
				if (end.IsEnemyPlanet(i))
					TAPIDX.push_back(i);
				else
					NTPIDX.push_back(i);
			}
			break;

			default: {
				EPIDX.push_back(i); 
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
	Map map(ap);                 // for the frontline etc
	std::vector<Action> actions; // the actions for this ply

	// turn 0 is a special case in which we will only capture neutrals
	if (turn == 0 && depth <= 1)
	{
		std::vector<Fleet> orders;
		std::vector<Planet>& AP = ap;
		std::vector<Fleet>&  AF = af;
		Planet& source = AP[NTPIDX.front()];
		const int sid = source.PlanetID();
		int numShipsToSpare = 0;
		int totalNumShipsToSpare = 0;

		// 1. Determine ships to spare
		const int eid = map.GetClosestPlanetIdx(source.Loc(), EPIDX);
		Planet& e = AP[eid];
		const int dist = source.Distance(e);
		int numShips = source.NumShips() - e.NumShips() -
			bot::GetIncommingFleets(sid, EFIDX) + dist*source.GrowthRate();

		numShips = std::min<int>(numShips, source.NumShips());
		if (numShips > 0)
		{
			numShipsToSpare = numShips;
			totalNumShipsToSpare += numShips;
		}

		// 2. Filter out candidates wrt numships and enemy
		std::vector<int> candidates;
		for (unsigned int i = 0, n = NPIDX.size(); i < n; i++)
		{
			Planet& target = AP[NPIDX[i]];
			const int eid = map.GetClosestPlanetIdx(target.Loc(), EPIDX);
			Planet& enemy = AP[eid];
			const int edist2target = target.Distance(enemy);
			const int mdist2target = target.Distance(source);
			const bool closer = mdist2target <= edist2target;
			const bool defend = source.NumShips() - bot::GetIncommingFleets(source.PlanetID(), EFIDX) > 0;
			if (closer && defend)
			{
				candidates.push_back(target.PlanetID());
				continue;
			}
		}

		// 3. Apply binary knapsack algorithm to select the best candidates
		std::vector<int> w; std::vector<double> v;
		for (unsigned int i = 0, n = candidates.size(); i < n; i++)
		{
			Planet& candidate = AP[candidates[i]];
			w.push_back(candidate.NumShips() + 1);
			const int dist = (source.Loc() - candidate.Loc()).len2D();
			double value = bot::GetValue(candidate, dist);
			v.push_back(value);
		}

		if (!candidates.empty())
		{
			KnapSack ks(w, v, totalNumShipsToSpare);
			std::vector<int> I = ks.Indices();
			for (unsigned int i = 0, n = I.size(); i < n; i++)
			{
				Planet& target = AP[candidates[I[i]]];
				const int tid = target.PlanetID();
				const int dist = target.Distance(source);
				int numShips = std::min<int>(target.NumShips() + 1, numShipsToSpare);
				numShips = std::min<int>(numShips, source.NumShips() - bot::GetIncommingFleets(sid, EFIDX));
				if (numShips <= 0 || source.NumShips() <= 0)
					continue;

				sim.Start(dist, AP, AF, false, true);
				numShips = std::min<int>(sim.GetPlanet(tid).NumShips() + 1, numShips);
				numShipsToSpare -= numShips;
				Fleet order(1, numShips, sid, tid, dist, dist);
				source.RemoveShips(numShips);
				orders.push_back(order);
			}
		}

		// create actions sending the remaining source-fleets to a planet
		if (source.NumShips() > 0)
		{
			for (int i = 0, n = orders.size(); i < n; i++)
			{
				Action action(orders);
				const int tid = orders[i].DestinationPlanet();
				const int dist = AP[tid].Distance(source);
				Fleet order(1, source.NumShips(), sid, tid, dist, dist);
				action.orders.push_back(order);
				actions.push_back(action);
			}
		}
		actions.push_back(Action(orders));
		return actions;
	}

	std::vector<int> FLPIDX;     // frontline planets
	std::vector<int> SPIDX;      // sources
	std::vector<int> TPIDX;      // targets
	FLPIDX = map.GetFrontLine();

	std::vector<std::vector<Action> > A; // A = {A_1,...,A_f}
	for (int i = 0, n = FLPIDX.size(); i < n; i++)
	{
		Planet& front = ap[FLPIDX[i]];
		const int sid = front.PlanetID();
		if (find(TAPIDX.begin(), TAPIDX.end(), sid) != TAPIDX.end())
			continue;

		SPIDX.push_back(sid);
		TPIDX.push_back(sid);

		A.push_back(std::vector<Action>());

		// select best neutral planet when we are behind or equal
		if (end.GetScore() <= 0)
		{
			std::priority_queue<bot::NPV> neutrals;
			for (int j = 0, m = NPIDX.size(); j < m; j++)
			{
				Planet& neutral = ap[NPIDX[j]];
				const int tid = neutral.PlanetID();
				const int dist = front.Distance(neutral);
				const int eid = map.GetClosestPlanetIdx(neutral.Loc(), EPIDX);
				ASSERT(eid != -1);
				Planet& enemy = ap[eid];
				const int edist2neutral = neutral.Distance(enemy);
				const int mdist2neutral = neutral.Distance(front);
				const int dist2front = front.Distance(enemy);
				const bool closer = mdist2neutral < edist2neutral;
				const bool defend = front.NumShips() - enemy.NumShips() +
					front.GrowthRate()*dist2front > neutral.NumShips();

				if (closer && defend)
				{
					double value = bot::GetValue(neutral, dist);
					neutrals.push(bot::NPV(tid, value));
				}
			}
			for (int j = 0; j < 1; j++)
			{
				if (!neutrals.empty())
				{
					TPIDX.push_back(neutrals.top().id);
					neutrals.pop();
				}
				else
					break;
			}
		}

		// select best enemy target
		std::priority_queue<bot::EPV> enemies;
		for (int j = 0, m = EPIDX.size(); j < m; j++)
		{
			Planet& enemy = ap[EPIDX[j]];
			const int tid = enemy.PlanetID();
			const int dist = front.Distance(enemy);
			int strength = bot::GetStrength(tid, dist, EPIDX, EFIDX);
			enemies.push(bot::EPV(tid, strength));
		}
		for (int j = 0; j < 1; j++)
		{
			if (!enemies.empty())
			{
				TPIDX.push_back(enemies.top().id);
				enemies.pop();
			}
			else
				break;
		}

		// select targetted planets in distance
		for (int j = 0, m = TAPIDX.size(); j < m; j++)
		{
			Planet& target = ap[TAPIDX[j]];
			const int tid = target.PlanetID();
			const int dist = front.Distance(target);
			Simulator::PlanetOwner& enemy = end.GetFirstEnemyOwner(tid);
			if (dist <= enemy.time)
				TPIDX.push_back(tid);
		}

		// now generate the actions for this frontline planet
		node.GenerateActions(0, SPIDX, TPIDX, A.back());
		
		TPIDX.clear(); // clear the target buffer
		SPIDX.clear(); // clear the source buffer
	}

	// compute the cartesian product of A_1 x A_2 x...x A_F
	std::vector<std::vector<Action> > tuples = CartesianProduct(0, A);

	// create the actions
	for (int i = 0, n = tuples.size(); i < n; i++)
	{
		for (int j = 0, m = tuples[i].size(); j < m; j++)
		{
			Action action;
			std::vector<Planet> AP(ap);
			std::vector<Fleet>  AF(af);
			bot::gAP = &AP;
			bot::gAF = &AF;
			Action& a = tuples[i][j]; // (a1,...,af) : ai in A_i
			for (int k = 0, l = a.orders.size(); k < l; k++)
			{
				const int sid = a.orders[k].SourcePlanet();
				const int tid = a.orders[k].DestinationPlanet();
				Planet& source = AP[sid];
				Planet& target = AP[tid];
				ASSERTD(source.Owner() == 1);
				const int dist = source.Distance(target);
				int numShips = source.NumShips() - bot::GetIncommingFleets(sid, EFIDX);
				switch (target.Owner())
				{
					// capture neutral planet
					case 0: {
						sim.Start(dist, AP, AF, false, true);
						numShips = std::min<int>(numShips, sim.GetPlanet(tid).NumShips() + 1);
					} break;

					// defend our planet
					case 1: {
						sim.Start(turnsRemaining, AP, AF, false, true);
						if (sim.IsEnemyPlanet(tid))
						{
							Simulator::PlanetOwner& enemy = sim.GetFirstEnemyOwner(tid);
							sim.Start(enemy.time, AP, AF, false, true);
							numShips = std::min<int>(numShips, sim.GetPlanet(tid).NumShips());
						}
					} break;

					// attack enemy planet
					default: {
						sim.Start(dist, AP, AF, false, true);
						int numShipsRequired = sim.GetPlanet(tid).NumShips() + 1;
						if (numShips < numShipsRequired)
							numShips = 0;
					} break;
				}
				if (numShips > 0)
				{
					source.RemoveShips(numShips);
					Fleet order(1, numShips, sid, tid, dist, dist);
					AF.push_back(order);
					action.orders.push_back(order);
				}
			}

			// Send ships to our closest frontline planet via hubs
			std::vector<Planet> AFP(AP); // all future planets
			std::vector<Fleet>  AFF(AF); // all future fleets
			end.Start(MAX_TURNS-turn, AFP, AFF);
			Map fmap(AFP); // future map, to compute future frontline
			std::vector<int>& FFLPIDX = fmap.GetFrontLine();

			// Send ships to our closest frontline planet via hubs
			for (unsigned int i = 0, n = NTPIDX.size(); i < n; i++)
			{
				Planet& source = AP[NTPIDX[i]];
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
				action.orders.push_back(order);
			}

			// compute action value and add to our actions for this ply
			if (!action.orders.empty()) actions.push_back(action);
		}
	}

	// sort actions for alpha beta efficiency
	return actions;
}
