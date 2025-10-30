#include "Manager.h"
#include "Hooks.h"
#include "Utils.h"

void Manager::RunPostLoad()
{
	if (m_enableWriteJSON)
	{
		if (!writeJson(getPresetPath(), getDIPPatches()))
		{
			m_errors.emplace_back("Automatic json writing failed!");
		}
		writeErrors();
		return;
	}

	readConfigs();

	if (m_configInformation.empty())
	{
		writeErrors();
		return;
	}

	if (m_enablePopupWindow)
	{
		SKSE::AllocTrampoline(28);
		Hooks::InstallHooks();
	}

	const auto DIPPath = getDIPPath();
	m_success = executeDIP(DIPPath);

	for (const auto& [jsonPath, config] : m_configInformation)
	{
		if (!writeJson(jsonPath, config))
		{
			m_errors.emplace_back(std::format("Saving of json: {} failed!", jsonPath.string()));
		}
	}

	writeErrors();
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
	std::string buffer{};
	glz::json_t json{};

	const auto result = glz::read_file_json(json, path.string(), buffer);
	if (result)
	{
		const std::string descriptive_error = glz::format_error(result, buffer);
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

	std::string buffer{};
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
	if (!std::filesystem::exists(m_configDirectory))
		return;

	// collect then sort JSON files by filename for deterministic order
	std::vector<std::filesystem::path> jsonFiles;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(m_configDirectory))
	{
		if (entry.is_regular_file() && entry.path().filename().extension() == ".json")
		{
			jsonFiles.emplace_back(entry.path());
		}
	}

	std::sort(jsonFiles.begin(), jsonFiles.end(), [](const auto& a, const auto& b) {
		const auto an = a.filename().string();
		const auto bn = b.filename().string();
		if (an != bn)
			return an < bn;
		return a.string() < b.string();
		});

	for (const auto& path : jsonFiles)
	{
		readJson(path);
	}
}

std::string Manager::getPresetPath()
{
	if (!std::filesystem::exists(m_configDirectory))
		std::filesystem::create_directories(m_configDirectory);

	std::string configName{};
	int configNumber = 0;
	do
	{
		configName = "Default" + std::to_string(configNumber) + ".json";
		configNumber++;
	} while (std::filesystem::exists(m_configDirectory + "\\" + configName));

	return m_configDirectory + "\\" + configName;
}

std::vector<Manager::Config> Manager::getDIPPatches()
{
	std::vector<Config> dipPatches;

	for (const auto& entry : std::filesystem::directory_iterator("Data"))
	{
		if (entry.is_directory())
		{
			const std::string folderName = Utils::tolower(entry.path().string());

			if (folderName.find("dip") != std::string::npos)
			{
				const std::filesystem::path patchDir = entry.path() / "patch";

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
		if (entry.is_regular_file() && entry.path().filename() == "DIP.exe")
		{
			const auto filepath = entry.path();
			const auto wstringFilepath = filepath.wstring();
			const auto version = getEXEVersion(wstringFilepath.c_str());
			if (version.has_value() && version.value() >= REL::Version(2, 0, 2, 0))
			{
				SKSE::log::info("Path of used DIP: {} - Version: {}", filepath.string(), version.value().string());
				return std::filesystem::current_path() / filepath;
			}
		}

	}

	m_errors.emplace_back("No DIP Installation found!");
	return "";
}

std::optional<REL::Version> Manager::getEXEVersion(const LPCWSTR& szVersionFile)
{
	// some information from: https://stackoverflow.com/questions/940707/how-do-i-programmatically-get-the-version-of-a-dll-or-exe-file
	std::uint32_t verHandle = 0;
	UINT size = 0;
	LPBYTE lpBuffer = NULL;

	std::uint32_t verSize = REX::W32::GetFileVersionInfoSizeW(szVersionFile, &verHandle);

	if (verSize == 0)
	{
		m_errors.emplace_back("Error when retrieving the version size of DIP.exe!");
		return std::nullopt;
	}

	std::vector<BYTE> verData(verSize);
	if (!REX::W32::GetFileVersionInfoW(szVersionFile, verHandle, verSize, verData.data()))
	{
		m_errors.emplace_back("Error when retrieving the version information of DIP.exe!");
		return std::nullopt;
	}

	if (!REX::W32::VerQueryValueW(verData.data(), L"\\", (VOID FAR * FAR*) & lpBuffer, &size) || size == 0)
	{
		m_errors.emplace_back("Error when retrieving the version of DIP.exe!");
		return std::nullopt;
	}

	VS_FIXEDFILEINFO* verInfo = (VS_FIXEDFILEINFO*)lpBuffer;
	if (verInfo->dwSignature != 0xfeef04bd)
	{
		m_errors.emplace_back("Invalid signature in the version information of DIP.exe!");
		return std::nullopt;
	}

	return REL::Version(HIWORD(verInfo->dwFileVersionMS), LOWORD(verInfo->dwFileVersionMS), HIWORD(verInfo->dwFileVersionLS), LOWORD(verInfo->dwFileVersionLS));
}

bool Manager::executeDIP(const std::filesystem::path& path)
{
	if (path.empty())
		return false;

	const std::filesystem::path currentPath = std::filesystem::current_path();
	const std::filesystem::path currentDataPath = currentPath / "data";

	for (auto& [jsonPath, config] : m_configInformation)
	{
		for (auto& info : config)
		{
			const auto patchPath = currentPath / info.patchPath;

			std::wstring command = L"\"" + path.wstring() + L"\" -s \"" + patchPath.wstring() + L"\" \"" + currentDataPath.wstring() + L"\"";

			SKSE::log::debug("Running DIP command: {}", Utils::wstringToString(command));

			REX::W32::STARTUPINFOW si;
			REX::W32::PROCESS_INFORMATION pi;

			ZeroMemory(&si, sizeof(si));
			si.size = sizeof(si);
			ZeroMemory(&pi, sizeof(pi));


			if (REX::W32::CreateProcessW(
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
				WaitForSingleObject(pi.process, INFINITE);
				CloseHandle(pi.process);
				CloseHandle(pi.thread);

				info.alreadyPatched = true;
			}
			else
			{
				m_errors.emplace_back(std::format("Couldn't run DIP, error: {}", GetLastError()));
			}
		}
	}

	return true;
}