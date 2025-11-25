#include <filesystem>
#include <fstream>
#include <future>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <shlwapi.h>
#include <urlmon.h>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "shlwapi.lib")

#include "nlohmann/json.hpp"

#include "FileUtils.h"
#include "MumbleUtils.h"
#include "Settings.h"
#include "Types.h"
#include "TypesUtils.h"

BenchFileInfo::BenchFileInfo(const std::filesystem::path &full, const std::filesystem::path &relative, bool is_header)
    : full_path(full), relative_path(relative), is_directory_header(is_header)
{
    if (is_header)
    {
        display_name = "[+] " + relative.string();
    }
    else
    {
        auto filename = relative.filename().string();
        if (filename.ends_with("_v4.json"))
        {
            filename = filename.substr(0, filename.length() - 8);
        }
        display_name = "    " + filename;
    }
};

void to_lowercase(char *str)
{
    if (str == nullptr)
        return;

    while (*str)
    {
        *str = static_cast<char>(::tolower(*str));
        str++;
    }
}

std::string to_lowercase(const std::string &str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::pair<std::vector<std::pair<int, const BenchFileInfo *>>, std::set<std::string>> get_file_data_pairs(
    std::vector<BenchFileInfo> &benches_files,
    char *filter_string)
{
    auto filtered_files = std::vector<std::pair<int, const BenchFileInfo *>>{};
    auto directories_with_matches = std::set<std::string>{};

    if (filter_string[0] == '\0' || filter_string == nullptr)
    {
        // When filter is empty, filter by current character's profession
        const auto profession = get_current_profession_name();
        auto current_profession = to_lowercase(profession);

        if (current_profession.empty())
        {
            // If no profession available, show all files
            for (int n = 0; n < benches_files.size(); n++)
                filtered_files.emplace_back(n, &benches_files[n]);
        }
        else
        {
            // Get elite specs for this profession
            auto profession_id = string_to_profession(current_profession);
            auto elite_specs = get_elite_specs_for_profession(profession_id);

            // First pass: find files that match the profession or elite specs and collect their directories
            for (int n = 0; n < benches_files.size(); n++)
            {
                const auto &file_info = benches_files[n];

                if (!file_info.is_directory_header)
                {
                    auto display_lower = to_lowercase(file_info.display_name);
                    auto path_lower = to_lowercase(file_info.relative_path.string());

                    bool matches = false;

                    // Check if file matches current profession
                    if ((display_lower.find(current_profession) != std::string::npos) ||
                        (path_lower.find(current_profession) != std::string::npos))
                    {
                        matches = true;
                    }

                    // Check if file matches any elite spec for this profession
                    if (!matches)
                    {
                        for (const auto &elite_spec : elite_specs)
                        {
                            if (display_lower.find(elite_spec) != std::string::npos ||
                                path_lower.find(elite_spec) != std::string::npos)
                            {
                                matches = true;
                                break;
                            }
                        }
                    }

                    if (matches)
                    {
                        const auto parent_dir = file_info.relative_path.parent_path().string();
                        if (!parent_dir.empty() && parent_dir != ".")
                        {
                            directories_with_matches.insert(parent_dir);
                        }
                    }
                }
            }

            // Second pass: add directory headers and matching files to filtered list
            for (int n = 0; n < benches_files.size(); n++)
            {
                const auto &file_info = benches_files[n];

                if (file_info.is_directory_header)
                {
                    if (directories_with_matches.count(file_info.relative_path.string()) > 0)
                    {
                        filtered_files.emplace_back(n, &file_info);
                    }
                }
                else
                {
                    auto display_lower = to_lowercase(file_info.display_name);
                    auto path_lower = to_lowercase(file_info.relative_path.string());

                    bool matches = false;

                    // Check if file matches current profession
                    if (display_lower.find(current_profession) != std::string::npos ||
                        path_lower.find(current_profession) != std::string::npos)
                    {
                        matches = true;
                    }

                    // Check if file matches any elite spec for this profession
                    if (!matches)
                    {
                        for (const auto &elite_spec : elite_specs)
                        {
                            if (display_lower.find(elite_spec) != std::string::npos ||
                                path_lower.find(elite_spec) != std::string::npos)
                            {
                                matches = true;
                                break;
                            }
                        }
                    }

                    if (matches)
                    {
                        filtered_files.emplace_back(n, &file_info);
                    }
                }
            }
        }

        return std::make_pair(filtered_files, directories_with_matches);
    }

    // First pass: find all files that match and collect their directories
    for (int n = 0; n < benches_files.size(); n++)
    {
        const auto &file_info = benches_files[n];

        if (!file_info.is_directory_header)
        {
            auto display_lower = to_lowercase(file_info.display_name);

            if (display_lower.find(filter_string) != std::string::npos)
            {
                const auto parent_dir = file_info.relative_path.parent_path().string();
                if (!parent_dir.empty() && parent_dir != ".")
                {
                    directories_with_matches.insert(parent_dir);
                }
            }
        }
    }

    // Second pass: add directory headers and matching files to filtered list
    for (int n = 0; n < benches_files.size(); n++)
    {
        const auto &file_info = benches_files[n];

        if (file_info.is_directory_header)
        {
            if (directories_with_matches.count(file_info.relative_path.string()) > 0)
            {
                filtered_files.emplace_back(n, &file_info);
            }
        }
        else
        {
            auto display_lower = to_lowercase(file_info.display_name);

            if (display_lower.find(filter_string) != std::string::npos)
            {
                filtered_files.emplace_back(n, &file_info);
            }
        }
    }

    return std::make_pair(filtered_files, directories_with_matches);
}


bool load_rotaion_json(const std::filesystem::path &json_path, nlohmann::json &j)
{
    try
    {
        auto file{std::ifstream{json_path}};
        file >> j;
    }
    catch (const nlohmann::json::exception &e)
    {
        std::cerr << "Error parsing rotation data JSON: " << e.what() << std::endl;
        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error loading rotation data: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool load_skill_data_map(const std::filesystem::path &json_path, nlohmann::json &j)
{
    const auto skill_data_json =
        json_path.parent_path().parent_path().parent_path().parent_path() / "skills" / "gw2_skills_en.json";

    try
    {
        auto file2{std::ifstream{skill_data_json}};
        file2 >> j;
    }
    catch (const nlohmann::json::exception &e)
    {
        std::cerr << "Error parsing skill data JSON: " << e.what() << std::endl;
        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error loading skill data: " << e.what() << std::endl;
        return false;
    }

    return true;
}

std::string format_build_name(const std::string &raw_name)
{
    auto result = raw_name;

    const auto start = result.find_first_not_of(" \t");
    if (start != std::string::npos)
        result = result.substr(start);

    if (result.starts_with("condition_"))
        result = "Condition " + result.substr(10); // Add "Condition " prefix
    else if (result.starts_with("power_"))
        result = "Power " + result.substr(6); // Add "Power " prefix

    std::replace(result.begin(), result.end(), '_', ' ');

    bool capitalize_next = true;
    std::ranges::transform(result, result.begin(), [&capitalize_next](char c) {
        if (c == ' ')
        {
            capitalize_next = true;
            return c;
        }

        if (capitalize_next)
        {
            capitalize_next = false;
            return static_cast<char>(std::toupper(c));
        }

        return static_cast<char>(std::tolower(c));
    });

    return "    " + result;
}

std::vector<BenchFileInfo> get_bench_files(const std::filesystem::path &bench_path)
{
    auto files = std::vector<BenchFileInfo>{};
    auto directory_files = std::map<std::string, std::vector<std::filesystem::path>>{};

    try
    {
        for (const auto &entry : std::filesystem::recursive_directory_iterator(bench_path))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".json")
            {
                auto relative_path = std::filesystem::relative(entry.path(), bench_path);
                auto parent_dir = relative_path.parent_path().string();

                if (parent_dir.empty())
                    parent_dir = ".";

                directory_files[parent_dir].push_back(entry.path());
            }
        }

        for (const auto &[dir_name, dir_files] : directory_files)
        {
            if (dir_name != ".")
            {
                auto header_path = bench_path / dir_name;
                files.emplace_back(header_path, std::filesystem::path(dir_name), true);
            }

            for (const auto &file_path : dir_files)
            {
                auto relative_path = std::filesystem::relative(file_path, bench_path);
                files.emplace_back(file_path, relative_path, false);
            }
        }
    }
    catch (const std::filesystem::filesystem_error &ex)
    {
        std::cerr << "Error scanning bench files: " << ex.what() << std::endl;
    }

    return files;
}

std::map<std::string, KeybindInfo> parse_xml_keybinds(const std::filesystem::path &xml_path)
{
    auto keybinds = std::map<std::string, KeybindInfo>{};

    if (!std::filesystem::exists(xml_path))
        return keybinds;

    try
    {
        std::ifstream file(xml_path);
        std::string line;

        while (std::getline(file, line))
        {
            if (line.find("<action") != std::string::npos)
            {
                auto keybind = KeybindInfo{};

                auto name_start = line.find("name=\"");
                if (name_start != std::string::npos)
                {
                    name_start += 6; // Skip 'name="'
                    auto name_end = line.find("\"", name_start);
                    if (name_end != std::string::npos)
                    {
                        keybind.action_name = line.substr(name_start, name_end - name_start);
                    }
                }

                // Check for button2/mod2 first (prioritized)
                auto button2_start = line.find("button2=\"");
                auto mod2_start = line.find("mod2=\"");
                bool has_button2 = button2_start != std::string::npos;

                if (has_button2)
                {
                    // Use button2/mod2 pair
                    button2_start += 9; // Skip 'button2="'
                    auto button2_end = line.find("\"", button2_start);
                    if (button2_end != std::string::npos)
                    {
                        auto button2_str = line.substr(button2_start, button2_end - button2_start);
                        try
                        {
                            auto button_val = std::stoi(button2_str);
                            keybind.button = static_cast<Keys>(button_val);
                        }
                        catch (...)
                        {
                            keybind.button = Keys::NONE;
                        }
                    }

                    if (mod2_start != std::string::npos)
                    {
                        mod2_start += 6; // Skip 'mod2="'
                        auto mod2_end = line.find("\"", mod2_start);
                        if (mod2_end != std::string::npos)
                        {
                            auto mod2_str = line.substr(mod2_start, mod2_end - mod2_start);
                            try
                            {
                                auto mod_val = std::stoi(mod2_str);
                                keybind.modifier = static_cast<Modifiers>(mod_val);
                            }
                            catch (...)
                            {
                                keybind.modifier = static_cast<Modifiers>(0);
                            }
                        }
                    }
                }
                else
                {
                    // Use button/mod pair as fallback
                    auto button_start = line.find("button=\"");
                    if (button_start != std::string::npos)
                    {
                        button_start += 8; // Skip 'button="'
                        auto button_end = line.find("\"", button_start);
                        if (button_end != std::string::npos)
                        {
                            std::string button_str = line.substr(button_start, button_end - button_start);
                            try
                            {
                                int button_val = std::stoi(button_str);
                                keybind.button = static_cast<Keys>(button_val);
                            }
                            catch (...)
                            {
                                keybind.button = Keys::NONE;
                            }
                        }
                    }

                    auto mod_start = line.find("mod=\"");
                    if (mod_start != std::string::npos)
                    {
                        mod_start += 5; // Skip 'mod="'
                        auto mod_end = line.find("\"", mod_start);
                        if (mod_end != std::string::npos)
                        {
                            std::string mod_str = line.substr(mod_start, mod_end - mod_start);
                            try
                            {
                                int mod_val = std::stoi(mod_str);
                                keybind.modifier = static_cast<Modifiers>(mod_val);
                            }
                            catch (...)
                            {
                                keybind.modifier = static_cast<Modifiers>(0);
                            }
                        }
                    }
                }

                // Only store specific skill-related keybinds
                if (!keybind.action_name.empty() && keybind.button != Keys::NONE)
                {
                    // Check if this is one of the allowed action names
                    if (keybind.action_name == "Profession Skill 1" || keybind.action_name == "Profession Skill 2" ||
                        keybind.action_name == "Profession Skill 3" || keybind.action_name == "Profession Skill 4" ||
                        keybind.action_name == "Profession Skill 5" || keybind.action_name == "Profession Skill 7" ||
                        keybind.action_name == "Healing Skill" || keybind.action_name == "Utility Skill 1" ||
                        keybind.action_name == "Utility Skill 2" || keybind.action_name == "Utility Skill 3" ||
                        keybind.action_name == "Elite Skill")
                    {
                        keybinds[keybind.action_name] = keybind;
                    }
                }
            }
        }

        file.close();
    }
    catch (const std::exception &e)
    {
        (void)Globals::APIDefs->Log(ELogLevel_WARNING, "GW2RotaHelper", "Error parsing XML keybinds");
    }

    return keybinds;
}

bool DownloadFile(const std::string &url, const std::filesystem::path &outputPath)
{
    try
    {
        (void)Globals::APIDefs->Log(ELogLevel_DEBUG, "GW2RotaHelper", "Started ZIP Downloading");

        const auto hr = URLDownloadToFileA(nullptr, url.c_str(), outputPath.string().c_str(), 0, nullptr);
        return SUCCEEDED(hr);
    }
    catch (...)
    {
        (void)Globals::APIDefs->Log(ELogLevel_CRITICAL, "GW2RotaHelper", "ZIP downloading failed.");
        return false;
    }
}

bool ExtractZipFile(const std::filesystem::path &zipPath, const std::filesystem::path &extractPath)
{
    try
    {
        (void)Globals::APIDefs->Log(ELogLevel_DEBUG, "GW2RotaHelper", "Started ZIP Extracting");

        const auto psCommand = "powershell.exe -Command \"Expand-Archive -Path '" + zipPath.string() +
                               "' -DestinationPath '" + extractPath.string() + "' -Force\"";

        STARTUPINFOA si = {sizeof(si)};
        PROCESS_INFORMATION pi = {};
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        if (CreateProcessA(nullptr,
                           const_cast<char *>(psCommand.c_str()),
                           nullptr,
                           nullptr,
                           FALSE,
                           0,
                           nullptr,
                           nullptr,
                           &si,
                           &pi))
        {
            (void)Globals::APIDefs->Log(ELogLevel_INFO, "GW2RotaHelper", "Started ZIP extraction.");
            WaitForSingleObject(pi.hProcess, 30000); // Wait max 30 seconds
            DWORD exitCode;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            return exitCode == 0;
        }

        (void)Globals::APIDefs->Log(ELogLevel_CRITICAL, "GW2RotaHelper", "Create Process Failed.");
        return false;
    }
    catch (...)
    {
        Globals::BenchDataDownloadState = DownloadState::FAILED;
        (void)Globals::APIDefs->Log(ELogLevel_CRITICAL, "GW2RotaHelper", "ZIP extraction failed.");
        return false;
    }
}

void DownloadAndExtractDataAsync(const std::filesystem::path &addonPath)
{
    std::thread([addonPath]() {
        try
        {
            const std::string data_url =
                "https://github.com/franneck94/GW2_RotaHelper/releases/latest/download/GW2RotaHelper.zip";

            auto temp_zip_path = addonPath / "temp_GW2RotaHelper.zip";
            auto extract_path = addonPath.parent_path(); // Extract one level above
            (void)Globals::APIDefs->Log(ELogLevel_INFO, "GW2RotaHelper", "Started Download Thread.");

            if (DownloadFile(data_url, temp_zip_path))
            {
                std::filesystem::create_directories(addonPath);

                if (ExtractZipFile(temp_zip_path, extract_path))
                {
                    std::filesystem::remove(temp_zip_path);
                    Globals::BenchDataDownloadState = DownloadState::FINISHED;
                    Globals::ExtractedBenchData = true;

                    Settings::VersionOfLastBenchFilesUpdate = Globals::VersionString;
                    Settings::Save(Globals::SettingsPath);
                }
                else
                {
                    std::filesystem::remove(temp_zip_path);
                    Globals::BenchDataDownloadState = DownloadState::FAILED;
                }
            }
            else
            {
                Globals::BenchDataDownloadState = DownloadState::FAILED;
            }
        }
        catch (...)
        {
            Globals::BenchDataDownloadState = DownloadState::FAILED;
            (void)Globals::APIDefs->Log(ELogLevel_CRITICAL, "GW2RotaHelper", "DownloadAndExtractDataAsync failed.");
        }
    }).detach();
}
