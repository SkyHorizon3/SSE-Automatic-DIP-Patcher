#include "Manager.h"

#define DLLEXPORT __declspec(dllexport)
SKSE_PLUGIN_VERSION = []()
	{
		SKSE::PluginVersionData v;
		v.PluginName(Plugin::NAME);
		v.PluginVersion(Plugin::VERSION);
		v.AuthorName("SkyHorizon"sv);
		v.UsesAddressLibrary();
		v.UsesNoStructs();
		return v;
	}
();

SKSE_PLUGIN_QUERY(const SKSE::QueryInterface*, SKSE::PluginInfo* pluginInfo)
{
	pluginInfo->name = SKSEPlugin_Version.pluginName;
	pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	pluginInfo->version = SKSEPlugin_Version.pluginVersion;
	return true;
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* skse)
{
	SKSE::Init(skse, true);

	const auto manager = Manager::GetSingleton();
	manager->loadINI();

	spdlog::set_pattern("[%H:%M:%S:%e] [%l] %v"s);

	if (manager->getDebugLogState())
	{
		spdlog::set_level(spdlog::level::trace);
		spdlog::flush_on(spdlog::level::trace);
	}
	else
	{
		spdlog::set_level(spdlog::level::info);
		spdlog::flush_on(spdlog::level::info);
	}

	SKSE::log::info("Game version: {}", skse->RuntimeVersion());

	manager->RunPostLoad();

	return true;
}