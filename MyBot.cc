#include "PlanetWars.h"
#include "Simulator.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <limits>
#include <cassert>
#include <cmath>
#include <utility>


#define VERSION "8.0"

#define MAX_ROUNDS 200

struct Point {
	Point(double x_, double y_):
		x(x_),
		y(y_)
	{}
	double x,y;

	Point& operator/= (const double s) {
		x /= s;
		y /= s;
		return *this;
	}
};


PlanetWars* globalPW     = NULL;
int         globalTarget = 0;
int         globalRemain = 0;
Point*      globalMP     = NULL;
Point*      globalEP     = NULL;

bool SortOnGrowthRateAndTurnsRemainingAndPlanet(const int fidA, const int fidB) {
	const Fleet& fA = globalPW->GetFleet(fidA);
	const Fleet& fB = globalPW->GetFleet(fidB);
	const Planet& pA = globalPW->GetPlanet(fA.DestinationPlanet());
	const Planet& pB = globalPW->GetPlanet(fB.DestinationPlanet());
	if (pA.PlanetID() == pB.PlanetID())
		return fA.TurnsRemaining() < fB.TurnsRemaining();
	else
	if (pA.GrowthRate() == pB.GrowthRate())
		return pA.PlanetID() < pB.PlanetID();
	else
		return pA.GrowthRate() > pB.GrowthRate();
}

// sorts like this: [1 2 ... 0 -1 -2 ...]
bool SortOnDistanceToFleet(const int pidA, const int pidB) {
	int distA = globalPW->Distance(pidA, globalTarget) - globalRemain;
	int distB = globalPW->Distance(pidB, globalTarget) - globalRemain;
	if (distA > 0 && distB > 0)
		return distA < distB;
	else
		return distA > distB;
}

bool SortOnDistanceToTarget(const int pidA, const int pidB) {
	int distA = globalPW->Distance(pidA, globalTarget);
	int distB = globalPW->Distance(pidB, globalTarget);
	return distA < distB;
}

#define EPS 0.1
bool SortOnGrowthShipRatio(const int pidA, const int pidB) {
	const Planet& a = globalPW->GetPlanet(pidA);
	const Planet& b = globalPW->GetPlanet(pidB);
	const double mDistA = sqrt(pow(a.X() - globalMP->x, 2.0) + pow(a.Y() - globalMP->y, 2.0));
	const double mDistB = sqrt(pow(b.X() - globalMP->x, 2.0) + pow(b.Y() - globalMP->y, 2.0));
	const double eDistA = sqrt(pow(a.X() - globalEP->x, 2.0) + pow(a.Y() - globalEP->y, 2.0));
	const double eDistB = sqrt(pow(b.X() - globalEP->x, 2.0) + pow(b.Y() - globalEP->y, 2.0));
	double growA = a.GrowthRate() / (1.0*a.NumShips() + 1.0);
	double growB = b.GrowthRate() / (1.0*b.NumShips() + 1.0);

	if (abs(mDistA - mDistB) < EPS)
		return growA > growB;
	else
		return mDistA < mDistB;
}

void DoTurn(PlanetWars& pw, int round, std::ofstream& file) {
	globalPW = &pw;
	std::vector<Planet> AP = pw.Planets();
	std::vector<int>    MPIDX;
	std::vector<int>    NPIDX;
	std::vector<int>    EPIDX;
	std::vector<int>    NMPIDX;

	int myNumShips = 0;
	int enemyNumShips = 0;
	Point mP(0.0, 0.0); globalMP = &mP;
	Point eP(0.0, 0.0); globalEP = &eP;
	for (unsigned int i = 0, n = AP.size(); i < n; i++)
	{
		Planet& p = AP[i];
		switch (p.Owner())
		{
			case 0: {
				NPIDX.push_back(i);
				NMPIDX.push_back(i); 
			} break;

			case 1: {
				MPIDX.push_back(i);
				mP.x += p.X();
				mP.y += p.Y();
				myNumShips += p.NumShips();
			} break;

			default: {
				NMPIDX.push_back(i); 
				EPIDX.push_back(i);
				eP.x += p.X();
				eP.y += p.Y();
				enemyNumShips += p.NumShips();
			} break;
		}
	}
	mP /= MPIDX.size(); eP /= EPIDX.size();

	std::vector<Fleet>  AF = pw.Fleets();
	std::vector<int>    EFIDX;
	std::vector<int>    MFIDX;
	for (unsigned int i = 0, n = AF.size(); i < n; i++)
	{
		Fleet& f = AF[i];
		if (f.Owner() == 1)
		{
			MFIDX.push_back(i);
			myNumShips += f.NumShips();
		}
		else
		{
			EFIDX.push_back(i);
			enemyNumShips += f.NumShips();
		}
	}


	Simulator s(file);
	s.Start(MAX_ROUNDS-round, AP, AF);

	// (1) defend our planets
	sort(EFIDX.begin(), EFIDX.end(), SortOnGrowthRateAndTurnsRemainingAndPlanet);
	for (unsigned int i = 0, n = EFIDX.size(); i < n; i++)
	{
		Fleet&  f      = AF[EFIDX[i]];
		const int tid  = f.DestinationPlanet();
		Planet& target = AP[f.DestinationPlanet()];

		if (target.Owner() == 1 && !s.IsMyPlanet(tid))
		{
			std::vector<Fleet>  orders;
			std::vector<Planet> backupAP(AP);
			std::vector<Fleet>  backupAF(AF);
			
			globalTarget = tid;
			Simulator s1(file);
			bool successfullAttack = false;
			sort(MPIDX.begin(), MPIDX.end(), SortOnDistanceToTarget);
			for (unsigned int j = 0, m = MPIDX.size(); j < m; j++)
			{
				Planet& source = AP[MPIDX[j]];
				int sid = source.PlanetID();

				// if we don't have enough ships anymore
				if (source.NumShips() <= 1)
				{
					continue;
				}

				const int distance = pw.Distance(tid, sid);
				s1.Start(f.TurnsRemaining(), AP, AF);
				if (s1.IsMyPlanet(tid))
				{
					break;
				}
								
				int fleetsRequired = s1.GetPlanet(tid).NumShips();

				const int fleetSize = std::min<int>(source.NumShips() - 1, fleetsRequired);

				Fleet order(1, fleetSize, sid, tid, distance, distance);
				orders.push_back(order);
				AF.push_back(order);
				source.NumShips(source.NumShips() - fleetSize);

				// See if given all previous orders, this planet will be ours
				s1.Start(MAX_ROUNDS-round, AP, AF);
				if (s1.IsMyPlanet(tid))
				{
					successfullAttack = true;
					break;
				}
			}
			if (successfullAttack)
			{
				for (unsigned int j = 0, m = orders.size(); j < m; j++)
				{
					Fleet& order = orders[j];
					LOG("\torders: "<<order);
					pw.IssueOrder(order.SourcePlanet(), order.DestinationPlanet(), order.NumShips());
				}
			}
			else
			{
				AP = backupAP;
				AF = backupAF;
			}
		}
	}
	
	// (2) overtake neutral planets captured by enemy in the future
	for (unsigned int i = 0, n = NPIDX.size(); i < n; i++)
	{
		Planet& curTarget = AP[NPIDX[i]];
		int tid = curTarget.PlanetID();

		const double mDist = sqrt(pow(curTarget.X()-mP.x, 2.0) + pow(curTarget.Y()-mP.y, 2.0));
		const double eDist = sqrt(pow(curTarget.X()-eP.x, 2.0) + pow(curTarget.Y()-eP.y, 2.0));
		if (!s.Winning() && mDist > eDist)
		{
			continue;
		}

		// This neutral planet will be captured by the enemy
		if (s.GetPlanet(tid).Owner() > 1)
		{
			std::vector<std::pair<int,int> >& history = s.GetOwnershipHistory(tid);

			std::vector<Fleet>  orders;
			std::vector<Planet> backupAP(AP);
			std::vector<Fleet>  backupAF(AF);
			
			globalTarget = tid;
			globalRemain = history.back().second;

			Simulator s1(file);
			s1.Start(globalRemain, AP, AF);

			bool successfullAttack = false;

			sort(MPIDX.begin(), MPIDX.end(), SortOnDistanceToFleet);
			for (unsigned int j = 0, m = MPIDX.size(); j < m; j++)
			{
				Planet& source = AP[MPIDX[j]];
				int sid = source.PlanetID();
				const int distance = pw.Distance(tid, sid);

				// if the first planet is still too close don't attack yet
				if (j == 0 && distance <= globalRemain)
				{
					break;
				}

				// if we don't have enough ships anymore
				if (source.NumShips() <= 1)
				{
					continue;
				}

				s1.Start(distance, AP, AF);
				int fleetsRequired = s1.GetPlanet(tid).NumShips() + 1;

				const int fleetSize = std::min<int>(source.NumShips() - 1, fleetsRequired);

				Fleet order(1, fleetSize, sid, tid, distance, distance);
				orders.push_back(order);
				AF.push_back(order);
				source.NumShips(source.NumShips() - fleetSize);

				// See if given all previous orders, this planet will be ours
				s1.Start(MAX_ROUNDS-round, AP, AF);
				if (s1.IsMyPlanet(tid))
				{
					successfullAttack = true;
					break;
				}
			}
			if (successfullAttack)
			{
				for (unsigned int j = 0, m = orders.size(); j < m; j++)
				{
					Fleet& order = orders[j];
					pw.IssueOrder(order.SourcePlanet(), order.DestinationPlanet(), order.NumShips());
				}
			}
			else
			{
				AP = backupAP;
				AF = backupAF;
			}
		}
	}

	// (3) capture good planets during first round
	if (round <= 0 || myNumShips > enemyNumShips*3)
	{
		sort(NMPIDX.begin(), NMPIDX.end(), SortOnGrowthShipRatio);
		for (unsigned int i = 0, n = NMPIDX.size(); i < n; i++)
		{
			Planet& curTarget = AP[NMPIDX[i]];
			const int tid = curTarget.PlanetID();
			if (s.IsMyPlanet(tid))
			{
				continue;
			}

			std::vector<Fleet>  orders;
			std::vector<Planet> backupAP(AP);
			std::vector<Fleet>  backupAF(AF);
			
			globalTarget = tid;
			Simulator s1(file);
			bool successfullAttack = false;

			sort(MPIDX.begin(), MPIDX.end(), SortOnDistanceToTarget);
			for (unsigned int j = 0, m = MPIDX.size(); j < m; j++)
			{
				Planet& source = AP[MPIDX[j]];
				int sid = source.PlanetID();
				const int distance = pw.Distance(tid, sid);

				// if we don't have enough ships anymore
				if (source.NumShips() <= 1)
				{
					continue;
				}

				s1.Start(distance, AP, AF);
				int fleetsRequired = s1.GetPlanet(tid).NumShips() + 1;

				const int fleetSize = std::min<int>(source.NumShips() - 1, fleetsRequired);

				Fleet order(1, fleetSize, sid, tid, distance, distance);
				orders.push_back(order);
				AF.push_back(order);
				source.NumShips(source.NumShips() - fleetSize);

				// See if given all previous orders, this planet will be ours
				s1.Start(MAX_ROUNDS-round, AP, AF);
				if (s1.IsMyPlanet(tid))
				{
					successfullAttack = true;
					break;
				}
			}
			if (successfullAttack)
			{
				for (unsigned int j = 0, m = orders.size(); j < m; j++)
				{
					Fleet& order = orders[j];
					pw.IssueOrder(order.SourcePlanet(), order.DestinationPlanet(), order.NumShips());
				}
			}
			else
			{
				AP = backupAP;
				AF = backupAF;
			}
		}
	}
}


// This is just the main game loop that takes care of communicating with the
// game engine for you. You don't have to understand or change the code below.
int main(int argc, char *argv[]) {
	unsigned int round = 0;
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
				LOG("round: " << round);
				LOG(pw.ToString());
				DoTurn(pw, round, file);
				LOG("\n--------------------------------------------------------------------------------\n");
				round++;
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
