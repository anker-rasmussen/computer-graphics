#include "HighResolutionTimer.h"

CHighResolutionTimer::CHighResolutionTimer() :
m_bStarted(false)
{
}

CHighResolutionTimer::~CHighResolutionTimer()
{
}

void CHighResolutionTimer::Start()
{
	m_bStarted = true;
	m_t1 = std::chrono::high_resolution_clock::now();
}

double CHighResolutionTimer::Elapsed()
{
	if (!m_bStarted)
		return 0.0;

	auto t2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = t2 - m_t1;
	return elapsed.count();
}
