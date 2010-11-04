#include "PlanetWars.h"
#include "Logger.h"
#include "Timer.h"
#include "AlphaBeta.h"

#include <string>
#include <iostream>

#ifdef DEBUG
	#include <sys/types.h>
	#include <unistd.h>
	#include <signal.h>
#endif

int turn = 0;

void DoTurn(PlanetWars& pw, int depth) {
	std::vector<Planet> AP = pw.Planets();
	std::vector<Fleet>  AF = pw.Fleets();

	AlphaBeta alphabeta(pw, 10.0);

	std::vector<Fleet> orders = alphabeta.GetOrders(turn, depth);

	for (unsigned int i = 0, n = orders.size(); i < n; i++)
	{
		Fleet& order = orders[i];
		const int sid = order.SourcePlanet();
		const int numShips = order.NumShips();
		const int tid = order.DestinationPlanet();
		Planet& source = AP[sid];
		source.RemoveShips(numShips);

		ASSERT_MSG(numShips > 0, order);
		ASSERT_MSG(source.NumShips() >= 0, order);
		ASSERT_MSG(AP[sid].Owner() == 1, order);
		ASSERT_MSG(tid >= 0 && tid != sid, order);
		pw.IssueOrder(sid, tid, numShips);
	}
}

#ifdef DEBUG
void SigHandler(int signum) {
	Logger logger("crash.txt");
	signal(signum, SIG_DFL);
	kill(getpid(), signum);
}
#endif

// This is just the main game loop that takes care of communicating with the
// game engine for you. You don't have to understand or change the code below.
int main(int argc, char *argv[]) {
	int depth = 1;
	if (argc == 2)
		depth = atoi(argv[1]);

	#ifdef DEBUG
	time_t t;
	time(&t);
	struct tm* lt = localtime(&t);
	char buf[1024] = {0};
	snprintf(
		buf,
		1024 - 1,
		"%s-%02d-%02d-%04d_%02d%02d.txt",
		argv[0],
		0,0,0,0,0
		/*
		lt->tm_mon + 1,
		lt->tm_mday,
		lt->tm_year + 1900,
		lt->tm_hour,
		lt->tm_min
		*/
	);
	signal(SIGSEGV, SigHandler);
	Logger logger(buf);
	Logger::SetLogger(&logger);
	LOG(argv[0]<<" initialized");
	#endif

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
				#ifdef DEBUG
				Timer t;
				t.Tick();
				#endif
				DoTurn(pw, depth);
				#ifdef DEBUG
				t.Tock();
				LOG("TIME: "<<t.Time()<<"s");
				#endif
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
