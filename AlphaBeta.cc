#include "AlphaBeta.h"
#include "Logger.h"
#include "vec3.h"

#include <sstream>
#include <algorithm>
#include <cmath>

#define SIMULATION_STEPS 1 

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
		// at depth = 0, we don't want to swap owner, and for neutral planets
		// never swap owner
		owner = (depth > 1 && owner != 0) ? (owner % 2) + 1 : owner;
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
		owner = (depth > 1) ? (owner % 2) + 1 : owner;
		f.Owner(owner);

		if (f.Owner() == 1)
		{
			myNumShips += f.NumShips();
			MFIDX.push_back(i);
		}
		else
		{
			enemyNumShips += f.NumShips();
			EFIDX.push_back(i);
		}
	}
}

Simulator AlphaBeta::sim;
int       AlphaBeta::turn;
int       AlphaBeta::maxDepth;
int       AlphaBeta::branchIndex = -1;
int       AlphaBeta::allBranchIndices = -1;
int       AlphaBeta::bestBranchIndex = -1;

std::vector<Fleet>& AlphaBeta::GetOrders(int t, int plies) {
	time(&startTime);
	std::vector<Planet> AP = pw.Planets();
	std::vector<Fleet>  AF = pw.Fleets();
	nodesVisited = 0;
	bestScore    = std::numeric_limits<int>::min();
	turn         = t;

	maxDepth = plies*2;
	maxDepth = std::min<int>(maxDepth, (MAX_TURNS-turn)*2+2);

	GameState gs(0, AP, AF);
	std::vector<GameState> history;
	history.push_back(gs);
	Node origin(0, history, gs);

	Search(origin, 0, std::numeric_limits<int>::min(),
		std::numeric_limits<int>::max());

	std::stringstream time;
	time << " TIME TAKEN: " << diffTime;
	if (diffTime > thinkTime)
	{
		time << " BRANCH REACHED: " << branchIndex;
	}

	LOG("NODES VISITED: " 
		<< nodesVisited 
		<< " SCORE: " 
		<< bestScore
		<< " BEST BRANCH: "
		<< bestBranchIndex
		<< "/"
		<< allBranchIndices
		<< time.str());

	return bestOrders;
}

int AlphaBeta::Search(Node& node, int depth, int alpha, int beta) {
	time(&endTime); diffTime = difftime(endTime, startTime);
	if (diffTime > thinkTime)
	{
		return node.GetScore();
	}

	nodesVisited++;
	bool simulate = (depth > 0 && depth % 2 == 0);
	
	if (simulate)
	{
		node.ApplySimulation();
	}

	if (depth >= maxDepth || node.IsTerminal(simulate))
	{
		int score = node.GetScore();
		node.RestoreSimulation();
		return score;
	}

	Node child(depth+1, node.history, node.curr);
	std::vector<std::vector<Fleet> > actions = child.GetActions();
	for (unsigned int i = 0, n = actions.size(); i < n; i++)
	{
		child.AddOrders(actions[i]);
		alpha = std::max<int>(alpha, -Search(child, depth+1, -beta, -alpha));
		child.RemoveOrders();

		if (depth == 0)
		{
			branchIndex = i;
			if (alpha > bestScore)
			{
				bestBranchIndex = i;
				bestOrders = actions[i];
				bestScore = alpha;
			}
		}

		if (beta <= alpha)
		{
			break;
		}
	}

	allBranchIndices = actions.size();

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
		ASSERTD(order.Owner() == src.Owner());
		curr.AF.push_back(order);
	}
	for (unsigned int i = 0, n = curr.orders.size(); i < n; i++)
	{
		Fleet& order = curr.orders[i];
		Planet& src  = curr.AP[order.SourcePlanet()];
		ASSERTD(order.Owner() == src.Owner());
		src.RemoveShips(order.NumShips());
		curr.AF.push_back(order);
	}
	sim.Start(SIMULATION_STEPS, curr.AP, curr.AF);
	curr.myNumShips = sim.MyNumShips();
	curr.enemyNumShips = sim.EnemyNumShips();
}

void AlphaBeta::Node::RestoreSimulation() {
	curr = history.back();
}

int AlphaBeta::Node::GetScore() {
	int score = 0;
	if (curr.enemyNumShips <= 0)
	{
		score = std::numeric_limits<int>::max();
	}
	else
	if (curr.myNumShips <= 0)
	{
		score = std::numeric_limits<int>::min();
	}
	else
	{
		sim.Start(MAX_TURNS-turn-(depth/2)+1, curr.AP, curr.AF);
		score = sim.GetScore();
	}
	return score;
}

bool AlphaBeta::Node::IsTerminal(bool simulate) {
	bool isTerminal = false;
	if (simulate)
	{
		isTerminal = curr.myNumShips <= 0 || curr.enemyNumShips <= 0;
	}
	return isTerminal;
}

// Everything below this line is for gathering good actions
// globals
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
	Planet& t = gAP->at(gTarget);
	Planet& a = gAP->at(pidA);
	Planet& b = gAP->at(pidB);
	int distA = a.Distance(t);
	int distB = b.Distance(t);
	return distA < distB;
}

bool SortOnGrowthRate(const int pidA, const int pidB) {
	const Planet& a = gAP->at(pidA);
	const Planet& b = gAP->at(pidB);
	return a.GrowthRate() > b.GrowthRate();
}

void AcceptOrRestore(bool success, Planet& target, std::vector<Fleet>& orders,
			std::vector<Planet>& AP, std::vector<Fleet>& AF, std::vector<Fleet>& allOrdersForAnAction) {
	if (success)
	{
		for (unsigned int j = 0, m = orders.size(); j < m; j++)
		{
			Fleet& order = orders[j];
			ASSERT(order.NumShips() > 0);
			allOrdersForAnAction.push_back(order);
		}
	}
	else // restore previous state
	{
		AP[target.PlanetID()].Restore();
		for (unsigned int j = 0, m = orders.size(); j < m; j++)
		{
			Fleet& order = orders[j];
			AP[order.SourcePlanet()].Restore();
		}
		AF.erase(AF.begin()+AF.size()-orders.size(), AF.end());
	}
}

// This function contains all the smart stuff
std::vector<std::vector<Fleet> > AlphaBeta::Node::GetActions() {
	std::vector<std::vector<Fleet> > actions;
	std::vector<Planet> AP(curr.AP);
	std::vector<Fleet>  AF(curr.AF);
	gAP = &AP;
	int owner = (depth % 2) + 1;
	int turnsRemaining = MAX_TURNS-turn-(depth/2)+1;


	// ***************META STUFF HERE, ENEMY DISTANCE ETC************
	Simulator end, tmp;
	end.Start(turnsRemaining, AP, AF, false, true);
	vec3<double> myLoc(0.0,0.0,0.0);
	vec3<double> enemyLoc(0.0,0.0,0.0);
	std::vector<Fleet> allOrdersForAnAction;
	std::vector<int> TPIDX, NTPIDX;

	// Determine average enemy location
	for (unsigned int i = 0; i < curr.EPIDX.size(); i++)
	{
		Planet& planet = AP[curr.EPIDX[i]];
		ASSERTD(planet.Owner() == 2);
		enemyLoc += planet.Loc();
	}
	enemyLoc /= (curr.EPIDX.size() > 0) ? curr.EPIDX.size() : 1.0;

	int closestPlanetID = -1;
	int closestDist     = std::numeric_limits<int>::max();
	// Determine our closest planet and our average location
	for (unsigned int i = 0; i < curr.MPIDX.size(); i++)
	{
		Planet& planet = AP[curr.MPIDX[i]];
		const int pid  = planet.PlanetID();
		int dist = (planet.Loc() - enemyLoc).len2D();
		if (dist < closestDist)
		{
			closestPlanetID = pid;
			closestDist = dist;
		}
		myLoc += planet.Loc();
	}
	myLoc /= (curr.MPIDX.size() > 0) ? curr.MPIDX.size() : 1.0;

	// Split up planets that will be captured and that will not
	for (unsigned int i = 0; i < curr.MPIDX.size(); i++)
	{
		Planet& planet = AP[curr.MPIDX[i]];
		const int pid  = planet.PlanetID();
		
		ASSERTD(planet.Owner() == 1);
		if (end.IsMyPlanet(pid) || pid == closestPlanetID)
			NTPIDX.push_back(pid);
		else
			TPIDX.push_back(pid);
	}

	std::vector<int> skipNP;

	// ***************DEFAULT ORDERS HERE, THESE WILL BE ADDED TO EVERY ACTION***********
	// (1) Defense
	sort(TPIDX.begin(), TPIDX.end(), SortOnGrowthRate);
	for (unsigned int i = 0, n = TPIDX.size(); i < n; i++)
	{
		Planet& target = AP[TPIDX[i]];
		const int tid = target.PlanetID();
		std::vector<Fleet> orders;
		
		gTarget = tid;
		bool successfullDefend = false;
		sort(NTPIDX.begin(), NTPIDX.end(), SortOnDistanceToTarget);
		for (unsigned int j = 0, m = NTPIDX.size(); j < m; j++)
		{
			Planet& source = AP[NTPIDX[j]];
			if (source.NumShips() <= 0)
				continue;
			source.Backup();
			int sid = source.PlanetID();

			const int distance = source.Distance(target);
			sim.Start(distance, AP, AF, false, true);
			Planet& simSrc = sim.GetPlanet(tid);
			while (end.IsEnemyPlanet(tid) && source.NumShips() > 0)
			{
				Simulator::PlanetOwner& enemy = end.GetFirstEnemyOwner(tid);
				int numShips;

				if (distance < enemy.time)
				{
					numShips = enemy.force - (target.NumShips() + enemy.time*target.GrowthRate());
				}
				else
				if (distance > enemy.time)
				{
					numShips = simSrc.NumShips() + 1;
				}
				else
				{
					numShips = enemy.force;
				}

				// FIXME: This should not happen...
				if (numShips <= 0) numShips = 1;
				numShips = std::min<int>(source.NumShips(), numShips);
				orders.push_back(Fleet(1, numShips, sid, tid, distance, distance));
				AF.push_back(orders.back());
				source.NumShips(source.NumShips() - numShips);
				end.Start(turnsRemaining, AP, AF, false, true);
			}
			if (end.IsMyPlanet(tid))
			{
				ASSERTD(target.Owner() == 1);
				successfullDefend = true;
				break;
			}
		}
		AcceptOrRestore(successfullDefend, target, orders, AP, AF, allOrdersForAnAction);
	}

	// (2) Snipe
	for (unsigned int i = 0, n = curr.NPIDX.size(); i < n; i++)
	{
		Planet& target = AP[curr.NPIDX[i]];
		int tid = target.PlanetID();

		// This neutral planet will be captured by the enemy
		if (end.IsEnemyPlanet(tid))
		{
			std::vector<Fleet> orders;
			
			gTarget = tid;
			Simulator::PlanetOwner& enemy = end.GetFirstEnemyOwner(tid);
			bool successfullAttack = false;
			sort(NTPIDX.begin(), NTPIDX.end(), SortOnDistanceToTarget);
			for (unsigned int j = 0, m = NTPIDX.size(); j < m; j++)
			{
				Planet& source = AP[NTPIDX[j]];
				if (source.NumShips() <= 0)
					continue;

				source.Backup();
				int sid = source.PlanetID();
				const int distance = source.Distance(target);

				// if we are too close or too far don't attack
				if ((distance - enemy.time) > 2)
					break;

				if (distance < enemy.time)
					continue;

				sim.Start(distance, AP, AF, false, true);
				const int fleetsRequired = sim.GetPlanet(tid).NumShips() + 1;
				const int fleetSize = std::min<int>(source.NumShips(), fleetsRequired);
				orders.push_back(Fleet(1, fleetSize, sid, tid, distance, distance));
				AF.push_back(orders.back());
				source.NumShips(source.NumShips() - fleetSize);

				// See if given all previous orders, this planet will be ours
				sim.Start(distance, AP, AF, false, true);
				if (sim.IsMyPlanet(tid))
				{
					skipNP.push_back(tid);
					ASSERTD(target.Owner() == 0);
					successfullAttack = true;
					break;
				}
			}
			AcceptOrRestore(successfullAttack, target, orders, AP, AF, allOrdersForAnAction);
		}
	}

	// (3) Enforce
	if (closestPlanetID != -1 && TPIDX.empty())
	{
		for (unsigned int i = 0; i < NTPIDX.size(); i++)
		{
			Planet& source = AP[NTPIDX[i]];
			if (source.PlanetID() == closestPlanetID || source.NumShips() <= 0)
				continue;
			const int distance = source.Distance(AP[closestPlanetID]);
			const int fleetSize = source.NumShips();
			Fleet order(1, fleetSize, source.PlanetID(), closestPlanetID, distance, distance);
			source.NumShips(source.NumShips() - fleetSize);
			allOrdersForAnAction.push_back(order);
			AF.push_back(order);
		}
	}


	unsigned int allOrdersForAnActionBeginSize = allOrdersForAnAction.size();
	unsigned int afBeginSize = AF.size();
	
	// *************ALPHA BETA ORDERS HERE, THESE ARE DEFINED PER ACTION************
	// TODO: Determine GOOD target planets
	end.Start(turnsRemaining, AP, AF, false, true);
	std::vector<int> ABT; // Alpha Beta targets to choose from

	for (unsigned int i = 0, n = curr.NMPIDX.size(); i < n; i++)
	{
		Planet& target = AP[curr.NMPIDX[i]];
		int tid = target.PlanetID();

		if (end.IsMyPlanet(tid) || find(skipNP.begin(), skipNP.end(), tid) != skipNP.end())
		{
			continue;
		}

		// This enables a "creeping" behavior
		const vec3<double>& loc = target.Loc();
		double myDist = (loc - myLoc).len2D();
		double enemyDist = (loc - enemyLoc).len2D();
		if (enemyDist - myDist > 1.0 || target.Owner() == 2 || curr.myNumShips > curr.enemyNumShips*2)
		{
			ABT.push_back(tid);
		}
	}

	sort(ABT.begin(), ABT.end(), SortOnGrowthShipRatio);
	for (unsigned int i = 0, n = ABT.size(); i < n; i++)
	{
		Planet& target = AP[ABT[i]];
		int tid = target.PlanetID();
		
		gTarget = tid;
		ASSERTD(curr.AP[tid].Owner() == 2 || curr.AP[tid].Owner() == 0);
		sort(NTPIDX.begin(), NTPIDX.end(), SortOnDistanceToTarget);
		bool successfullAttack = false;
		for (unsigned int k = 0, m = NTPIDX.size(); k < m; k++)
		{
			Planet& source = AP[NTPIDX[k]];
			int sid = source.PlanetID();
			ASSERTD(curr.AP[sid].Owner() == 1);
			if (source.NumShips() <= 0)
				continue;
			source.Backup();

			const int distance = source.Distance(target);
			sim.Start(distance, AP, AF, false, true);

			const int fleetsRequired = sim.GetPlanet(tid).NumShips() + 1;
			const int fleetSize = std::min<int>(source.NumShips(), fleetsRequired);
			ASSERTD(fleetSize > 0);

			source.NumShips(source.NumShips()-fleetSize);
			Fleet order(1, fleetSize, sid, tid, distance, distance);
			allOrdersForAnAction.push_back(order);
			AF.push_back(order);

			sim.Start(distance, AP, AF, false, true);
			if (sim.IsMyPlanet(tid))
			{
				successfullAttack = true;
				break;
			}
		}

		if (successfullAttack)
		{
			actions.push_back(allOrdersForAnAction);
		}

		// remove orders from this specific action again
		allOrdersForAnAction.erase(
			allOrdersForAnAction.begin() + allOrdersForAnActionBeginSize, 
			allOrdersForAnAction.end()
		);
		ASSERTD(allOrdersForAnAction.size() == allOrdersForAnActionBeginSize);

		// Restore souce planets we altered and remove orders from ALL FLEETS again
		for (unsigned int k = afBeginSize; k < AF.size(); k++)
		{
			AP[AF[k].SourcePlanet()].Restore();
		}

		AF.erase(AF.begin() + afBeginSize, AF.end());
		ASSERTD(AF.size() == afBeginSize);
	}

	// Add null action
	actions.push_back(allOrdersForAnAction);

	// set owner to real ingame owner
	for (unsigned int i = 0; i < actions.size(); i++)
	{
		for (unsigned int j = 0; j < actions[i].size(); j++)
		{
			actions[i][j].Owner(owner);
		}
	}

	return actions;
}
