#pragma once

#include <chrono>

class CHighResolutionTimer
{
public:
	CHighResolutionTimer();
	~CHighResolutionTimer();

	void Start();
	double Elapsed();

private:
	std::chrono::high_resolution_clock::time_point m_t1;
	bool m_bStarted;
};
