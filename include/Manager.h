#pragma once

class Manager : public ISingleton<Manager>
{
public:

	void loadINI();
	void RunPostLoad();
	std::vector<std::string> getErrors() const noexcept { return m_errors; }
	bool getSuccess() const noexcept { return m_success; }
	bool getDebugLogState() const noexcept { return m_enableDebugLog; }

private:

	const std::string m_configDirectory = "Data\\SKSE\\Plugins\\AutomaticPatcher\\DIP";

	struct Config
	{
		std::filesystem::path patchPath{};
		bool alreadyPatched = false;
	};

	bool readJson(const std::filesystem::path& path);
	bool writeJson(const std::filesystem::path& path, const std::vector<Config>& vec);

	void readConfigs();
	std::string getPresetPath();
	std::vector<Config> getDIPPatches();
	std::filesystem::path getDIPPath();

	std::optional<REL::Version> getEXEVersion(const LPCWSTR& szVersionFile);
	bool executeDIP(const std::filesystem::path& path);

	void writeErrors() const
	{
		for (const auto& error : m_errors)
			SKSE::log::error("{}", error);
	}

	std::map<std::filesystem::path, std::vector<Config>> m_configInformation;
	std::vector<std::string> m_errors;
	bool m_success = false;

	// INI settings
	bool m_enableDebugLog = false;
	bool m_enableWriteJSON = false;
	bool m_enablePopupWindow = true;
};