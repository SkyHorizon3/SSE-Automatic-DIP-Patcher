#include "Manager.h"
#include "Hooks.h"
#include "Utils.h"

void Manager::RunPostLoad()
{
	if (m_enablePopupWindow)
	{
		Hooks::InstallHooks();
	}

	if (m_enableWriteJSON)
	{
		if (!writeJson(getPresetPath(), getDIPPatches()))
		{
			m_errors.emplace_back("writeJSON failed!");
		}
		return;
	}

	// read jsons, get DIP and patch, Display stuff in popup

	readConfigs();

	if (m_configInformation.empty())
		return;

	const auto DIPPath = getDIPPath();

	m_success = executeDIP(DIPPath);

}

void Manager::loadINI()
{
	const auto path = std::format("Data/SKSE/Plugins/{}.ini", Plugin::NAME);

	CSimpleIniA ini;
	ini.SetUnicode();
	ini.LoadFile(path.c_str());

	constexpr const char* main = "Main";
	m_enableDebugLog = ini.GetBoolValue(main, "EnableDebugLog");
	m_enableWriteJSON = ini.GetBoolValue(main, "EnableWriteJSON");
	m_enablePopupWindow = ini.GetBoolValue(main, "EnablePopupWindow");
}

bool Manager::readJson(const std::filesystem::path& path)
{
	std::ifstream openFile(path);
	if (!openFile.is_open())
	{
		m_errors.emplace_back(std::format("Couldn't load JSON: {}!", path.string()));
		return false;
	}

	std::stringstream buffer;
	buffer << openFile.rdbuf();
	const auto stringBuffer = buffer.str();

	glz::json_t json{};
	const auto result = glz::read_json(json, stringBuffer);
	if (result)
	{
		const std::string descriptive_error = glz::format_error(result, stringBuffer);
		m_errors.emplace_back(std::format("Error parsing JSON: {}", descriptive_error));
		return false;
	}

	for (const auto& entry : json.get_array())
	{
		if (entry.contains("patchPath") && entry.contains("alreadyPatched"))
		{
			const auto& patchPath = entry["patchPath"].get_string();
			const auto& alreadyPatched = entry["alreadyPatched"].get_boolean();

			if (alreadyPatched)
				continue;

			bool pathExists = false;
			for (const auto& [key, configs] : m_configInformation)
			{
				const auto it = std::find_if(configs.begin(), configs.end(), [&patchPath](const Config& config) {
					return config.patchPath == patchPath;
					});

				if (it != configs.end())
				{
					pathExists = true;
					break;
				}
			}

			if (!pathExists)
			{
				auto& configs = m_configInformation[path];
				configs.emplace_back(Config{ patchPath, alreadyPatched });
				SKSE::log::info("JSON: {} - Apply DIP for entry: {}", path.filename().string(), patchPath);
			}

		}
	}

	return true;
}

bool Manager::writeJson(const std::filesystem::path& path, const std::vector<Config>& vec)
{
	std::ofstream outFile(path);
	if (!outFile.is_open())
	{
		m_errors.emplace_back("Couldn't save automatically written json!");
		return false;
	}

	std::string buffer;
	const auto result = glz::write_json(vec, buffer);
	if (result)
	{
		const std::string descriptive_error = glz::format_error(result, buffer);
		m_errors.emplace_back(std::format("Error serializing vector: {}", descriptive_error));
		return false;
	}

	outFile << glz::prettify_json(buffer);
	outFile.close();

	return true;
}

void Manager::readConfigs()
{
	static constexpr const char* presetDirectory = "Data\\SKSE\\Plugins\\AutomaticDIPPatcher";
	if (!std::filesystem::exists(presetDirectory))
		return;

	for (const auto& config : std::filesystem::recursive_directory_iterator(presetDirectory))
	{
		if (config.is_regular_file() && config.path().filename().extension() == ".json")
		{
			if (!readJson(config.path()))
				continue;
		}
	}

}

std::string Manager::getPresetPath()
{
	static constexpr const char* configDirectory = "Data\\SKSE\\Plugins\\AutomaticDIPPatcher";

	if (!std::filesystem::exists(configDirectory))
		std::filesystem::create_directories(configDirectory);

	std::string configName{};
	int configNumber = 0;
	do
	{
		configName = "Default" + std::to_string(configNumber) + ".json";
		configNumber++;
	} while (std::filesystem::exists(std::string(configDirectory) + "\\" + configName));

	return std::string(configDirectory) + "\\" + configName;
}

std::vector<Manager::Config> Manager::getDIPPatches()
{
	std::vector<Config> dipPatches;

	for (const auto& entry : std::filesystem::directory_iterator("Data"))
	{
		if (entry.is_directory())
		{
			const std::string folderName = Utils::tolower(entry.path().string());
#
			if (folderName.find("dip") != std::string::npos)
			{
				const std::filesystem::path patchDir = entry.path() / "Patch";

				if (std::filesystem::exists(Utils::tolower(patchDir.string())))
				{
					dipPatches.emplace_back(entry.path(), false);
				}
			}
		}
	}

	return dipPatches;
}

std::filesystem::path Manager::getDIPPath()
{
	constexpr std::string_view stdDIPPath = "Data\\DIP";
	const std::filesystem::path searchPath = std::filesystem::exists(stdDIPPath) ? stdDIPPath : "Data";

	for (const auto& entry : std::filesystem::recursive_directory_iterator(searchPath))
	{
		if (entry.is_regular_file() && entry.path().filename() == "DIP_cli.exe")
		{
			const auto filepath = entry.path();
			const auto wstringFilepath = filepath.wstring();
			const auto version = getEXEVersion(wstringFilepath.c_str());
			if (version >= Version(2, 0, 2, 0))
			{
				SKSE::log::info("Path of used DIP: {} - Version: {}", filepath.string(), version.toString());
				return std::filesystem::current_path() / filepath;
			}
		}

	}

	m_errors.emplace_back("No DIP Installation found!");
	return "";
}

Version Manager::getEXEVersion(const LPCWSTR& szVersionFile)
{
	// some information from: https://stackoverflow.com/questions/940707/how-do-i-programmatically-get-the-version-of-a-dll-or-exe-file
	DWORD verHandle = 0;
	UINT size = 0;
	LPBYTE lpBuffer = NULL;
	DWORD verSize = GetFileVersionInfoSize(szVersionFile, &verHandle);

	if (verSize == 0)
	{
		m_errors.emplace_back("Error when retrieving the version size of DIP_cli.exe!");
		return Version();
	}

	std::vector<char> verData(verSize);
	if (!GetFileVersionInfo(szVersionFile, verHandle, verSize, verData.data()))
	{
		m_errors.emplace_back("Error when retrieving the version information of DIP_cli.exe!");
		return Version();
	}

	if (!VerQueryValue(verData.data(), L"\\", (VOID FAR * FAR*) & lpBuffer, &size) || size == 0)
	{
		m_errors.emplace_back("Error when retrieving the version of DIP_cli.exe!");
		return Version();
	}

	VS_FIXEDFILEINFO* verInfo = (VS_FIXEDFILEINFO*)lpBuffer;
	if (verInfo->dwSignature != 0xfeef04bd)
	{
		m_errors.emplace_back("Invalid signature in the version information of DIP_cli.exe!");
		return Version();
	}

	const int major = (verInfo->dwFileVersionMS >> 16) & 0xffff;
	const int minor = (verInfo->dwFileVersionMS >> 0) & 0xffff;
	const int patch = (verInfo->dwFileVersionLS >> 16) & 0xffff;
	const int build = (verInfo->dwFileVersionLS >> 0) & 0xffff;

	return Version(major, minor, patch, build);
}

bool Manager::executeDIP(const std::filesystem::path& path)
{
	if (path.empty())
		return false;

	static const std::filesystem::path currentPath = std::filesystem::current_path();
	static const std::filesystem::path currentDataPath = currentPath / "data";

	for (const auto& [jsonPath, config] : m_configInformation)
	{
		for (const auto& info : config)
		{
			const auto patchPath = currentPath / info.patchPath;

			std::wstring command = L"\"" + path.wstring() + L"\" -s \"" + patchPath.wstring() + L"\" \"" + currentDataPath.wstring() + L"\"";

			SKSE::log::debug("Running DIP command: {}", Utils::wstringToString(command));

			STARTUPINFO si;
			PROCESS_INFORMATION pi;

			ZeroMemory(&si, sizeof(si));
			si.cb = sizeof(si);
			ZeroMemory(&pi, sizeof(pi));

			if (CreateProcess(
				path.wstring().data(),
				command.data(), // command.data()
				NULL,
				NULL,
				FALSE,
				0,
				NULL,
				path.parent_path().wstring().data(),
				&si,
				&pi
				))
			{
				WaitForSingleObject(pi.hProcess, INFINITE);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
			}
			else
			{
				m_errors.emplace_back(std::format("Couldn't run DIP, error: {}", GetLastError()));
			}
		}
	}

	return true;
}