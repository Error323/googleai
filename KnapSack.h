#include <vector>

class KnapSack {
public:
	KnapSack(std::vector<int>&, std::vector<double>&, int);
	std::vector<int>& Indices();
	void Init(std::vector<int>&, std::vector<double>&, int);

private:
	std::vector<int>    w; // w_0,...,w_n
	std::vector<double> v; // v_0,...,v_n
	int W;                 // Knapsack capacity
	int N;                 // Total amount of items
	std::vector<double> C; // Cost matrix
	std::vector<int>    I; // Solution indices

	double ZeroOne(int, int);
};
