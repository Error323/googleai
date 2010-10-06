#include "AlphaBeta.h"

#include <limits>

#define MAX_ROUNDS 200

std::vector<Fleet> AlphaBeta::GetOrders(int turn) {
	std::vector<Planet> AP = pw.Planets();
	std::vector<Fleet>  AF = pw.Fleets();
	turnsRemaining = MAX_ROUNDS-turn;
	Node origin(AP, AF);

	Search(origin, 0, std::numeric_limits<int>::min(),
		std::numeric_limits<int>::max());

	return bestOrders;
}

int AlphaBeta::Search(Node& node, int depth, int alpha, int beta)
{
	if (depth > 0 && depth % 2 == 0)
	{
		node.Simulate();
	}

	if (depth == turnsRemaining || node.IsTerminal())
	{
		return node.GetScore(depth % 2 == 0);
	}
	
	std::vector<std::vector<Fleet> > actions = node.GetActions();
	Node child(node.Planets(), node.Fleets());
	for (unsigned int i = 0, n = actions.size(); i < n; i++)
	{
		child.ApplyAction(actions[i]);
		alpha = std::max<int>(alpha, -Search(child, depth+1, -beta, -alpha));
		child.RestoreAction(actions[i]);

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
				bestOrders.push_back(orders[i]);
			bestScore = alpha;
		}
	}
	return alpha;
}

void AlphaBeta::Node::ApplyAction(std::vector<Fleet>& action) {
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
}

void AlphaBeta::Node::RestoreAction(std::vector<Fleet>& action) {
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
}

std::vector<std::vector<Fleet> > AlphaBeta::Node::GetActions() {
	return std::vector<std::vector<Fleet> >();
}

int AlphaBeta::Node::GetScore(bool isMe) {
	int myNumShips    = 0;
	int enemyNumShips = 0;
	for (unsigned int i = 0, n = AP.size(); i < n; i++)
	{
		if (AP[i].Owner() == 1)
			myNumShips += AP[i].NumShips();
		else
			enemyNumShips += AP[i].NumShips();
	}
	for (unsigned int i = 0, n = AF.size(); i < n; i++)
	{
		if (AF[i].Owner() == 1)
			myNumShips += AF[i].NumShips();
		else
			enemyNumShips += AF[i].NumShips();
	}
	int score = myNumShips - enemyNumShips;
	return isMe ? score : -score;
}

bool AlphaBeta::Node::IsTerminal() {
	int myNumShips    = 0;
	int enemyNumShips = 0;
	for (unsigned int i = 0, n = AP.size(); i < n; i++)
	{
		if (AP[i].Owner() == 1)
			myNumShips += AP[i].NumShips();
		else
			enemyNumShips += AP[i].NumShips();
	}
	for (unsigned int i = 0, n = AF.size(); i < n; i++)
	{
		if (AF[i].Owner() == 1)
			myNumShips += AF[i].NumShips();
		else
			enemyNumShips += AF[i].NumShips();
	}
	return myNumShips <= 0 || enemyNumShips <= 0;
}

void AlphaBeta::Node::Simulate() {
	Simulator sim;
	sim.Start(1, AP, AF);
}
