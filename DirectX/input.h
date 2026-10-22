#pragma once

class Input
{
private:
	static BYTE m_OldKeyState[256];
	static BYTE m_KeyState[256];
	static bool m_PendingKeyTriggers[256];
	static bool m_KeyTriggers[256];

	static LONG m_MouseDeltaX;
	static LONG m_MouseDeltaY;
	static LONG m_PendingMouseDeltaX;
	static LONG m_PendingMouseDeltaY;

	static bool m_MouseLookRequested;
	static bool m_MouseCapturePaused;
	static bool m_CursorHidden;

	static void UpdateMouseCapture();

public:
	static void Init();
	static void Uninit();
	static void Update();

	static bool GetKeyPress(BYTE KeyCode);
	static bool GetKeyTrigger(BYTE KeyCode);
	static void ProcessKeyDown(WPARAM KeyCode, LPARAM KeyData);

	static void ProcessRawInput(LPARAM RawInputHandle);
	static void SetMouseLookEnabled(bool Enabled);
	static bool IsMouseLookActive();
	static float GetMouseDeltaX();
	static float GetMouseDeltaY();
};
