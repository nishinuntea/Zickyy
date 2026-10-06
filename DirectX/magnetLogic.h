#pragma once

#include <algorithm>
#include <cmath>

namespace MagnetLogic
{
	enum class Polarity : int
	{
		South = -1,
		North = 1,
	};

	inline Polarity Opposite(Polarity value)
	{
		return value == Polarity::North ? Polarity::South : Polarity::North;
	}

	inline bool Attracts(Polarity first, Polarity second)
	{
		return first != second;
	}

	inline float ForceDirection(Polarity first, Polarity second)
	{
		return Attracts(first, second) ? 1.0f : -1.0f;
	}

	inline float ForceFalloff(float distance, float radius)
	{
		if (radius <= 0.0f || distance >= radius)
		{
			return 0.0f;
		}

		const float normalized = std::clamp(1.0f - distance / radius, 0.0f, 1.0f);
		return normalized * normalized;
	}

	inline float ForceMagnitude(float distance, float radius, float maximumForce)
	{
		return std::max(0.0f, maximumForce) * ForceFalloff(distance, radius);
	}

	inline bool Overlaps(
		float firstCenter,
		float firstHalfExtent,
		float secondCenter,
		float secondHalfExtent)
	{
		return std::abs(firstCenter - secondCenter) <=
			std::max(0.0f, firstHalfExtent) + std::max(0.0f, secondHalfExtent);
	}

	inline bool TickRespawn(float& remainingSeconds, float deltaTime)
	{
		remainingSeconds = std::max(0.0f, remainingSeconds - std::max(0.0f, deltaTime));
		return remainingSeconds <= 0.0f;
	}
}
