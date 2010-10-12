#include "PlanetWars.h"
#include "Simulator.h"
#include "AlphaBeta.h"
#include "Logger.h"

#include <iostream>
#include <fstream>
#include <string>

#include <cstdlib>

#define VERSION "11.2"

void DoTurn(PlanetWars& pw, int turn, int plies, int thinkTime) {
	AlphaBeta ab(pw, thinkTime);
	std::vector<Fleet>& orders = ab.GetOrders(turn, plies);
	std::vector<Planet> AP     = pw.Planets();
	
	for (unsigned int i = 0; i < orders.size(); i++)
	{
		const Fleet& order = orders[i];
		const Planet& src = AP[order.SourcePlanet()];
		const Planet& dst = AP[order.DestinationPlanet()];
		ASSERT_MSG(order.NumShips() > 0 && src.Owner() == 1 && dst.PlanetID() != src.PlanetID(), order);
		pw.IssueOrder(order.SourcePlanet(), order.DestinationPlanet(), order.NumShips());
	}
}


// This is just the main game loop that takes care of communicating with the
// game engine for you. You don't have to understand or change the code below.
int main(int argc, char *argv[]) {
	int plies     = 2;
	int thinkTime = 1;
	std::string playerid("0");
	switch(argc)
	{
		case 2: {
			playerid  = argv[1];
		} break;
		case 3: {
			playerid  = argv[1];
			plies     = atoi(argv[2]);
		} break;
		case 4: {
			playerid  = argv[1];
			plies     = atoi(argv[2]);
			thinkTime = atoi(argv[3]);
		} break;
		default: {
		}
	}
	Logger logger(std::string(argv[0]) + "-E323-" + VERSION + "-" + playerid + ".txt");
	Logger::SetLogger(&logger);

	unsigned int turn = 0;
	std::string current_line;
	std::string map_data;
	LOG(argv[0]<<"-E323-v"<<VERSION<<" initialized");
	LOG("\tMAX DEPTH: "<<plies*2);
	LOG("\tMAX THINK: "<<thinkTime<<"\n");
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
				DoTurn(pw, turn, plies, thinkTime);
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
	return 0;
}
