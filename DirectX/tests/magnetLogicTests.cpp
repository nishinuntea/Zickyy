#include "../magnetLogic.h"

#include <cassert>
#include <cmath>
#include <iostream>

namespace
{
	bool NearlyEqual(float first, float second, float tolerance = 0.0001f)
	{
		return std::abs(first - second) <= tolerance;
	}
}

int main()
{
	using MagnetLogic::Polarity;

	assert(MagnetLogic::Opposite(Polarity::North) == Polarity::South);
	assert(MagnetLogic::Opposite(Polarity::South) == Polarity::North);
	assert(MagnetLogic::Attracts(Polarity::North, Polarity::South));
	assert(!MagnetLogic::Attracts(Polarity::North, Polarity::North));
	assert(NearlyEqual(MagnetLogic::ForceDirection(Polarity::North, Polarity::South), 1.0f));
	assert(NearlyEqual(MagnetLogic::ForceDirection(Polarity::South, Polarity::South), -1.0f));

	assert(NearlyEqual(MagnetLogic::ForceFalloff(0.0f, 3.0f), 1.0f));
	assert(NearlyEqual(MagnetLogic::ForceFalloff(1.5f, 3.0f), 0.25f));
	assert(NearlyEqual(MagnetLogic::ForceFalloff(3.0f, 3.0f), 0.0f));
	assert(NearlyEqual(MagnetLogic::ForceMagnitude(1.5f, 3.0f, 20.0f), 5.0f));
	assert(NearlyEqual(MagnetLogic::ForceMagnitude(1.0f, 0.0f, 20.0f), 0.0f));

	assert(MagnetLogic::Overlaps(0.0f, 0.5f, 1.0f, 0.5f));
	assert(!MagnetLogic::Overlaps(0.0f, 0.5f, 1.01f, 0.5f));

	float respawn = 0.25f;
	assert(!MagnetLogic::TickRespawn(respawn, 0.1f));
	assert(MagnetLogic::TickRespawn(respawn, 0.2f));
	assert(NearlyEqual(respawn, 0.0f));

	std::cout << "magnetLogicTests: all tests passed\n";
	return 0;
}
