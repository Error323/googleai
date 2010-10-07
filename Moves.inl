// globals
std::vector<Planet>* gAP;
int gTarget;

inline int Distance(const Planet& a, const Planet& b) {
	double x = a.X()-b.X();
	double y = a.Y()-b.Y();
	return int(ceil(sqrt(x*x + y*y)));
}

bool SortOnGrowthShipRatio(const int pidA, const int pidB) {
	const Planet& a = gAP->at(pidA);
	const Planet& b = gAP->at(pidB);
	
	double growA = a.GrowthRate() / (1.0*a.NumShips() + 1.0);
	double growB = b.GrowthRate() / (1.0*b.NumShips() + 1.0);

	return growA > growB;
}

bool SortOnDistanceToTarget(const int pidA, const int pidB) {
	Planet& t      = gAP->at(gTarget);
	Planet& a      = gAP->at(pidA);
	Planet& b      = gAP->at(pidB);
	int distA = Distance(a, t);
	int distB = Distance(b, t);
	return distA < distB;
}

// This function contains all the smart stuff
std::vector<std::vector<Fleet> > AlphaBeta::Node::GetActions() {
	gAP = &AP;
	std::vector<std::vector<Fleet> > actions;
	sort(NPIDX.begin(), NPIDX.end(), SortOnGrowthShipRatio);
	for (unsigned int i = 0, n = NPIDX.size(); i < n; i++)
	{
		std::vector<Fleet> orders;
		Planet& target = AP[NPIDX[i]];
		int totalFleet = 0;
		gTarget = target.PlanetID();
		sort(MPIDX.begin(), MPIDX.end(), SortOnDistanceToTarget);
		for (unsigned int k = 0, m = MPIDX.size(); k < m; k++)
		{
			Planet& source = AP[MPIDX[k]];
			if (source.NumShips() <= 0 || target.PlanetID() == source.PlanetID())
				continue;
			const int distance = Distance(target, source);
			int fleetSize = std::min<int>(source.NumShips(), target.NumShips()+1);
			orders.push_back(Fleet(1, fleetSize, source.PlanetID(),
				target.PlanetID(), distance, distance));
			totalFleet += fleetSize;
			if (totalFleet >= target.NumShips()+1)
				break;
		}
		if (totalFleet >= target.NumShips()+1)
			actions.push_back(orders);
	}
	return actions;
}
