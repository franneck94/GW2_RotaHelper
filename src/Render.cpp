#include <windows.h>

#include <commdlg.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
#include <d3d11.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <numbers>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#include "imgui.h"

#include "mumble/Mumble.h"

#include "ArcEvents.h"
#include "Defines.h"
#include "FileUtils.h"
#include "LogData.h"
#include "MumbleUtils.h"
#include "Render.h"
#include "RenderUtils.h"
#include "Rotation.h"
#include "Settings.h"
#include "Shared.h"
#include "SkillData.h"
#include "Textures.h"
#include "Types.h"
#include "TypesUtils.h"
#include "Version.h"

namespace
{
static auto last_time_aa_did_skip = std::chrono::steady_clock::time_point{};
} // namespace

RenderType::~RenderType()
{
    ReleaseTextureMap(Globals::TextureMap);
}

void RenderType::set_data_path(const std::filesystem::path &path)
{
    Globals::RenderData.data_path = path;
    Globals::RenderData.img_path = Globals::RenderData.data_path / "img";
    Globals::RenderData.bench_path = Globals::RenderData.data_path / "bench";

    Globals::RenderData.builds.initialize_build_categories();
    Globals::RenderData.benches_files = get_bench_files(Globals::RenderData.bench_path);
}

void RenderType::append_to_played_rotation(const EvCombatDataPersistent &combat_data)
{
    if (Globals::RenderData.played_rotation.size() > 300)
    {
        auto last_100 = std::vector<EvCombatDataPersistent>(Globals::RenderData.played_rotation.end() - 100,
                                                            Globals::RenderData.played_rotation.end());
        Globals::RenderData.played_rotation = std::move(last_100);
    }

    Globals::RenderData.played_rotation.push_back(combat_data);
}

void RenderType::skill_activation_callback(EvCombatDataPersistent combat_data)
{
    if (Globals::RenderData.curr_combat_data.SkillID == combat_data.SkillID)
        combat_data.RepeatedSkill = true;

    Globals::RenderData.skill_event_in_this_frame = true;
    Globals::RenderData.curr_combat_data = combat_data;

    append_to_played_rotation(combat_data);
}

EvCombatDataPersistent RenderType::get_current_skill()
{
    if (!Globals::RenderData.skill_event_in_this_frame)
    {
        auto skill_ev = EvCombatDataPersistent{};
        skill_ev.SkillID = SkillID::FALLBACK;
        return skill_ev;
    }

    if (Globals::RenderData.curr_combat_data.SkillID == SkillID::NONE)
    {
        auto skill_ev = EvCombatDataPersistent{};
        skill_ev.SkillID = SkillID::FALLBACK;
        return skill_ev;
    }

    return Globals::RenderData.curr_combat_data;
}

void RenderType::set_show_window(const bool flag)
{
    show_window = flag;
}

void RenderType::CycleSkillsLogic(const EvCombatDataPersistent &skill_ev)
{
    if (Globals::RotationRun.missing_rotation_steps.empty())
        return;

    if (skill_ev.SkillID == SkillID::NONE || skill_ev.SkillID == SkillID::FALLBACK)
        return;

    const auto debug_msg = std::string{"Player Casted Skill: "} + skill_ev.SkillName;
    (void)Globals::APIDefs->Log(LOGL_DEBUG, "GW2RotaHelper", debug_msg.c_str());

    SkillDetectionLogic(num_skills_wo_match, time_since_last_match, Globals::RotationRun, skill_ev);
}

void RenderType::restart_rotation(const bool not_ooc_triggered)
{
    Globals::RotationRun.restart_rotation();

    Globals::RenderData.played_rotation.clear();
    Globals::RenderData.curr_combat_data = EvCombatDataPersistent{};

    time_since_last_match = std::chrono::steady_clock::now();
    num_skills_wo_match = 0U;

    last_time_aa_did_skip = std::chrono::steady_clock::now();
    ArcEv::ResetSkillCastTracking();
    Globals::SkillLastTimeCast.clear();

    if (not_ooc_triggered)
    {
        Globals::RenderData.show_rotation_window = true;
        Globals::RenderData.show_rotation_keybinds = false;
        Globals::RenderData.show_rotation_icons_overview = false;
        Globals::RenderData.do_highlight_skill = false;
        Globals::RenderData.highlight_skill_id = SkillID::NONE;
    }
}

void RenderType::render(ID3D11Device *pd3dDevice)
{
    Globals::RenderData.pd3dDevice = pd3dDevice;

    static auto time_went_ooc = std::chrono::steady_clock::now();

    if (!Settings::ShowWindow)
        return;

    KeypressSkillDetectionLogic(Globals::RotationRun);

    if (Globals::RenderData.skill_event_in_this_frame)
    {
        const auto skill_ev = get_current_skill();
        Globals::RenderData.skill_event_in_this_frame = false;
        CycleSkillsLogic(skill_ev);
    }

    const auto curr_is_infight = IsInfight();
    if (!curr_is_infight)
    {
        auto now = std::chrono::steady_clock::now();
        auto time_since_went_ooc_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - time_went_ooc).count();

        if (time_since_went_ooc_ms > 1000)
        {
            restart_rotation(false);
            time_went_ooc = std::chrono::steady_clock::now();
        }
    }
    else
    {
        time_went_ooc = std::chrono::steady_clock::now();
    }

    if (Globals::RenderData.benches_files.size() == 0)
        Globals::RenderData.benches_files = get_bench_files(Globals::RenderData.bench_path);

    Globals::OptionsRender.render();

    if (Globals::RotationRun.futures.size() != 0)
    {
        if (Globals::RotationRun.futures.front().valid())
        {
            Globals::RotationRun.futures.front().get();
            Globals::RotationRun.futures.pop_front();
        }

        return;
    }

    if (Globals::RotationRun.all_rotation_steps.size() == 0)
        return;

    if (Globals::TextureMap.size() == 0)
        Globals::TextureMap =
            LoadAllSkillTextures(pd3dDevice, Globals::RotationRun.log_skill_info_map, Globals::RenderData.img_path);

    if (!IsValidMap())
        return;

    if (Globals::RenderData.show_rotation_window)
        Globals::RotationRender.render();
}
