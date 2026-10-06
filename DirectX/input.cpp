#include "main.h"
#include "input.h"

BYTE Input::m_OldKeyState[256]{};
BYTE Input::m_KeyState[256]{};
bool Input::m_PendingKeyTriggers[256]{};
bool Input::m_KeyTriggers[256]{};

LONG Input::m_MouseDeltaX{};
LONG Input::m_MouseDeltaY{};
LONG Input::m_PendingMouseDeltaX{};
LONG Input::m_PendingMouseDeltaY{};

bool Input::m_MouseLookRequested{};
bool Input::m_MouseCapturePaused{};
bool Input::m_CursorHidden{};

void Input::Init()
{
	memset(m_OldKeyState, 0, sizeof(m_OldKeyState));
	memset(m_KeyState, 0, sizeof(m_KeyState));
	memset(m_PendingKeyTriggers, 0, sizeof(m_PendingKeyTriggers));
	memset(m_KeyTriggers, 0, sizeof(m_KeyTriggers));

	RAWINPUTDEVICE mouseDevice{};
	mouseDevice.usUsagePage = 0x01;
	mouseDevice.usUsage = 0x02;
	mouseDevice.dwFlags = 0;
	mouseDevice.hwndTarget = GetWindow();
	if (!RegisterRawInputDevices(
		&mouseDevice,
		1,
		sizeof(mouseDevice)))
	{
		ThrowIfFailed(
			HRESULT_FROM_WIN32(GetLastError()),
			"RegisterRawInputDevices(mouse)");
	}
}

void Input::Uninit()
{
	SetMouseLookEnabled(false);

	RAWINPUTDEVICE mouseDevice{};
	mouseDevice.usUsagePage = 0x01;
	mouseDevice.usUsage = 0x02;
	mouseDevice.dwFlags = RIDEV_REMOVE;
	mouseDevice.hwndTarget = nullptr;
	RegisterRawInputDevices(
		&mouseDevice,
		1,
		sizeof(mouseDevice));
}

void Input::Update()
{
	memcpy(m_KeyTriggers, m_PendingKeyTriggers, sizeof(m_KeyTriggers));
	memset(m_PendingKeyTriggers, 0, sizeof(m_PendingKeyTriggers));
	memcpy(m_OldKeyState, m_KeyState, sizeof(m_KeyState));

	if (!GetKeyboardState(m_KeyState))
	{
		memset(m_KeyState, 0, sizeof(m_KeyState));
	}

	m_MouseDeltaX = m_PendingMouseDeltaX;
	m_MouseDeltaY = m_PendingMouseDeltaY;
	m_PendingMouseDeltaX = 0;
	m_PendingMouseDeltaY = 0;

	if (m_MouseLookRequested && GetKeyTrigger(VK_TAB))
	{
		m_MouseCapturePaused = !m_MouseCapturePaused;
		m_MouseDeltaX = 0;
		m_MouseDeltaY = 0;
	}

	UpdateMouseCapture();
	if (!IsMouseLookActive())
	{
		m_MouseDeltaX = 0;
		m_MouseDeltaY = 0;
	}
}

bool Input::GetKeyPress(BYTE KeyCode)
{
	return (m_KeyState[KeyCode] & 0x80) != 0;
}

bool Input::GetKeyTrigger(BYTE KeyCode)
{
	return
		m_KeyTriggers[KeyCode] ||
		((m_KeyState[KeyCode] & 0x80) != 0 &&
		(m_OldKeyState[KeyCode] & 0x80) == 0);
}

void Input::ProcessKeyDown(WPARAM KeyCode, LPARAM KeyData)
{
	// Preserve even a press/release pair between two fixed simulation ticks.
	if (KeyCode < 256 && (KeyData & (1LL << 30)) == 0)
		m_PendingKeyTriggers[KeyCode] = true;
}

void Input::ProcessRawInput(LPARAM RawInputHandle)
{
	RAWINPUT rawInput{};
	UINT rawInputSize = sizeof(rawInput);
	const UINT bytesRead = GetRawInputData(
		reinterpret_cast<HRAWINPUT>(RawInputHandle),
		RID_INPUT,
		&rawInput,
		&rawInputSize,
		sizeof(RAWINPUTHEADER));
	if (bytesRead == static_cast<UINT>(-1) ||
		rawInput.header.dwType != RIM_TYPEMOUSE ||
		(rawInput.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0)
	{
		return;
	}

	m_PendingMouseDeltaX += rawInput.data.mouse.lLastX;
	m_PendingMouseDeltaY += rawInput.data.mouse.lLastY;
}

void Input::SetMouseLookEnabled(bool Enabled)
{
	m_MouseLookRequested = Enabled;
	m_MouseCapturePaused = false;
	m_MouseDeltaX = 0;
	m_MouseDeltaY = 0;
	m_PendingMouseDeltaX = 0;
	m_PendingMouseDeltaY = 0;
	UpdateMouseCapture();
}

bool Input::IsMouseLookActive()
{
	return
		m_MouseLookRequested &&
		!m_MouseCapturePaused &&
		GetForegroundWindow() == GetWindow() &&
		!IsIconic(GetWindow());
}

float Input::GetMouseDeltaX()
{
	return static_cast<float>(m_MouseDeltaX);
}

float Input::GetMouseDeltaY()
{
	return static_cast<float>(m_MouseDeltaY);
}

void Input::UpdateMouseCapture()
{
	if (IsMouseLookActive())
	{
		RECT clientRectangle{};
		if (GetClientRect(GetWindow(), &clientRectangle))
		{
			POINT topLeft{
				clientRectangle.left,
				clientRectangle.top
			};
			POINT bottomRight{
				clientRectangle.right,
				clientRectangle.bottom
			};
			if (ClientToScreen(GetWindow(), &topLeft) &&
				ClientToScreen(GetWindow(), &bottomRight))
			{
				RECT screenRectangle{
					topLeft.x,
					topLeft.y,
					bottomRight.x,
					bottomRight.y
				};
				ClipCursor(&screenRectangle);
			}
		}

		if (!m_CursorHidden)
		{
			ShowCursor(FALSE);
			m_CursorHidden = true;
		}
		return;
	}

	ClipCursor(nullptr);
	if (m_CursorHidden)
	{
		ShowCursor(TRUE);
		m_CursorHidden = false;
	}
}
