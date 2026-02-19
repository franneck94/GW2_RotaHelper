#include <map>
#include <set>
#include <string>
#include <string_view>

#include "Builds.h"
#include "Types.h"

namespace
{

static const std::set<std::string_view> red_crossed_builds = {
    // CHECKED POWER BUILDS
    "power_amalgam_hammer",
    "power_alacrity_amalgam_hammer",
    "power_holosmith_sword_pistol",
    // CHECKED CONDITION BUILDS
    "condition_amalgam_one_kit_spear",
    "condition_alacrity_amalgam_steamshrieker_spear",
    "condition_alacrity_amalgam_two_kits_spear",
    "condition_firebrand_axe_torch_pistol_pistol",
    "condition_quickness_firebrand_pistol_torch_pistol",
};

static const std::set<std::string_view> orange_crossed_builds = {
    // CHECKED POWER BUILDS
    "power_tempest_hammer",
    "power_vindicator_greatsword_sword_sword",
    "power_quickness_untamed_hammer_sword_axe",
    "power_quickness_untamed_hammer_mace_mace",
    "power_quickness_untamed_hammer_sword_mace",
    "power_quickness_catalyst_sword_dagger",
    "power_catalyst_sword_dagger",
    "power_quickness_evoker_scepter_dagger",
    // CONDI BUILDS
    "inferno_quickness_evoker_specialized_elements",
    "inferno_evoker_specialized_elements",
    "condition_boon_chronomancer_staff_scepter_torch",
    "condition_chronomancer_staff_scepter_torch",
    "condition_virtuoso_dagger_sword_focus",
    "condition_quickness_untamed_axe_dagger_dagger_torch",
    "condition_quickness_catalyst_pistol_warhorn",
    "condition__pistol_dagger",
    "condition_quickness_catalyst_pistol_dagger",
};

static const std::set<std::string_view> yellow_tick_builds = {
    // CHECKED POWER BUILDS
    "inferno_catalyst_scepter_dagger",
    "inferno_tempest_scepter_dagger",
    "power_ritualist_greatsword_spear",
    "power_tempest_sword_dagger",
    "power_tempest_scepter_dagger",
    "power_tempest_inferno_scepter_dagger",
    "power_catalyst_scepter_dagger_inferno",
    "power_catalyst_scepter_dagger",
    "power_catalyst_scepter_dagger_pf",
    "power_virtuoso_spear_greatsword",
    "power_virtuoso_greatsword_dagger_sword",
    "power_virtuoso_spear_dagger_sword",
    "power_troubadour_spear_dagger_sword",
    "power_conduit_staff_sword_sword",               // maybe green
    "power_conduit_greatsword_sword_sword",          // maybe green
    "power_herald_staff_sword_sword",                // maybe green
    "power_quickness_herald_greatsword_sword_sword", // maybe green
    "power_quickness_herald_staff_sword_sword",      // maybe green
    "power_scrapper_hammer",                         // optimize again to go green
    "power_willbender",
    "power_dragonhunter_virtues_spear_greatsword",
    "power_dragonhunter_radiance_spear_greatsword",
    "power_dragonhunter_radiance_longbow_greatsword",
    "power_untamed_hammer_spear",
    "power_untamed_hammer_sword_axe",
    "power_chronomancer_spear_dagger_sword",
    "power_evoker_scepter_dagger",
    "power_alacrity_evoker_scepter_dagger",
    "power_weaver_sword_dagger",
    "power_reaper_dagger_sword",
    "power_reaper_sword_sword",
    "power_luminary_spear_greatsword",
    "power_renegade_greatsword_sword_sword",
    "power_dragonhunter_lb",
    //  POWER BOON BUILDS
    "power_alacrity_willbender_greatsword_sword_focus",
    "power_alacrity_willbender_spear_greatsword",
    "power_alacrity_luminary_longbow_greatsword",
    "power_alacrity_luminary_spear_greatsword",
    "power_boon_chronomancer_spear_dagger_sword",
    "inferno_quickness_catalyst_scepter_dagger",
    "power_inferno_quickness_catalyst_scepter_dagger_pf",
    "power_quickness_ritualist_greatsword_spear",
    "power_quickness_catalyst_scepter_dagger",
    "power_quickness_catalyst_scepter_dagger_inferno",
    "power_quickness_catalyst_scepter_dagger",
    "power_quickness_catalyst_scepter_dagger_pf",
    "power_quickness_scrapper_hammer",
    "power_alacrity_tempest",
    "power_alacrity_tempest_inferno_scepter_focus",
    "power_alacrity_tempest_inferno_scepter_focus",
    "power_alacrity_tempest_hammer",
    "power_alacrity_renegade_staff_sword_sword",
    // CONDITION BOON BUILDS
    "celestial_alacrity_scourge_dagger_torch_pistol_warhorn",
    "condition_alacrity_scourge_scepter_torch_pistol",
    "condition_alacrity_renegade_spear_mace_axe",
    "condition_quickness_scrapper_spear",
    "condition_harbinger_pistol_torch_scepter_dagger",
    "condition_druid_dagger_torch_axe_dagger",
    "condition_thief",
    "condition_thief_spear",
    "condition_soulbeast_axe_dagger_dagger_torch",
    "condition_soulbeast_shortbow_dagger_dagger",
    "condition_soulbeast_axe_torch_dagger_axe",
    "condition_willbender_scepter_pistol_pistol_torch",
    "condition_willbender_pistol_torch_pistol",
    "condition_berserker_longbow_sword_torch",
    "condition_reaper_greatsword_spear",
    "condition_scourge_scepter_torch_pistol",
    "condition_weaver_scepter_warhorn",
    "condition_weaver_pistol_warhorn",
    "condition_tempest_scepter_warhorn",
    "condition_tempest_pistol_warhorn",
    "condition_holosmith_spear",
    "condition_renegade_spear_mace_axe",
    "condition_renegade_spear_shortbow",
    "condition_mechanist_no_kit_spear",
    "condition_paragon_longbow_sword_sword",
    // BOON CONDITION BUILDS
    "condition_quickness_harbinger_pistol_dagger_scepter_torch",
    "condition_alacrity_tempest_scepter",
    "condition_alacrity_tempest",
    "condition_quickness_herald_shortbow_mace_axe",
    "condition_quickness_herald_spear_mace_axe",
    "condition_quickness_herald_spear_only",
};

static const std::set<std::string_view> green_tick_builds = {
    // POWER BUILDS
    "power_berserker_axe_axe_mace",
    "power_berserker_spear_axe_axe",
    "power_berserker_axe_axe_axe_mace",
    "power_berserker_greatsword_axe_axe",
    "power_berserker_hammer_axe_mace",
    "power_reaper_greatsword_dagger_sword",
    "power_reaper_greatsword_sword_sword",
    "power_harbinger_greatsword_dagger_sword",
    "power_harbinger_greatsword_sword_sword",
    "power_harbinger_greatsword_spear",
    "power_mechanist_rifle",
    "power_mechanist_sword_pistol",
    "power_reaper_greatsword_spear",
    "power_paragon_sword_axe_dagger_mace",
    "power_warrior_sword_axe_dagger_mace",
    "power_galeshot_longbow_axe_axe",
    "power_soulbeast_hammer_axe_axe",
    "power_spellbreaker_sword_mace_dagger_axe",
    "power_spellbreaker_hammer_dagger_mace",
    "power_berserker_tactics_spear_greatsword",
    // POWER BOON BUILDS
    "power_quickness_harbinger_greatsword_spear",
    "power_alacrity_mechanist_sword_pistol",
    "power_alacrity_mechanist_rifle",
    "power_quickness_berserker_spear_axe_axe",
    "power_quickness_berserker_greatsword_axe_axe",
    "power_quickness_harbinger",
    "power_quickness_berserker_tactics_spear_greatsword",
    "power_quickness_galeshot_longbow_axe_axe",
    // CONDITION BUILDS
    "condition_harbinger",
    "condition_mechanist_one_kit_spear",
    "condition_mechanist_two_kits_spear",
    "condition_conduit_mace_axe",
    "condition_conduit_spear_mace_axe",
    // CONDI BOON BUILDS
    "condition_quickness_harbinger",
    "condition_alacrity_mechanist_one_kit_spear",
    "condition_alacrity_mechanist_two_kits_spear",
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

    build_categories_initialized = true;
}

BuildCategory BuildsType::get_build_category(const std::string &display_name) const
{
    auto build_name = display_name.length() > 4 ? display_name.substr(4) : display_name;

    if (build_name.contains("antiquary"))
        return BuildCategory::RED_CROSSED;
    if (build_name.contains("daredevil"))
        return BuildCategory::ORANGE_CROSSED;
    if (build_name.contains("deadeye"))
        return BuildCategory::ORANGE_CROSSED;
    if (build_name.contains("bladesworn"))
        return BuildCategory::RED_CROSSED;
    if (build_name.contains("mirage"))
        return BuildCategory::RED_CROSSED;

    for (const auto &[name, category] : build_category_cache)
    {
        if (name.starts_with(build_name))
            return category;
    }

    return BuildCategory::UNTESTED;
}
