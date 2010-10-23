#include "KnapSack.h"
#include "Logger.h"

KnapSack::KnapSack(std::vector<int>& w_, std::vector<double>& v_, int W_):
	w(w_), v(v_), W(W_) {
	ASSERT(w.size() == v.size());
	N = w.size();

	w.insert(w.begin(),0);
	v.insert(v.begin(),0.0);
	for (int i = 0; i <= N; i++)
	{
		for (int j = 0; j <= W; j++)
		{
			if (i == 0 || j == 0)
				C[i][j] =  0.0;
			else
				C[i][j] = -1.0;
		}
	}
}

std::vector<int>& KnapSack::Indices() {
	if (!I.empty())
	{
		return I;
	}

	double profit = ZeroOne(N, W);
	int aux = W;
	for (int i = N; i > 0; i--)
	{
		if (C[i][aux] != C[i - 1][aux])
		{
			I.push_back(i-1);
			aux -= w[i];
		}
	}
	return I;
}


double KnapSack::ZeroOne(int n, int c) {
	double val, temp, temp1;
	if (C[n][c] < 0.0)
	{
		temp = ZeroOne(n - 1, c);
		if (c < w[n])
		{
			val = temp;
		}
		else
		{
			temp1 = v[n] + ZeroOne(n - 1, c - w[n]);
			if (temp1 > temp)
				val = temp1;
			else
				val = temp;
		}
		C[n][c] = val;
	}
	return C[n][c];
}
