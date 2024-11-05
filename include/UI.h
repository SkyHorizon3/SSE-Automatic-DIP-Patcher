#pragma once
#include "imgui.h"

class UI : public ISingleton<UI>
{
public:

	void Init();
	void Render();

private:

	void Draw();

	float CalcMiddle(const std::string& text);
	void CenteredText(const std::string& text);
	void CenteredTextColored(const ImVec4& color, const std::string& text);

	bool m_initialized = false;
	bool m_return = false;
	std::chrono::time_point<std::chrono::steady_clock> m_startTime;

};