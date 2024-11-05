#pragma once


struct Version
{
	int major;
	int minor;
	int patch;
	int build;

	Version(int maj = 0, int min = 0, int pat = 0, int bld = 0)
		: major(maj), minor(min), patch(pat), build(bld) {}

	bool operator>=(const Version& other) const
	{
		if (major > other.major) return true;
		if (major == other.major && minor > other.minor) return true;
		if (major == other.major && minor == other.minor && patch > other.patch) return true;
		if (major == other.major && minor == other.minor && patch == other.patch && build >= other.build) return true;
		return false;
	}

	std::string toString() const
	{
		return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch) + "." + std::to_string(build);
	}
};

class Manager : public ISingleton<Manager>
{
public:

	void loadINI();
	void RunPostLoad();
	void RunDataLoaded();
	std::vector<std::string> getErrors() const { return m_errors; }
	bool getSuccess() const { return m_success; }
	bool getDebugLogState() const { return m_enableDebugLog; }

private:

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

	Version getEXEVersion(const LPCWSTR& szVersionFile);
	bool executeDIP(const std::filesystem::path& path);

	std::unordered_map<std::filesystem::path, std::vector<Config>> m_configInformation;
	std::vector<std::string> m_errors;
	bool m_success = false;

	// INI settings
	bool m_enableDebugLog = false;
	bool m_enableWriteJSON = false;
	bool m_enablePopupWindow = true;
};