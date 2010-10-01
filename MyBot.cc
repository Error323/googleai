#include "PlanetWars.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <cassert>

#define DEBUG
#ifdef DEBUG
#define LOG(msg) { file << msg << std::endl; file.flush(); }
#else
#define LOG(msg)
#endif

bool cmp(const Planet& a, const Planet& b) {
	if (a.GrowthRate() == b.GrowthRate())
		return a.NumShips() > b.NumShips();
	else
		return a.GrowthRate() > b.GrowthRate();
}

void DoTurn(const PlanetWars& pw, std::ofstream& file) {
	// The overall idea is to maximize growth every turn
	// (1) determine number of our ships
	unsigned int numShips = 0;
	std::vector<Planet> myPlanets, notMyPlanets;
	std::vector<Planet> planets = pw.Planets();
	sort(planets.begin(), planets.end(), cmp);
	for (unsigned int i = 0, n = planets.size(); i < n; i++)
	{
		const Planet& p = planets[i];
		if (p.Owner() == 1)
		{
			numShips += p.NumShips() - 1;
			myPlanets.push_back(p);
		}
		else
		{
			notMyPlanets.push_back(p);
		}
	}

	if (numShips < 1)
	{
		return;
	}

	// (2) determine best planets for attack
	unsigned int bestGrowthRate = 0;
	std::vector<unsigned int> skip;
	std::vector<Planet> bestAttackable;
	while (skip.size() < notMyPlanets.size())
	{
		int tmpNumShips = numShips;
		unsigned int highestNumShips = 0;
		unsigned int skipIndex = 0;
		unsigned int growthRate = 0;
		std::vector<Planet> attackable;

		for (unsigned int i = 0, n = notMyPlanets.size(); i < n; i++)
		{
			if (find(skip.begin(), skip.end(), i) != skip.end())
				continue;

			const Planet& p = notMyPlanets[i];

			tmpNumShips -= p.NumShips() + 1;
			if (tmpNumShips < 0)
			{
				break;
			}

			growthRate += p.GrowthRate();
			assert(growthRate <= numShips);
			attackable.push_back(p);

			if (highestNumShips < p.NumShips())
			{
				highestNumShips = p.NumShips();
				skipIndex = i;
			}
		}

		if (growthRate > bestGrowthRate)
		{
			bestGrowthRate = growthRate;
			bestAttackable = attackable;
		}

		skip.push_back(skipIndex);
	}

	// (3) greedily determine shortest routes for attackable planets
	for (unsigned int i = 0, n = bestAttackable.size(); i < n; i++)
	{
		const Planet& target = bestAttackable[i];
		Planet& bestSource = myPlanets[0];
		int bestDistance = pw.Distance(bestSource.PlanetID(), target.PlanetID());
		
		for (unsigned int j = 1, m = myPlanets.size(); j < m; j++)
		{
			const Planet& source = myPlanets[j];
			if (source.NumShips() <= 1)
				continue;

			int distance = pw.Distance(source.PlanetID(), target.PlanetID());
			if (distance < bestDistance)
			{
				bestDistance = distance;
				bestSource = source;
			}
		}

		int fleetSize = std::min<int>(bestSource.NumShips() - 1, target.NumShips() + 1);
		bestSource.NumShips(bestSource.NumShips() - fleetSize);
		pw.IssueOrder(bestSource.PlanetID(), target.PlanetID(), fleetSize);
	}
}

// This is just the main game loop that takes care of communicating with the
// game engine for you. You don't have to understand or change the code below.
int main(int argc, char *argv[]) {
  std::ofstream file;
#ifdef DEBUG
  file.open("mybot.txt", std::ios::in|std::ios::trunc);
#endif
  std::string current_line;
  std::string map_data;
  while (true) {
    int c = std::cin.get();
    current_line += (char)c;
    if (c == '\n') {
      if (current_line.length() >= 2 && current_line.substr(0, 2) == "go") {
        PlanetWars pw(map_data);
        map_data = "";
        DoTurn(pw, file);
	pw.FinishTurn();
      } else {
        map_data += current_line;
      }
      current_line = "";
    }
  }
  if (file.good())
	file.close();
  return 0;
}
