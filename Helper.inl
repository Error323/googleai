const std::vector<Planet>* gAP     = NULL;
int                        gTarget = 0;

inline bool SortOnGrowthRate(const int pidA, const int pidB) {
	ASSERT(pidA >= 0 && pidA < gAP->size());
	const Planet& a = gAP->at(pidA);
	ASSERT(pidB >= 0 && pidB < gAP->size());
	const Planet& b = gAP->at(pidB);
	return a.GrowthRate() > b.GrowthRate();
}

inline bool SortOnDistanceToTarget(const int pidA, const int pidB) {
	ASSERT(gTarget >= 0 && gTarget < gAP->size());
	const Planet& t = gAP->at(gTarget);
	ASSERT(pidA >= 0 && pidA < gAP->size());
	const Planet& a = gAP->at(pidA);
	ASSERT(pidB >= 0 && pidB < gAP->size());
	const Planet& b = gAP->at(pidB);
	const int distA = a.Distance(t);
	const int distB = b.Distance(t);
	return distA < distB;
}

inline bool SortOnDistanceToEnemy(const int pidA, const int pidB) {
	ASSERT(pidA >= 0 && pidA < gAP->size());
	const Planet& a = gAP->at(pidA);
	ASSERT(pidB >= 0 && pidB < gAP->size());
	const Planet& b = gAP->at(pidB);
	int dista = 1000000;
	int distb = 1000000;
	for (unsigned int i = 0, n = gAP->size(); i < n; i++)
	{
		const Planet& p = gAP->at(i);
		if (p.Owner() > 1)
		{
			int da = p.Distance(a);
			int db = p.Distance(b);
			dista = std::min<int>(dista, da);
			distb = std::min<int>(distb, db);
		}
	}
	if (dista == distb)
		return a.GrowthRate() > b.GrowthRate();
	else
		return dista < distb;
}

inline bool SortOnGrowthShipRatio(const int pidA, const int pidB) {
	ASSERT(pidA >= 0 && pidA < gAP->size());
	const Planet& a = gAP->at(pidA);
	ASSERT(pidB >= 0 && pidB < gAP->size());
	const Planet& b = gAP->at(pidB);
	const double growA = a.GrowthRate() / (1.0*a.NumShips() + 1.0);
	const double growB = b.GrowthRate() / (1.0*b.NumShips() + 1.0);
	return growA > growB;
}

inline bool SortOnPlanetAndTurnsLeft(const Fleet& a, const Fleet& b) {
	if (a.DestinationPlanet() == b.DestinationPlanet())
		return a.TurnsRemaining() < b.TurnsRemaining();
	else
		return a.DestinationPlanet() < b.DestinationPlanet();
}

void Erase(std::vector<int>& subject, std::vector<int>& eraser) {
	ASSERT(subject.size() >= eraser.size());
	sort(eraser.begin(), eraser.end());

	for (int i = eraser.size()-1; i >= 0; i--)
		subject.erase(subject.begin()+eraser[i]);

	eraser.clear();
}
