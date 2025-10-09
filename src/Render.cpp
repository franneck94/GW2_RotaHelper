#include <windows.h>

#include <commdlg.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
#include <conio.h>
#include <d3d11.h>

#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <map>
#include <optional>
#include <queue>
#include <regex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <variant>

#include "imgui.h"
#include "nlohmann/json.hpp"

#include "Constants.h"
#include "Render.h"
#include "Shared.h"

using json = nlohmann::json;

struct SkillInfo
{
    std::string name;
    std::string icon_url;
};

using SkillInfoMap = std::map<std::string, SkillInfo>;
using LogDataTypes = std::variant<int, float, bool, std::string>;

struct IntNode
{
    std::map<std::string, IntNode> children;
    std::optional<LogDataTypes> value;
};

struct RotationInfo
{
    int skill_id;
    int cast_time;
    int duration;
    int idle_time;
    std::string skill_name;
};

using RotationInfoVec = std::vector<RotationInfo>;
using RotationInfoQueue = std::queue<RotationInfo>;

namespace
{
    std::vector<std::filesystem::path> get_bench_files(const std::filesystem::path &bench_path)
    {
        std::vector<std::filesystem::path> files;

        for (const auto &entry : std::filesystem::directory_iterator(bench_path))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".json")
            {
                files.push_back(entry.path());
            }
        }

        return files;
    }

    void collect_json(const json &jval, IntNode &node, const bool drop_first_char = false)
    {
        if (jval.is_object())
        {
            for (auto it = jval.begin(); it != jval.end(); ++it)
            {
                std::string key = it.key();
                if (drop_first_char && !key.empty())
                {
                    key = key.substr(1);
                }
                collect_json(it.value(), node.children[key]);
            }
        }
        else if (jval.is_array())
        {
            for (size_t i = 0; i < jval.size(); ++i)
            {
                collect_json(jval[i], node.children[std::to_string(i)]);
            }
        }
        else if (jval.is_number_integer())
        {
            node.value = jval.get<int>();
        }
        else if (jval.is_boolean())
        {
            node.value = jval.get<bool>();
        }
        else if (jval.is_number_float())
        {
            node.value = jval.get<float>();
        }
        else if (jval.is_string())
        {
            node.value = jval.get<std::string>();
        }
    }

    void get_skill_info(const IntNode &node, SkillInfoMap &skill_info_map)
    {
        for (const auto &[skill_id, skill_node] : node.children)
        {
            std::string name, icon;
            auto name_it = skill_node.children.find("name");
            if (name_it != skill_node.children.end() && name_it->second.value.has_value())
            {
                if (auto pval = std::get_if<std::string>(&name_it->second.value.value()))
                {
                    name = *pval;
                }
            }
            auto icon_it = skill_node.children.find("icon");
            if (icon_it != skill_node.children.end() && icon_it->second.value.has_value())
            {
                if (auto pval = std::get_if<std::string>(&icon_it->second.value.value()))
                {
                    icon = *pval;
                }
            }
            skill_info_map[skill_id] = {name, icon};
        }
    }

    void get_rotation_info(const IntNode &node, const SkillInfoMap &skill_info_map, RotationInfoVec &rotation_vector)
    {
        // The rotation data is structured as an array of arrays
        // Each inner array contains: [cast_time, skill_id, duration, ?, ?]
        for (const auto &rotation_entry : node.children)
        {
            const auto &rotation_array = rotation_entry.second;

            for (const auto &skill_entry : rotation_array.children)
            {
                const auto &skill_array = skill_entry.second;

                // Extract values from the array: [cast_time, skill_id, duration, ?, ?]
                float cast_time = 0.0f;
                int skill_id = 0;
                int duration = 0;

                // Get cast_time (index 0)
                auto cast_time_it = skill_array.children.find("0");
                if (cast_time_it != skill_array.children.end() && cast_time_it->second.value.has_value())
                {
                    if (auto pval = std::get_if<float>(&cast_time_it->second.value.value()))
                    {
                        cast_time = *pval;
                    }
                }

                // Get skill_id (index 1)
                auto skill_id_it = skill_array.children.find("1");
                if (skill_id_it != skill_array.children.end() && skill_id_it->second.value.has_value())
                {
                    if (auto pval = std::get_if<int>(&skill_id_it->second.value.value()))
                    {
                        skill_id = *pval;
                    }
                }

                // Get duration (index 2)
                auto duration_it = skill_array.children.find("2");
                if (duration_it != skill_array.children.end() && duration_it->second.value.has_value())
                {
                    if (auto pval = std::get_if<int>(&duration_it->second.value.value()))
                    {
                        duration = *pval;
                    }
                }

                // Skip invalid entries
                if (skill_id <= 0)
                    continue;

                // Get skill name from skill map
                std::string skill_name = "Unknown Skill";
                auto skill_info_it = skill_info_map.find(std::to_string(skill_id));
                if (skill_info_it != skill_info_map.end())
                {
                    skill_name = skill_info_it->second.name;
                }

                // Convert cast_time from seconds to milliseconds
                int cast_time_ms = static_cast<int>(cast_time * 1000);

                rotation_vector.push_back(RotationInfo{
                    .skill_id = skill_id,
                    .cast_time = cast_time_ms,
                    .duration = duration,
                    .idle_time = 0,
                    .skill_name = skill_name
                });
            }
        }

        // Sort by cast time
        std::sort(rotation_vector.begin(), rotation_vector.end(), [](const RotationInfo &a, const RotationInfo &b)
                  { return a.cast_time < b.cast_time; });

        // Calculate idle times
        for (size_t i = 1; i < rotation_vector.size(); ++i)
        {
            rotation_vector[i].idle_time = rotation_vector[i].cast_time - (rotation_vector[i - 1].cast_time + rotation_vector[i - 1].duration);
        }
    }

    std::tuple<SkillInfoMap, RotationInfoVec> get_dpsreport_data(const nlohmann::json &j)
    {
        const auto rotation_data = j["players"][0]["details"]["rotation"]; // TODO: player index
        const auto skill_data = j["skillMap"];

        auto kv_rotation = IntNode{};
        collect_json(rotation_data, kv_rotation);
        auto kv_skill = IntNode{};
        collect_json(skill_data, kv_skill, true);

        SkillInfoMap skill_info_map;
        get_skill_info(kv_skill, skill_info_map);
        RotationInfoVec rotation_info_vec;
        get_rotation_info(kv_rotation, skill_info_map, rotation_info_vec);

        return std::make_tuple(skill_info_map, rotation_info_vec);
    }

    std::string DownloadHTML(const std::string &url)
    {
        auto hInternet = InternetOpenA("SkillIconDownloader", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (!hInternet)
            return "";

        auto hFile = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (!hFile)
        {
            InternetCloseHandle(hInternet);
            return "";
        }

        std::string html;
        char buffer[4096];
        DWORD bytesRead = 0;
        do
        {
            if (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
            {
                html.append(buffer, bytesRead);
            }
        } while (bytesRead > 0);

        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);

        return html;
    }

    std::string ExtractBase64PNG(const std::string &html)
    {
        std::regex re("<img[^>]*src=['\"]data:image/png;base64,([^'\"]+)");
        std::smatch match;

        if (std::regex_search(html, match, re) && match.size() > 1)
            return match[1].str();

        return "";
    }

    std::vector<unsigned char> Base64Decode(const std::string &encoded_string)
    {
        static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        auto in_len = encoded_string.size();
        int i = 0, j = 0, in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::vector<unsigned char> ret;
        while (in_len-- && (encoded_string[in_] != '=') && isalnum(encoded_string[in_]) || encoded_string[in_] == '+' || encoded_string[in_] == '/')
        {
            char_array_4[i++] = encoded_string[in_];
            in_++;
            if (i == 4)
            {
                for (i = 0; i < 4; i++)
                    char_array_4[i] = static_cast<unsigned char>(base64_chars.find(char_array_4[i]));
                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
                for (i = 0; i < 3; i++)
                    ret.push_back(char_array_3[i]);
                i = 0;
            }
        }
        if (i)
        {
            for (j = i; j < 4; j++)
                char_array_4[j] = 0;
            for (j = 0; j < 4; j++)
                char_array_4[j] = static_cast<unsigned char>(base64_chars.find(char_array_4[j]));
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            for (j = 0; j < i - 1; j++)
                ret.push_back(char_array_3[j]);
        }
        return ret;
    }

    void SaveBinaryToFile(const std::filesystem::path &path, const std::vector<unsigned char> &data)
    {
        std::ofstream ofs(path, std::ios::binary);
        ofs.write(reinterpret_cast<const char *>(data.data()), data.size());
        ofs.close();
    }

    bool DownloadFileFromURL(const std::string &url, const std::filesystem::path &out_path)
    {
        const auto html = DownloadHTML(url);
        const auto base64 = ExtractBase64PNG(html);
        if (base64.empty())
            return false;

        const auto png_data = Base64Decode(base64);
        SaveBinaryToFile(out_path, png_data);

        return true;
    }

    std::queue<std::future<void>> StartDownloadAllSkillIcons(
        const SkillInfoMap &skill_info_map,
        const std::filesystem::path &img_folder)
    {
        std::filesystem::create_directories(img_folder);
        std::queue<std::future<void>> futures;

        for (const auto &[skill_id, info] : skill_info_map)
        {
            if (info.icon_url.empty())
                continue;

            std::string ext = ".png";
            const auto dot_pos = info.icon_url.find_last_of('.');
            if (dot_pos != std::string::npos && dot_pos + 1 < info.icon_url.size())
            {
                ext = info.icon_url.substr(dot_pos);
            }
            std::filesystem::path out_path = img_folder / (skill_id + ext);
            if (std::filesystem::exists(out_path))
                continue;

            std::cout << "Downloading " << info.icon_url << " to " << out_path << std::endl;
            futures.push(std::async(std::launch::async, [url = info.icon_url, out_path]()
                                         { DownloadFileFromURL(url, out_path); }));
        }

        return futures;
    }

    ID3D11ShaderResourceView *LoadTextureFromPNG_WIC(ID3D11Device *device, const std::wstring &filename)
    {
        IWICImagingFactory *factory = nullptr;
        IWICBitmapDecoder *decoder = nullptr;
        IWICBitmapFrameDecode *frame = nullptr;
        IWICFormatConverter *converter = nullptr;

        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr))
            return nullptr;

        hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                              IID_PPV_ARGS(&factory));
        if (FAILED(hr))
            return nullptr;

        hr = factory->CreateDecoderFromFilename(filename.c_str(), nullptr, GENERIC_READ,
                                                WICDecodeMetadataCacheOnLoad, &decoder);
        if (FAILED(hr))
        {
            factory->Release();
            return nullptr;
        }

        hr = decoder->GetFrame(0, &frame);
        if (FAILED(hr))
        {
            decoder->Release();
            factory->Release();
            return nullptr;
        }

        hr = factory->CreateFormatConverter(&converter);
        if (FAILED(hr))
        {
            frame->Release();
            decoder->Release();
            factory->Release();
            return nullptr;
        }

        hr = converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
                                   WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom);
        if (FAILED(hr))
        {
            converter->Release();
            frame->Release();
            decoder->Release();
            factory->Release();
            return nullptr;
        }

        UINT width, height;
        converter->GetSize(&width, &height);
        std::vector<BYTE> buffer(width * height * 4);
        converter->CopyPixels(nullptr, width * 4, static_cast<uint32_t>(buffer.size()), buffer.data());

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA subResource = {};
        subResource.pSysMem = buffer.data();
        subResource.SysMemPitch = width * 4;

        ID3D11Texture2D *texture = nullptr;
        ID3D11ShaderResourceView *srv = nullptr;
        hr = device->CreateTexture2D(&desc, &subResource, &texture);
        if (SUCCEEDED(hr))
            device->CreateShaderResourceView(texture, nullptr, &srv);

        if (texture)
            texture->Release();
        converter->Release();
        frame->Release();
        decoder->Release();
        factory->Release();
        CoUninitialize();

        return srv;
    }

    std::unordered_map<std::string, ID3D11ShaderResourceView *> LoadAllSkillTextures(
        ID3D11Device *device,
        const SkillInfoMap &skill_info_map,
        const std::filesystem::path &img_folder)
    {
        std::unordered_map<std::string, ID3D11ShaderResourceView *> texture_map;

        for (const auto &[skill_id, info] : skill_info_map)
        {
            if (info.name.empty())
                continue;
            std::string ext = ".png";
            size_t dot = info.icon_url.find_last_of('.');
            if (dot != std::string::npos && dot + 1 < info.icon_url.size())
            {
                ext = info.icon_url.substr(dot);
            }
            std::filesystem::path img_path = img_folder / (skill_id + ext);
            if (!std::filesystem::exists(img_path))
                continue;
            auto *tex = LoadTextureFromPNG_WIC(device, img_path.wstring());
            if (tex)
                texture_map[info.name] = tex;
        }
        return texture_map;
    }
}

class RotationRun
{
public:
    void load_data(const std::filesystem::path &json_path, const std::filesystem::path &img_path)
    {
        std::ifstream file(json_path);
        nlohmann::json j;
        file >> j;

        auto [_skill_info_map, _bench_rotation_vector] = get_dpsreport_data(j);
        skill_info_map = std::move(_skill_info_map);
        rotation_vector = std::move(_bench_rotation_vector);

        restart_rotation();

        futures = StartDownloadAllSkillIcons(skill_info_map, img_path);
    }

    void print_rotation_info() const
    {
        for (const auto &info : rotation_vector)
        {
            std::cout << "Skill ID: " << info.skill_id
                      << ", Name: " << info.skill_name
                      << ", CastTime: " << info.cast_time
                      << ", Duration: " << info.duration
                      << ", IdleTime: " << info.idle_time << std::endl;
        }
    }

    void print_skill_info() const
    {
        for (const auto &[skill_id, info] : skill_info_map)
        {
            std::cout << "Skill ID: " << skill_id
                      << ", Name: " << info.name
                      << ", Icon: " << info.icon_url << std::endl;
        }
    }

    void pop_bench_rotation_queue()
    {
        if (!bench_rotation_queue.empty())
        {
            bench_rotation_queue.pop();
        }
    }

    std::tuple<int, int, size_t> get_current_rotation_indices() const
    {
        if (bench_rotation_queue.empty())
            return {-1, -1, -1};

        const auto queue_size = bench_rotation_queue.size();
        const auto total_size = rotation_vector.size();
        const auto current_idx = total_size - queue_size;
        const auto start = static_cast<int32_t>(current_idx - 2);
        const auto end = static_cast<int32_t>(current_idx + 2);

        return {start, end, current_idx};
    }

    void print_current_rotation_slice() const
    {
        if (bench_rotation_queue.empty())
            return;

        const auto [start, end, current_idx] = get_current_rotation_indices();
        std::cout << "----------------------" << current_idx << "----------------------\n";
        for (int32_t i = start; i <= end; ++i)
        {
            if (i < 0 || static_cast<size_t>(i) >= rotation_vector.size())
            {
                std::cout << "   [none]" << std::endl;
                continue;
            }

            const auto &skill_info = get_rotation_skill(static_cast<size_t>(i));
            std::cout << (i == (int)current_idx ? "-> " : "   ");
            std::cout << " - " << skill_info.skill_name << std::endl;
        }
    }

    RotationInfo get_rotation_skill(const size_t idx) const
    {
        if (idx < rotation_vector.size())
            return rotation_vector.at(idx);

        return RotationInfo{};
    }

    void restart_rotation()
    {
        bench_rotation_queue = RotationInfoQueue(std::deque<RotationInfo>(rotation_vector.begin(), rotation_vector.end()));
    }

    bool is_current_run_done() const
    {
        return bench_rotation_queue.empty();
    }

    std::queue<std::future<void>> futures;

private:
    SkillInfoMap skill_info_map;
    RotationInfoVec rotation_vector;
    RotationInfoQueue bench_rotation_queue;
};

bool Render::OpenFileDialog()
{
#ifdef _WIN32
    OPENFILENAMEA ofn;
    char szFile[260] = {0};

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "All Files\0*.*\0ARCDPS Logs\0*.evtc\0Compressed Logs\0*.zevtc\0\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn) == TRUE)
    {
        selected_file_path = std::string(szFile);
        return true;
    }
#endif
    return false;
}

bool Render::ParseEvtcFile(const std::string &filePath)
{
    std::string cliPath = "GuildWars2EliteInsights-CLI.exe";
    std::string configPath = "ei_config.conf";
    std::string command = cliPath + " -c  " + configPath + " " + filePath;

    int result = system(command.c_str());

    if (result == 0)
    {
        std::string jsonPath = filePath;
        size_t lastDot = jsonPath.find_last_of('.');
        if (lastDot != std::string::npos)
        {
            jsonPath = jsonPath.substr(0, lastDot) + ".json";
        }
        else
        {
            jsonPath += ".json";
        }

        try
        {
            std::ifstream jsonFile(jsonPath);
            if (jsonFile.is_open())
            {
                nlohmann::json logData;
                jsonFile >> logData;

                if (logData.contains("fightName"))
                {
                    std::string fightName = logData["fightName"];
                }

                if (logData.contains("players"))
                {
                    auto players = logData["players"];
                }

                jsonFile.close();
            }
        }
        catch (const std::exception &e)
        {
            return false;
        }

        return true;
    }

    return false;
}

void Render::render()
{
    if (!show_window)
        return;

    static int selected_bench_index = -1;
    static auto data_path = std::filesystem::absolute(std::filesystem::path(__FILE__).parent_path().parent_path() / "data");
    static auto img_path = data_path / "img";
    static auto bench_path = data_path / "bench";
    static auto benches_files = get_bench_files(bench_path);
    static std::filesystem::path selected_file_path;
    static RotationRun rotation_run;

    if (ImGui::Begin("GW2RotaHelper", &show_window))
    {
        if (!benches_files.empty())
        {
            ImGui::Text("Select Bench File:");
            if (ImGui::BeginCombo("##benches_combo", selected_bench_index >= 0 ? benches_files[selected_bench_index].filename().string().c_str() : "Select..."))
            {
                for (int n = 0; n < benches_files.size(); n++)
                {
                    bool is_selected = (selected_bench_index == n);
                    if (ImGui::Selectable(benches_files[n].filename().string().c_str(), is_selected))
                    {
                        selected_bench_index = n;
                        selected_file_path = bench_path / std::filesystem::path(benches_files[n]);

                        rotation_run.load_data(selected_file_path, img_path);
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }

        if (!selected_file_path.empty())
        {
            ImGui::SameLine();
            ImGui::Text("Selected: %s", selected_file_path.c_str());
        }

        if (rotation_run.futures.size() != 0)
        {
            if (rotation_run.futures.front().valid())
            {
                rotation_run.futures.front().get();
            }
            rotation_run.futures.pop();

            ImGui::End();

            return;
        }

        ImGui::Separator();

        ImGui::Text("Combat Events Buffer (last 10):");
        ImGui::BeginChild("CombatBufferChild", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::Columns(2, "cb_columns", true);
        ImGui::Text("Log Skill");
        ImGui::NextColumn();
        ImGui::Text("User Skill");
        ImGui::NextColumn();
        ImGui::Separator();

        const auto start_index = (combat_buffer_index >= 10) ? (combat_buffer_index - 10) : (combat_buffer.size() + combat_buffer_index - 10);
        if (start_index == 0)
            return;

        for (int i = 0; i < 10; ++i)
        {
            const auto &skill_info = rotation_run.get_rotation_skill(static_cast<size_t>(i));
            ImGui::Text("%s", skill_info.skill_name.empty() ? "N/A" : skill_info.skill_name.c_str());
            ImGui::NextColumn();

            const auto index = (start_index + i) % combat_buffer.size();
            const auto &entry = combat_buffer[index];
            ImGui::Text("%s", entry.SkillName.empty() ? "N/A" : entry.SkillName.c_str());
            ImGui::NextColumn();
        }

        ImGui::EndChild();
    }

    ImGui::End();
}
