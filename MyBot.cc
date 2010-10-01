#include "PlanetWars.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <limits>
#include <cassert>

#define LOG(msg)                  \
	if (file.good())              \
	{                             \
		file << msg << std::endl; \
	}

unsigned int round = 0;

PlanetWars* planetWars;
Planet* globalTarget = NULL;

bool cmp1(const Planet& a, const Planet& b) {
	float x = a.GrowthRate() / (a.NumShips() + 1.0f);
	float y = b.GrowthRate() / (b.NumShips() + 1.0f);
	return x > y;
}

bool cmp2(const Planet* a, const Planet* b) {
	float distA = planetWars->Distance(a->PlanetID(), globalTarget->PlanetID());
	float distB = planetWars->Distance(b->PlanetID(), globalTarget->PlanetID());
	return distA < distB;
}

void DoTurn(PlanetWars& pw, std::ofstream& file) {
	LOG(round);
	planetWars = &pw;

	// The overall idea is to maximize growth every turn
	// (1) determine number of our ships and divide planets
	int numShips = 0;

	std::vector<Planet> planets = pw.Planets();
	std::vector<Planet*> myPlanets, notMyPlanets;
	sort(planets.begin(), planets.end(), cmp1);
	for (unsigned int i = 0, n = planets.size(); i < n; i++)
	{
		Planet* p = &planets[i];

		if (p->Owner() == 1)
		{
			numShips += p->NumShips();
			myPlanets.push_back(p);
		}
		else
		{
			notMyPlanets.push_back(p);
		}
	}

	// (2) divide fleets
	std::vector<Fleet> fleets = pw.Fleets();
	std::vector<Fleet*> myFleets, notMyFleets;
	for (unsigned int i = 0, n = fleets.size(); i < n; i++)
	{
		Fleet* f = &fleets[i];

		if (f->Owner() == 1)
		{
			myFleets.push_back(f);
		}
		else
		{
			notMyFleets.push_back(f);
		}
	}

	// (3) attack weakest planets
	for (unsigned int i = 0, n = notMyPlanets.size(); i < n; i++)
	{
		Planet* target = notMyPlanets[i];
		const bool isNeutral = target->Owner() == 0;

		int targetNumShips = target->NumShips();

		// subtract our incomming fleets
		for (unsigned int j = 0, m = myFleets.size(); j < m; j++)
		{
			Fleet* f = myFleets[j];
			if (f->DestinationPlanet() == target->PlanetID())
			{
				targetNumShips -= f->NumShips();
			}
		}

		// add their incomming fleets
		for (unsigned int j = 0, m = notMyFleets.size(); j < m; j++)
		{
			Fleet* f = notMyFleets[j];
			if (f->DestinationPlanet() == target->PlanetID())
			{
				targetNumShips += f->NumShips();
			}
		}

		// if this target will be ours, we don't need to attack it, skip target
		if (targetNumShips < 0)
		{
			continue;
		}

		// if we don't have the amount of ships to deal with this, skip target
		if ((numShips - targetNumShips) < 0)
		{
			continue;
		}

		// sort our sourceplanets on distance from target creating an outwards
		// spiral
		globalTarget = target;
		sort(myPlanets.begin(), myPlanets.end(), cmp2);

		// send fleets until we've captured the planet
		for (unsigned int j = 0, m = myPlanets.size(); j < m; j++)
		{
			Planet* source = myPlanets[j];

			if (source->NumShips() <= 1)
			{
				continue;
			}

			const int distance = pw.Distance(source->PlanetID(), target->PlanetID());
			const int targetGrowth = isNeutral ? 0 : distance*target->GrowthRate();
			const int fleetSize = std::min<int>(source->NumShips() - 1, targetNumShips + targetGrowth + 1);

			numShips -= fleetSize;
			pw.IssueOrder(source->PlanetID(), target->PlanetID(), fleetSize);
			source->NumShips(source->NumShips() - fleetSize);
			target->NumShips(target->NumShips() - fleetSize + targetGrowth);

			// This planet is ours, proceed to next target
			if (target->NumShips() < 0)
			{
				break;
			}
		}
	}

	round++;
}

// This is just the main game loop that takes care of communicating with the
// game engine for you. You don't have to understand or change the code below.
int main(int argc, char *argv[]) {
  std::ofstream file;
  file.open("mybot.txt", std::ios::in|std::ios::trunc);
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
