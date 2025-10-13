///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - Licensed under the MIT License.
///
/// Name         :  ArcEvents.cpp
/// Description  :  Contains the callbacks for ArcDPS.
///----------------------------------------------------------------------------------------------------

#include "ArcEvents.h"

#include "Shared.h"

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
		static auto last_time_skill_cast = std::map<uint32_t, uint64_t>{};

		if (APIDefs == nullptr)
			return false;

		EvCombatData evCbtData{
			ev,
			src,
			dst,
			skillname,
			id,
			revision};

		APIDefs->Events.Raise(channel, &evCbtData);

		if (evCbtData.src->Name != nullptr && evCbtData.skillname != nullptr && evCbtData.ev != nullptr)
		{
			if (evCbtData.src && evCbtData.id != 0 && evCbtData.src->IsSelf)
			{
				const auto skill_id = static_cast<uint32_t>(evCbtData.ev->SkillID);
				const auto current_time = evCbtData.ev->Time;

				auto it = last_time_skill_cast.find(skill_id);
				if (it != last_time_skill_cast.end())
				{
					const auto time_diff = current_time - it->second;
					if (time_diff <= 250)
					{
						return false;
					}
				}

				last_time_skill_cast[skill_id] = current_time;

				auto data = EvCombatDataPersistent{
					.SrcName = evCbtData.src->Name,
					.SrcID = evCbtData.src->ID,
					.SrcProfession = evCbtData.src->Profession,
					.SrcSpecialization = evCbtData.src->Specialization,
					.DstID = evCbtData.dst->ID,
					.DstProfession = evCbtData.dst->Profession,
					.DstSpecialization = evCbtData.dst->Specialization,
					.SkillName = std::string(evCbtData.skillname),
					.SkillID = evCbtData.ev->SkillID};

				prev_combat_buffer_index = combat_buffer_index;
				combat_buffer[combat_buffer_index] = data;
				combat_buffer_index = (combat_buffer_index + 1) % combat_buffer.size();

				render.key_press_cb(true, data);

				std::cout << evCbtData.skillname << " (FIRST CAST)\n";

				return true;
			}
		}

		return false;
	}
}
