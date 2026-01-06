#include <map>
#include <set>
#include <string>
#include <string_view>

#include "Builds.h"
#include "Types.h"

namespace
{

static const inline std::set<std::string_view> red_crossed_builds = {
    // CHECKED POWER BUILDS
    "power_amalgam",
    "power_alacrity_amalgam",
    "power_bladesworn",
    "power_alacrity_bladesworn_overcharged",
    "power_alacrity_bladesworn",
    "power_holosmith",
    "power_deadeye_staff_and_dagger",
    // CHECKED CONDITION BUILDS
    "condition_amalgam_steamshrieker",
    "condition_alacrity_amalgam_two_kits",
    "condition_daredevil",
    "condition_firebrand",
    "condition_quickness_firebrand",
    "condition_mirage_dagger",
    "condition_mirage_dune_cloak",
    "condition_mirage_ih_ether",
    "condition_mirage_ih_oasis",
    "condition_alacrity_mirage_staff",
    "condition_alacrity_mirage_axe_torch_staff",
};

static const inline std::set<std::string_view> orange_crossed_builds = {
    // CHECKED POWER BUILDS
    "power_tempest_hammer",
    "inferno_quickness_evoker_specialized_elements",
    "inferno_evoker_specialized_elements",
    // CHECKED CONDITION BUILDS
    "condition_catalyst",
    "condition_quickness_catalyst",
    // POWER BUILDS
    "condition_boon_chronomancer",
    "condition_chronomancer",
    "power_vindicator",
    "power_quickness_untamed",
    "power_quickness_untamed_maces",
    "power_quickness_untamed_offhand_mace",
    "power_quickness_catalyst_sword_dagger_pf",
    "power_catalyst_sword_dagger_pf",
    "power_luminary",
    "power_alacrity_luminary",
    "power_quickness_evoker",
    // CONDI BUILDS
    "condition_virtuoso",
    "condition_quickness_untamed",
    "condition_quickness_catalyst_pistol_warhorn",
};

static const inline std::set<std::string_view> yellow_tick_builds = {
    // CHECKED POWER BUILDS
    "power_ritualist",
    "power_tempest_sword",
    "power_tempest",
    "power_tempest_inferno_scepter_dagger",
    "inferno_catalyst_scepter_dagger",
    "power_catalyst_scepter_dagger_inferno",
    "power_catalyst_scepter_dagger",
    "power_catalyst_scepter_dagger_pf",
    "power_virtuoso_spear_greatsword",
    "power_virtuoso_dagger_sword_greatsword",
    "power_virtuoso",
    "power_troubadour",
    "inferno_tempest_scepter_dagger",
    "power_herald",                         // maybe green
    "power_conduit",                        // maybe green
    "power_conduit_greatsword_sword_sword", // maybe green
    "power_quickness_herald",               // maybe green
    "power_quickness_herald_sword",         // maybe green
    "power_scrapper",                       // optimize again to go green
    "power_willbender",
    "power_dragonhunter",
    "power_dragonhunter_virtues_spear",
    "power_dragonhunter_radiance_longbow",
    // CHECKED POWER BOON BUILDS
    "power_quickness_ritualist",
    "power_quickness_catalyst_scepter_dagger",
    "power_quickness_catalyst_scepter_dagger_inferno",
    "inferno_quickness_catalyst_scepter_dagger",
    "power_quickness_catalyst_scepter_dagger",
    "power_quickness_catalyst_scepter_dagger_pf",
    "power_inferno_quickness_catalyst_scepter_dagger_pf",
    "power_alacrity_tempest",
    "power_alacrity_tempest_inferno_scepter_focus",
    "power_quickness_scrapper",
    "power_weaver",
    // CHECKED CONDITION BOON BUILDS
    "celestial_alacrity_scourge",
    "condition_alacrity_scourge",
    "condition_alacrity_renegade",
    "condition_quickness_scrapper",
    // CHECKED CONDITION BUILDS
    "condition_berserker",
    "condition_reaper",
    "condition_scourge",
    "condition_weaver_scepter",
    "condition_weaver_pistol",
    "condition_evoker",
    "condition_tempest_scepter",
    "condition_holosmith_spear",
    "condition_renegade",
    "condition_mechanist_kitless",
    // UNCHECKED POWER BUILDS
    "power_untamed",
    "power_untamed_sword_axe",
    "power_chronomancer",
    "power_evoker_scepter_dagger",
    "power_alacrity_evoker_scepter_dagger",
    // UNCHECKED POWER BOON BUILDS
    "power_alacrity_tempest_inferno_scepter_focus",
    "power_boon_chronomancer",
    "power_alacrity_tempest_hammer",
    "power_alacrity_renegade",
    "condition_paragon",
    // UNCHECKED CONDITION BUILDS
    "condition_druid",
    "condition_thief",
    "condition_thief_spear",
    "condition_tempest",
    "condition_soulbeast",
    "condition_soulbeast_shortbow",
    "condition_soulbeast_quickdraw",
    // UNCHECKED CONDI BOON BUILDS
    "condition_alacrity_tempest_scepter",
    "condition_alacrity_tempest",
    "condition_quickness_herald_shortbow",
    "condition_quickness_herald_spear",
    "condition_quickness_herald",
};

static const inline std::set<std::string_view> green_tick_builds = {
    // POWER BUILDS
    "power_berserker",
    "power_warrior",
    "power_spellbreaker",
    "power_berserker_axe_axe_axe_mace",
    "power_berserker_greatsword",
    "power_reaper",
    "power_reaper_greatsword_sword_sword",
    "power_harbinger_greatsword_dagger_sword",
    "power_harbinger_greatsword_sword_sword",
    "power_mechanist",
    "power_mechanist_sword",
    "power_harbinger",
    "power_reaper_spear",
    "power_paragon",
    "power_galeshot",
    "power_soulbeast_hammer",
    "power_spellbreaker_hammer",
    "power_berserker_hammer_axe_mace",
    // POWER BOON BUILDS
    "power_quickness_berserker",
    "power_alacrity_mechanist_sword",
    "power_quickness_berserker_greatsword",
    "power_alacrity_mechanist",
    "power_quickness_harbinger",
    "power_quickness_galeshot",
    // CONDITION BUILDS
    "condition_harbinger",
    "condition_mechanist",
    "condition_mechanist_two_kits",
    "condition_willbender_scepter",
    "condition_willbender",
    "condition_conduit",
    // CONDI BOON BUILDS
    "condition_quickness_harbinger",
    "condition_alacrity_mechanist_1_kit",
    "condition_alacrity_mechanist",
};
} // namespace

void BuildsType::initialize_build_categories()
{
    if (build_categories_initialized)
        return;

    build_category_cache.clear();

    for (const auto &build : red_crossed_builds)
        build_category_cache[std::string(build)] = BuildCategory::RED_CROSSED;

    for (const auto &build : orange_crossed_builds)
        build_category_cache[std::string(build)] = BuildCategory::ORANGE_CROSSED;

    for (const auto &build : green_tick_builds)
        build_category_cache[std::string(build)] = BuildCategory::GREEN_TICKED;

    for (const auto &build : yellow_tick_builds)
        build_category_cache[std::string(build)] = BuildCategory::YELLOW_TICKED;

    build_category_cache["antiquary"] = BuildCategory::RED_CROSSED;
    build_category_cache["daredevil"] = BuildCategory::ORANGE_CROSSED;
    build_category_cache["deadeye"] = BuildCategory::ORANGE_CROSSED;

    build_categories_initialized = true;
}

BuildCategory BuildsType::get_build_category(const std::string &display_name) const
{
    auto build_name = display_name.length() > 4 ? display_name.substr(4) : display_name;

    auto it = build_category_cache.find(build_name);
    if (it != build_category_cache.end())
        return it->second;

    if (display_name.find("antiquary") != std::string::npos)
        return BuildCategory::RED_CROSSED;

    if (display_name.find("daredevil") != std::string::npos || display_name.find("deadeye") != std::string::npos)
        return BuildCategory::ORANGE_CROSSED;

    return BuildCategory::UNTESTED;
}
