#pragma once

#include "gameObject.h"

class GrappleAnchor final : public GameObject
{
private:
	bool m_Highlighted{};
	bool m_Attached{};

public:
	void Init() override
	{
		m_Layer = 2;
	}

	void SetHighlighted(bool Highlighted) { m_Highlighted = Highlighted; }
	void SetAttached(bool Attached) { m_Attached = Attached; }

	bool IsHighlighted() const { return m_Highlighted; }
	bool IsAttached() const { return m_Attached; }
};
