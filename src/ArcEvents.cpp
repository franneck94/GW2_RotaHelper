///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - Licensed under the MIT License.
///
/// Name         :  ArcEvents.cpp
/// Description  :  Contains the callbacks for ArcDPS.
///----------------------------------------------------------------------------------------------------
#include <map>
#include <string>

#include "ArcEvents.h"

#include "Shared.h"
#include "Types.h"

#define USE_ANY_SKILL_LOGIC
#define USE_SKILL_ID_MATCH_LOGIC

namespace
{
	bool IsValidCombatEvent(const EvCombatData &evCbtData)
	{
		return evCbtData.src != nullptr && evCbtData.skillname != nullptr && evCbtData.ev != nullptr;
	}

	bool IsSkillFromBuild_IdBased(const EvCombatDataPersistent &evCbtData)
	{
		const auto &skill_info_map = rotation_run.skill_info_map;
		return skill_info_map.find(evCbtData.SkillID) != skill_info_map.end();
	}

	bool IsSkillFromBuild_NameBased(const EvCombatDataPersistent &evCbtData)
	{
		const auto &skill_info_map = rotation_run.skill_info_map;
		for (const auto &kv : skill_info_map)
		{
			if (kv.second.name == evCbtData.SkillName)
				return true;
		}
		return false;
	}

	bool IsAnySkillFromBuild(const EvCombatDataPersistent &evCbtData)
	{
#ifdef USE_SKILL_ID_MATCH_LOGIC
		return IsSkillFromBuild_IdBased(evCbtData);
#else
		return IsSkillFromBuild_NameBased(evCbtData);
#endif
	}
};

namespace ArcEv
{
	void OnCombatLocal(
		ArcDPS::CombatEvent *ev,
		ArcDPS::AgentShort *src,
		ArcDPS::AgentShort *dst,
		char *skillname,
		uint64_t id,
		uint64_t revision)
	{
		OnCombat("EV_ARCDPS_COMBATEVENT_LOCAL_RAW", ev, src, dst, skillname, id, revision);
	}

	bool OnCombat(
		const char *channel,
		ArcDPS::CombatEvent *ev,
		ArcDPS::AgentShort *src,
		ArcDPS::AgentShort *dst,
		char *skillname,
		uint64_t id, uint64_t revision)
	{
		if (APIDefs == nullptr)
			return false;

		auto evCbtData = EvCombatData{
			ev,
			src,
			dst,
			skillname,
			id,
			revision};

		APIDefs->Events.Raise(channel, &evCbtData);

		if (IsValidCombatEvent(evCbtData))
		{
			auto data = EvCombatDataPersistent{
				.SrcID = evCbtData.src->ID,
				.SrcProfession = evCbtData.src->Profession,
				.SrcSpecialization = evCbtData.src->Specialization,
				.DstID = evCbtData.dst->ID,
				.DstProfession = evCbtData.dst->Profession,
				.DstSpecialization = evCbtData.dst->Specialization,
				.SkillName = std::string(evCbtData.skillname),
				.SkillID = evCbtData.id};

#ifdef USE_ANY_SKILL_LOGIC
			if (IsAnySkillFromBuild(data))
#endif
			{
				prev_combat_buffer_index = combat_buffer_index;
				combat_buffer[combat_buffer_index] = data;
				combat_buffer_index = (combat_buffer_index + 1) % combat_buffer.size();

				render.key_press_cb(true, data);

				return true;
			}
		}

		return false;
	}
}
