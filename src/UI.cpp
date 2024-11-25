#include "UI.h"
#include "Manager.h"
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

// Inspired by https://github.com/ersh1/OpenAnimationReplacer/tree/main. Credits to Ersh!

void UI::Init()
{
	if (m_initialized)
		return;

	const auto renderManager = RE::BSGraphics::Renderer::GetSingleton();
	if (!renderManager)
	{
		SKSE::log::error("Couldn't get Renderer!");
		return;
	}

	const auto swapChain = reinterpret_cast<IDXGISwapChain*>(renderManager->GetRuntimeData().renderWindows[0].swapChain);
	if (!swapChain)
	{
		SKSE::log::error("Couldn't get swapChain!");
		return;
	}

	DXGI_SWAP_CHAIN_DESC desc{};
	if (FAILED(swapChain->GetDesc(&desc)))
	{
		SKSE::log::error("IDXGISwapChain::GetDesc failed.");
		return;
	}

	ImGui::CreateContext();

	auto& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
	io.IniFilename = nullptr;

	ImGui::StyleColorsDark();

	const auto device = reinterpret_cast<ID3D11Device*>(renderManager->GetRuntimeData().forwarder);
	const auto context = reinterpret_cast<ID3D11DeviceContext*>(renderManager->GetRuntimeData().context);

	ImGui_ImplWin32_Init(desc.OutputWindow);
	ImGui_ImplDX11_Init(device, context);

	m_initialized = true;
}

void UI::Draw()
{
	constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

	const auto appWidth = ImGui::GetIO().DisplaySize.x;
	const auto appHeight = ImGui::GetIO().DisplaySize.y;

	const ImVec2 windowSize = ImVec2(appWidth * 0.1f, appHeight * 0.13f);
	ImGui::SetNextWindowSize(windowSize);

	const ImVec2 windowPos = ImVec2(appWidth * 0.5f, appHeight - windowSize.y);
	ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, ImVec2(0.5f, 1.0f));

	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 8.0f);
	if (ImGui::Begin("Automatic DIP Patcher##Information", nullptr, flags))
	{
		const auto titleText = std::format("Automatic DIP Patcher {}.{}.{}", Plugin::VERSION.major(), Plugin::VERSION.minor(), Plugin::VERSION.patch());

		CenteredText(titleText);
		ImGui::Separator();

		const auto manager = Manager::GetSingleton();

		const bool success = manager->getSuccess();
		const auto statusMessage = success ? "Patching was successful!"sv : "Patching failed!"sv;
		const auto statusColor = success ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

		CenteredTextColored(statusColor, statusMessage.data());

		const auto errors = manager->getErrors();
		if (!errors.empty())
		{
			if (errors.size() > 1)
			{
				CenteredText(std::format("{} errors occurred!", errors.size()));
			}
			else
			{
				CenteredText(std::format("{} error occurred!", errors.size()));
			}
			CenteredText("Issues written to:");
			CenteredText(std::format("{}.log", SKSE::PluginDeclaration::GetSingleton()->GetName()));
		}

	}
	ImGui::PopStyleVar();
	ImGui::End();
}

float UI::CalcMiddle(const std::string& text)
{
	const auto textWidth = ImGui::CalcTextSize(text.data()).x;
	return (ImGui::GetWindowSize().x - textWidth) * 0.5f;
}

void UI::CenteredText(const std::string& text)
{
	ImGui::SetCursorPosX(CalcMiddle(text));
	ImGui::TextUnformatted(text.c_str());
}

void UI::CenteredTextColored(const ImVec4& color, const std::string& text)
{
	ImGui::SetCursorPosX(CalcMiddle(text));
	ImGui::TextColored(color, text.c_str());
}

void UI::Render()
{
	if (!m_initialized || m_return)
		return;

	static bool timerInitialized = false;

	if (!timerInitialized)
	{
		m_startTime = std::chrono::steady_clock::now();
		timerInitialized = true;
	}

	const auto currentTime = std::chrono::steady_clock::now();
	const std::chrono::duration<double> elapsed = currentTime - m_startTime;

	if (elapsed.count() >= 10.0)
	{
		m_return = true;
		return;
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	{

		Draw();

	}

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}