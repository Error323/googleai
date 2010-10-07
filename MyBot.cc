#include "PlanetWars.h"
#include "Simulator.h"
#include "AlphaBeta.h"

#include <iostream>
#include <fstream>

#define VERSION "11.0"

void DoTurn(PlanetWars& pw, int turn, std::ofstream& file) {
	AlphaBeta ab(pw, file);
	std::vector<Fleet>& orders = ab.GetOrders(turn);
	std::vector<Planet> AP     = pw.Planets();
	
	for (unsigned int i = 0; i < orders.size(); i++)
	{
		const Fleet& order = orders[i];
		const Planet& src = AP[order.SourcePlanet()];
		const Planet& dst = AP[order.DestinationPlanet()];
		if (order.NumShips() > 0 && src.Owner() == 1 && dst.PlanetID() != src.PlanetID())
		{
			pw.IssueOrder(order.SourcePlanet(), 
				order.DestinationPlanet(), order.NumShips());
		}
		else
		{
			LOG("ERROR: invalid order - " << order);
			LOG("\tSource: " << src);
			LOG("\tDest  : " << dst);
		}
	}
}


// This is just the main game loop that takes care of communicating with the
// game engine for you. You don't have to understand or change the code below.
int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;

	unsigned int turn = 0;
	std::ofstream file;
	std::string filename("-Error323-v");
	filename = argv[0] + filename + VERSION + ".txt";
	file.open(filename.c_str(), std::ios::in|std::ios::trunc);
	std::string current_line;
	std::string map_data;
	while (true) {
		int c = std::cin.get();
		current_line += (char)c;
		if (c == '\n') 
		{
			if (current_line.length() >= 2 && current_line.substr(0, 2) == "go") 
			{
				PlanetWars pw(map_data);
				map_data = "";
				LOG("turn: " << turn);
				//LOG(pw.ToString());
				DoTurn(pw, turn, file);
				LOG("\n--------------------------------------------------------------------------------\n");
				turn++;
				pw.FinishTurn();
			} 
			else 
			{
				map_data += current_line;
			}
			current_line = "";
		}
	}
	if (file.good())
	{
		file.close();
	}
	return 0;
}
