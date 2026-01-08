#include <map>
#include <set>
#include <string>
#include <string_view>

#include "MumbleUtils.h"
#include "SkillData.h"

#include "Types.h"

namespace SkillRuleData
{

const std::set<std::string_view> skills_match_weapon_swap_like = {
    "Weapon Swap",
    // GUARDIAN
    "Radiant Forge",
    "Enter Radiant Forge",
    "Exit Radiant Forge",
    // ENGINEER
    "Photon Forge",
    "Exit Photon Forge",
    "Holoforge",
    "Exit Holoforge",
    "Flamethrower",
    "Elixir Gun",
    "Bomb Kit",
    "Grenade Kit",
    // RANGER
    "Celestial Avatar",
    "Summon Cyclone Bow",
    "Dismiss Cyclone Bow",
    "Unleash Ranger",
    "Unleash Pet",
    // ELEMENTALIST
    "Fire Attunement",
    "Earth Attunement",
    "Air Attunement",
    "Water Attunement",
    // NECROMANCER
    "Harbinger Shroud",
    "Exit Harbinger Shroud",
    "Reaper's Shroud",
    "Exit Reaper's Shroud",
    "Ritualist's Shroud",
    "Exit Ritualist's Shroud",
    // REVENANT
    "Legendary Entity Stance",
    "Legendary Alliance Stance",
    "Legendary Alliance",
    "Legendary Renegade Stance",
    "Legendary Demon Stance",
    "Legendary Dwarf Stance",
    "Legendary Centaur Stance",
    "Legendary Assassin Stance",
    "Legendary Dragon Stance",
};

const std::set<std::string_view> skills_substr_weapon_swap_like = {
    // GUARDIAN
    // WARRIOR
    "Berserk",
    "Sheathe Gunsaber",
    "Unsheathe Gunsaber",
    "Arcing Slice",
    // ENGINEER
    " Kit",
    // RANGER
    "Ranger Pet",
    // THIEF
    // ELEMENTALIST
    // MESMER
    // NECROMANCER
    // REVENANT
};

/* In Genreal Traits/Effects that are in the log */
const std::set<std::string_view> skills_substr_to_drop = {
    "Bloodstone Fervor",
    "Relic of",
    "Nourys's Hunger",
    "Relikt des ",
    "Mushroom King",
    "Superior Sigil",
    // GUARDIAN
    "Fire Jurisdiction",
    "Rushing Justice (Hit)",
    // WARRIOR
    // ENGINEER
    // RANGER
    // THIEF
    // ELEMENTALIST
    "Flame Expulsion",
    // MESMER
    // NECROMANCER
    // REVENANT
    "Form of the ",
};

/* In Genreal Traits/Effects that are in the log */
const std::set<std::string_view> skills_match_to_drop = {
    "Doom",
    // GUARDIAN
    // WARRIOR
    "King of Fires",
    "Magebane Tether",
    // ENGINEER
    "Focused Devastation",
    "Explosive Entrance",
    "Spark Revolver",
    "Fire Rocket Barrage",
    "Orbital Command Strike",
    "Bomb",
    // RANGER
    "Wuthering Wind",
    "Unleashed Overbearing Smash (Leap)",
    "Quick Draw",
    // THIEF
    "Impaling Lotus",
    // ELEMENTALIST
    "Sunspot",
    "Earthen Blast",
    "Flame Explosion",
    // MESMER
    "Mirage Cloak",
    "Syncopate",
    "Syncopate (Delay Wave)",
    // NECROMANCER
    "Approaching Doom",
    "Chilling Nova",
    "Cascading Corruption",
    "Explosive Growth",
    "Deathly Haste",
    // REVENANT
    "Invoke Torment",
    "Mistfire",
};

const std::set<std::string_view> special_substr_to_gray_out = {
    // GUARDIAN
    "Chapter ",
    "Tome of ", // 10 different tomes
    " Radiant Forge",
    // WARRIOR
    "Dragon Slashâ€”",
    // ENGINEER
    "Detonate",
    "Photon Forge",
    "-Storm",
    "Offensive Protocol: ",
    "Defensive Protocol: ",
    "Rifle Burst Grenade",
    // RANGER
    "Blood Moon",
    // THIEF
    // ELEMENTALIST
    "Deploy Jade Sphere", // There are 20 different skills like this
    // MESMER
    "Distortion",
    // NECROMANCER
    // REVENANT
    "True Nature",
};

const std::set<std::string_view> special_match_to_gray_out_names = {
    "Dodge",
    // Warrior
    "Cyclone Trigger",
    "Steel Divide",
    // ELEMENTALIST
    "Zap",
    "Splash",
    "Calcify",
    // ENGINEER
    "Mine Field",
    "Reconstruction Field",
};

const std::set<SkillID> special_match_to_gray_out = {
    // GUARDIAN
    SkillID::ZEALOT_S_FLAME,
    SkillID::RUSHING_JUSTICE,
    SkillID::SYMBOL_OF_PUNISHMENT,
    SkillID::FLOWING_RESOLVE,
    SkillID::JURISDICTION,
    SkillID::ZEALOT_S_FLAME,
    SkillID::ZEALOT_S_FLAME_1,
    SkillID::SYMBOL_OF_RESOLUTION,
    SkillID::SWORD_OF_JUSTICE,
    SkillID::SWORD_OF_JUSTICE_1,
    SkillID::SWORD_OF_JUSTICE_2,
    SkillID::SWORD_OF_JUSTICE_3,
    SkillID::SOLAR_STORM,
    SkillID::PROCESSION_OF_BLADES,
    SkillID::DRAGON_S_MAW,
    SkillID::RADIANT_JUSTICE,
    SkillID::RADIANT_RESOLVE,
    SkillID::RADIANT_RESOLVE_1,
    SkillID::RADIANT_COURAGE,
    SkillID::RADIANT_COURAGE_1,
    // WARRIOR
    SkillID::DRAGON_TRIGGER,
    SkillID::FLOW_STABILIZER,
    SkillID::TACTICAL_RELOAD,
    SkillID::SIGNET_OF_FURY,
    SkillID::SIGNET_OF_MIGHT,
    SkillID::SIGNET_OF_RAGE,
    SkillID::BERSERK,
    SkillID::BLOOD_RECKONING,
    SkillID::OUTRAGE,
    SkillID::OVERCHARGED_CARTRIDGES,
    SkillID::ARCING_SLICE,
    SkillID::CHANT_OF_ACTION,
    SkillID::FLAMES_OF_WAR,
    // ENGINEER
    SkillID::DEVASTATOR,
    SkillID::FLAME_BLAST,
    SkillID::AIR_BLAST,
    SkillID::OVERCHARGED_SHOT,
    SkillID::SUPERCONDUCTING_SIGNET,
    SkillID::OVERCLOCK_SIGNET,
    SkillID::LIGHTNING_ROD,
    SkillID::RADIANT_ARC,
    SkillID::NAPALM,
    SkillID::GLUE_SHOT,
    SkillID::ACID_BOMB,
    SkillID::FLAME_BLAST,
    SkillID::AIR_BLAST,
    SkillID::EVOLVE,
    SkillID::EVOLVE_1,
    SkillID::ROCKET_PUNCH,
    SkillID::ROCKET_PUNCH_1,
    SkillID::ROLLING_SMASH,
    SkillID::DISCHARGE_ARRAY,
    SkillID::SKY_CIRCUS,
    SkillID::BARRIER_BURST,
    SkillID::JADE_MORTAR,
    SkillID::JADE_MORTAR_1,
    SkillID::CRISIS_ZONE,
    SkillID::CORE_REACTOR_SHOT,
    SkillID::CORE_REACTOR_SHOT_1,
    SkillID::SPARK_REVOLVER,
    SkillID::CORONA_BURST,
    SkillID::PHOTON_BLITZ,
    SkillID::HOLO_LEAP,
    SkillID::ELIXIR_SHELL,
    // RANGER
    SkillID::FLAME_TRAP,
    SkillID::FROST_TRAP,
    SkillID::SIC_EM,
    SkillID::SHARPENING_STONE,
    SkillID::SUN_SPIRIT,
    SkillID::ENTANGLE,
    SkillID::MISTRAL,
    SkillID::PELT,
    SkillID::RENDING_VINES,
    SkillID::PATH_OF_SCARS,
    SkillID::PERFECT_STORM,
    SkillID::LUNAR_IMPACT,
    SkillID::LUNAR_IMPACT_1,
    SkillID::SEED_OF_LIFE,
    SkillID::SEED_OF_LIFE_1,
    SkillID::NATURAL_CONVERGENCE,
    SkillID::NATURAL_CONVERGENCE_1,
    SkillID::SHARPEN_SPINES,
    SkillID::VULTURE_STANCE,
    SkillID::QUARRY_S_PERIL,
    SkillID::PERFECT_STORM,
    SkillID::ENVELOPING_HAZE,
    SkillID::VENOMOUS_OUTBURST,
    SkillID::SIC_EM,
    SkillID::BUMBLE,
    SkillID::RAIN_OF_SPIKES,
    SkillID::VIPER_S_NEST,
    SkillID::VIPER_S_NEST_1,
    // THIEF
    SkillID::PREPARE_THOUSAND_NEEDLES,
    SkillID::SPIDER_VENOM,
    SkillID::ASSASSIN_S_SIGNET,
    SkillID::SIGNET_OF_MALICE,
    SkillID::SKRITT_SCUFFLE,
    SkillID::HARROWING_STORM,
    SkillID::SHADOW_MELD,
    SkillID::THIEVES_GUILD,
    SkillID::CALTROPS,
    SkillID::FORGED_SURFER_DASH,
    SkillID::FORGED_SURFER_DASH_1,
    SkillID::SUMMON_KRYPTIS_TURRET,
    SkillID::SUMMON_KRYPTIS_TURRET_1,
    // ELEMENTALIST
    SkillID::EARTHQUAKE,
    SkillID::FIRE_SHIELD,
    SkillID::SIGNET_OF_RESTORATION,
    SkillID::SAND_SQUALL,
    SkillID::DUST_STORM,
    SkillID::DRAGON_S_TOOTH,
    SkillID::FLAME_BARRAGE,
    SkillID::IGNITE,
    SkillID::IGNITE_1,
    SkillID::RELENTLESS_FIRE,
    SkillID::CONFLAGRATION,
    SkillID::AERIAL_AGILITY,
    SkillID::AERIAL_AGILITY_1,
    SkillID::AERIAL_AGILITY_2,
    SkillID::OTTER_S_COMPASSION,
    SkillID::OTTER_S_COMPASSION_1,
    SkillID::BUOYANT_DELUGE,
    // MESMER
    SkillID::SIGNET_OF_DOMINATION,
    SkillID::SIGNET_OF_MIDNIGHT,
    SkillID::SIGNET_OF_INSPIRATION,
    SkillID::SIGNET_OF_ILLUSIONS,
    SkillID::SIGNET_OF_THE_ETHER,
    SkillID::JAUNT,
    SkillID::PHANTASMAL_BERSERKER,
    SkillID::PHANTASMAL_LANCER,
    SkillID::PHANTASMAL_DISENCHANTER,
    SkillID::PHANTASMAL_WARDEN,
    SkillID::CONTINUUM_SHIFT,
    SkillID::CONTINUUM_SPLIT,
    SkillID::BLADESONG_SORROW,
    SkillID::BLADETURN_REQUIEM,
    SkillID::CRYSTAL_SANDS,
    SkillID::BLADECALL,
    SkillID::BLADECALL_1,
    SkillID::TALE_OF_THE_SOULKEEPER,
    SkillID::HARMONIOUS_HARP,
    SkillID::HARMONIOUS_HARP_1,
    SkillID::TIME_SINK,
    SkillID::TALE_OF_THE_AUGUST_QUEEN,
    // NECROMANCER
    SkillID::MANIFEST_SAND_SHADE,
    SkillID::DEATH_S_CHARGE,
    SkillID::YOU_ARE_ALL_WEAKLINGS,
    SkillID::SOUL_SHARDS,
    SkillID::PRESERVATION,
    SkillID::INNERVATE_PRESERVATION,
    SkillID::INNERVATE_PRESERVATION_1,
    SkillID::GRASPING_DARKNESS,
    SkillID::DISTRESS,
    SkillID::PERFORATE,
    SkillID::ANGUISH,
    SkillID::INNERVATE_ANGUISH,
    SkillID::INNERVATE_ANGUISH_1,
    SkillID::WANDERLUST,
    SkillID::WANDERLUST_1,
    SkillID::INNERVATE_WANDERLUST,
    SkillID::INNERVATE_WANDERLUST_1,
    SkillID::PLAGUE_SIGNET,
    SkillID::PLAGUELANDS,
    SkillID::ELIXIR_OF_PROMISE,
    SkillID::ELIXIR_OF_PROMISE_1,
    SkillID::GARISH_PILLAR,
    SkillID::HAUNT,
    SkillID::NEFARIOUS_FAVOR,
    SkillID::TERRIFY,
    SkillID::INFUSING_TERROR,
    SkillID::SUMMON_SPIRITS,
    SkillID::SUMMON_SPIRITS_1,
    SkillID::SPLINTER_WEAPON,
    SkillID::NIGHTMARE_WEAPON,
    // REVENANT
    SkillID::ABYSSAL_BLITZ,
    SkillID::FACET_OF_CHAOS,
    SkillID::FACET_OF_ELEMENTS,
    SkillID::FACET_OF_DARKNESS,
    SkillID::FACET_OF_LIGHT,
    SkillID::FACET_OF_NATURE,
    SkillID::FACET_OF_STRENGTH,
    SkillID::RAZORCLAW_S_RAGE,
    SkillID::ENERGY_MELD,
    SkillID::ENERGY_MELD_1,
    SkillID::COSMIC_WISDOM,
    SkillID::RELINQUISH_POWER,
    SkillID::SPEAR_OF_ARCHEMORUS,
    SkillID::ORDERS_FROM_ABOVE,
    SkillID::EMBRACE_THE_DARKNESS,
    SkillID::EMBRACE_THE_DARKNESS_1,
    SkillID::RESIST_THE_DARKNESS,
    SkillID::SANDSTORM_SHROUD,
};

const std::map<std::string_view, std::set<SkillID>> class_map_special_match_to_gray_out = {
    {
        "power_tempest_sword_dagger_v4",
        {
            SkillID::RIDE_THE_LIGHTNING,
        },
    },
    {
        "condition_weaver_scepter_warhorn_v4",
        {
            SkillID::SIGNET_OF_EARTH,
            SkillID::SIGNET_OF_FIRE,
            SkillID::PRIMORDIAL_STANCE,
        },
    },
    {
        "power_weaver_sword_dagger_v4",
        {
            SkillID::RIDE_THE_LIGHTNING,
            SkillID::PRIMORDIAL_STANCE,
            SkillID::ARCANE_BLAST,
        },
    },
    {
        "power_luminary_spear_greatsword_v4",
        {
            SkillID::PIERCING_STANCE,
            SkillID::EFFULGENT_STANCE,
            SkillID::DARING_ADVANCE,
            SkillID::DARING_ADVANCE_1,
            SkillID::DAYBREAKING_SLASH,
        },
    },
    {
        "power_scrapper_hammer_v4",
        {
            SkillID::POSITIVE_STRIKE,
            SkillID::NEGATIVE_BASH,
            SkillID::EQUALIZING_BLOW,
        },
    },
    {
        "power_virtuoso_spear_greatsword_v4",
        {
            SkillID::SPATIAL_SURGE
        },
    },
    {
        "power_virtuoso_greatsword_dagger_sword_v4",
        {
            SkillID::SPATIAL_SURGE
        },
    },
};

const std::map<std::string_view, std::set<SkillID>> class_map_easy_mode_drop_match = {
    {
        "condition_weaver_scepter_warhorn_v4",
        {
            SkillID::FLAMESTRIKE,
        },
    },
    {
        "condition_mechanist_one_kit_spear_v4",
        {
            SkillID::ROLLING_SMASH,
            SkillID::DISCHARGE_ARRAY,
            SkillID::SKY_CIRCUS,
            SkillID::JADE_MORTAR,
        },
    },
};

const std::set<std::string_view> special_substr_to_remove_duplicates_names = {
    "Offensive Protocol: Demolish",
    "Cyclone Trigger",
    "Steel Divide",
};

const std::set<SkillID> special_substr_to_remove_duplicates = {
    // REVENANT
    SkillID::LEGENDARY_ALLIANCE,
    SkillID::LEGENDARY_ALLIANCE_STANCE,
    SkillID::LEGENDARY_ASSASSIN_STANCE,
    SkillID::LEGENDARY_CENTAUR_STANCE,
    SkillID::LEGENDARY_DEMON_STANCE,
    SkillID::LEGENDARY_DRAGON_STANCE,
    SkillID::LEGENDARY_DWARF_STANCE,
    SkillID::LEGENDARY_ENTITY_STANCE,
    SkillID::LEGENDARY_RENEGADE_STANCE,
    SkillID::LEGENDARY_RENEGADE_STANCE_1,
    SkillID::ABYSSAL_BLITZ,
    // GUARDIAN
    SkillID::RUSHING_JUSTICE,
    // ENGINEER
    SkillID::DEVASTATOR,
    SkillID::OVERCHARGED_SHOT,
    // MESMER
    SkillID::JAUNT,
    SkillID::CRY_OF_FRUSTRATION,
    // RANGER
    SkillID::WOLF_S_ONSLAUGHT,
    // WARRIOR
    SkillID::SIGNET_OF_FURY,
    // NA
    SkillID::DEATHSTRIKE,
    SkillID::UNLEASHED_OVERBEARING_SMASH,
    SkillID::ARCANE_BLAST,
    SkillID::TWILIGHT_COMBO,
    SkillID::DEATH_BLOSSOM,
    SkillID::BEGUILING_HAZE,
};

const std::set<std::string_view> easy_mode_drop_match_name = {
    // ENGINEER
    "Sky Circus",
    "Spark Revolver",
    "Core Reactor Shot",
    "Jade Mortar",
    "Rocket Punch",
    "Rolling Smash",
    "Barrier Burst",
    "Crisis Zone",
    "Discharge Array",
};

const std::set<SkillID> easy_mode_drop_match = {
    // REVENANT
    SkillID::ABYSSAL_STRIKE,
    SkillID::ABYSSAL_FIRE,
    // MESMER
    SkillID::POWER_SPIKE,
};

const SkillRules skill_rules = SkillRules{
    skills_substr_weapon_swap_like,
    skills_match_weapon_swap_like,
    skills_substr_to_drop,
    skills_match_to_drop,
    special_substr_to_gray_out,
    special_match_to_gray_out_names,
    special_match_to_gray_out,
    special_substr_to_remove_duplicates_names,
    special_substr_to_remove_duplicates,
    easy_mode_drop_match_name,
    easy_mode_drop_match,
    class_map_special_match_to_gray_out,
    class_map_easy_mode_drop_match,
};

const std::set<SkillID> skills_to_not_track = {
    SkillID::RIFLE_BURST_GRENADE,
};

const std::map<SkillID, SkillID> special_mapping_skills = {
    {SkillID::DEVASTATOR, SkillID::FOCUSED_DEVASTATION},
    {SkillID::FOCUSED_DEVASTATION, SkillID::DEVASTATOR},
    {SkillID::LIGHTNING_ROD, SkillID::ELECTRIC_ARTILLERY},
    {SkillID::ELECTRIC_ARTILLERY, SkillID::LIGHTNING_ROD},
};

const std::set<SkillID> berserker_f1_skills = {
    SkillID::EVISCERATE,
    SkillID::EARTHSHAKER,
    SkillID::ARC_DIVIDER,
    SkillID::FLAMING_FLURRY,
    SkillID::DECAPITATE,
    SkillID::RUPTURING_SMASH,
    SkillID::WILD_THROW,
    SkillID::SCORCHED_EARTH,
};

const std::set<SkillID> mesmer_weapon_4_skills = {
    SkillID::PHANTASMAL_DUELIST,
    SkillID::TEMPORAL_CURTAIN,
    SkillID::PHANTASMAL_BERSERKER,
    SkillID::ILLUSIONARY_RIPOSTE,
    SkillID::THE_PRESTIGE,
    SkillID::SLIPSTREAM,
    SkillID::PHANTASMAL_WHALER,
    SkillID::CHAOS_ARMOR,
    SkillID::COUNTER_BLADE,
    SkillID::INTO_THE_VOID,
    SkillID::DEJA_VU,
    SkillID::ECHO_OF_MEMORY,
    SkillID::PHANTASMAL_SHARPSHOOTER,
    SkillID::PHANTASMAL_LANCER,
};

const std::set<SkillID> reset_like_skill = {
    // WARRRIOR
    SkillID::CRUSHING_BLOW,
    // ENGINEER
    SkillID::REFRACTION_CUTTER,
    SkillID::REFRACTION_CUTTER_1,
    // MESMER
    SkillID::MIND_THE_GAP,
    // ELEMENTALIST
    SkillID::OVERLOAD_AIR,
    SkillID::OVERLOAD_FIRE,
    SkillID::OVERLOAD_WATER,
    SkillID::OVERLOAD_EARTH,
};

const std::map<SkillID, float> skill_cast_time_map = {
    {SkillID::ESSENCE_BLAST, 0.75f},    // rit shroud aa
    {SkillID::ISOLATE, 0.25f},          //
    {SkillID::PERFORATE, 1.0f},         //
    {SkillID::DISTRESS, 0.5f},          //
    {SkillID::LIFE_REND, 0.5f},         // reaper shroud aa
    {SkillID::LIFE_SLASH, 0.5f},        // reaper shroud aa
    {SkillID::LIFE_REAP, 0.5f},         // reaper shroud aa
    {SkillID::TAINTED_BOLTS, 0.5f},     // harbinger shroud aa
    {SkillID::HAIL_OF_JUSTICE, 1.25f},  // guard pistol 4
    {SkillID::CLEANSING_FLAME, 3.25f},  // guard torch 5
    {SkillID::RAPID_FIRE, 2.25f},       // ranger lb 2
    {SkillID::BARRAGE, 2.25f},          // ranger lb 5
    {SkillID::WEAKENING_WHIRL, 0.75f},  // thief staff 3
    {SkillID::DOUBLE_TAP, 0.75f},       // thief rifle 3
    {SkillID::THREE_ROUND_BURST, 1.0f}, // thief rifle 3
    {SkillID::WILD_THROW, 3.25f},       // warrior spear f1
    {SkillID::WHIRLING_DEFENSE, 3.25f}, // ranger axe 5
    {SkillID::WHIRLING_WRATH, 0.75f},   // gaurd
    {SkillID::RIFLE_BURST, 0.5f},       // engi rifle 1
    {SkillID::SPATIAL_SURGE, 1.0f},     // mesmer gs 1
    {SkillID::FLYING_CUTTER, 0.5f},     // mesmer sw 1
    {SkillID::FLAMING_FLURRY, 3.25f},   // warrior sw f1
    {SkillID::SCORCHED_EARTH, 3.25f},   // warrior lb f1
    {SkillID::OVERLOAD_AIR, 4.0f},      // ele tempest f1 air
    {SkillID::OVERLOAD_FIRE, 4.0f},     // ele tempest f1 fire
    {SkillID::FLAMESTRIKE, 0.75f},      // ele scepter 1 fire
    {SkillID::SCORCHING_SHOT, 0.5f},    // ele earth pistol 1
    {SkillID::PIERCING_PEBBLE, 0.5f},   // ele fire pistol 1
    {SkillID::RIFT_SLASH, 0.5f},        // rev sword 1
    {SkillID::PUNCTURING_JAB, 0.5f},    // engi spear 1
    {SkillID::RENDING_STRIKE, 0.5f},    // engi spear 1
    {SkillID::AMPLIFYING_SLICE, 0.5f},  // engi spear 1
    {SkillID::CONDUIT_SURGE, 0.5f},     // engi spear 2
    {SkillID::LIGHTNING_ROD, 0.5f},     // engi spear 3
    {SkillID::ORB_OF_WRATH, 0.5f},      // gaurd scepter 1
    {SkillID::POLARIC_SLASH, 0.5f},      //
    {SkillID::FIRE_SWIPE, 0.5f},      //
};

const std::map<SkillID, float> grey_skill_cast_time_map = {
    // GUARDIAN
    {SkillID::RUSHING_JUSTICE, 0.5f},
    {SkillID::SYMBOL_OF_PUNISHMENT, 0.25f},
    {SkillID::FLOWING_RESOLVE, 0.5f},
    {SkillID::JURISDICTION, 0.75f},
    // MESMER
    {SkillID::PHANTASMAL_BERSERKER, 0.75f},
    {SkillID::SIGNET_OF_THE_ETHER, 1.0f},
    {SkillID::PHANTASMAL_DISENCHANTER, 1.0f},
    // ENGINEER
    {SkillID::FLAME_JET, 2.0f},    // NOTE: lower than real
    {SkillID::FLAME_BLAST, 0.50f}, //
    {SkillID::NAPALM, 2.0f},       // NOTE: lower than real
    {SkillID::SUPERCONDUCTING_SIGNET, 0.75f},
    {SkillID::DEVASTATOR, 1.0f}, // XXX: Check if it works
    // ELEMENTALIST
    {SkillID::DRAGON_S_TOOTH, 0.75f},
    {SkillID::AERIAL_AGILITY, 0.5f},
    {SkillID::AERIAL_AGILITY_1, 0.5f},
    {SkillID::AERIAL_AGILITY_2, 0.5f},
};

const std::map<int, int> unk_skill_id_fix = {
    {1, 73055},     // daybreaking slash
    {72923, 73055}, // daybreaking slash
    {7, 62668},     // rushing justice
};

// TODO: Check if can be moved to skill_data_unk_map
const std::map<int, int> fix_skill_img_ids = {
    {3332122, 3379164}, // Isolate
    {3332077, 3379162}, // Perforate
    {3332117, 3379165}, // Distress
    {3332087, 3379166}, // Extirpate
    {1, 3379124},       // daybreaking slash
    {7, 2479367},       // rushing justice
};

SkillData GetDataByID(const SkillID skill_id, const SkillDataMap &skill_data_map)
{
    auto it = skill_data_map.find(skill_id);

    if (it != skill_data_map.end())
        return it->second;

    return {};
}

bool IsProfessionResetLikeSKill(const SkillID skill_id)
{
    auto current_profession = get_current_profession_name();
    auto profession_lower = to_lowercase(current_profession);

    auto is_mesmer_weapon_4 = false;
    auto is_berserker_f1 = false;
    if (profession_lower == "mesmer")
        is_mesmer_weapon_4 =
            SkillRuleData::mesmer_weapon_4_skills.find(skill_id) != SkillRuleData::mesmer_weapon_4_skills.end();
    else if (profession_lower == "warrior")
        is_berserker_f1 = SkillRuleData::berserker_f1_skills.find(skill_id) != SkillRuleData::berserker_f1_skills.end();

    return is_mesmer_weapon_4 || is_berserker_f1;
}

} // namespace SkillRuleData
