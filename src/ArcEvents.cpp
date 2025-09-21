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

	void OnCombat(
		const char *channel,
		ArcDPS::CombatEvent *ev,
		ArcDPS::AgentShort *src,
		ArcDPS::AgentShort *dst,
		char *skillname,
		uint64_t id, uint64_t revision)
	{
		if (APIDefs == nullptr)
		{
			return;
		}

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
			if (evCbtData.src && evCbtData.dst && evCbtData.id != 0)
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

				combat_buffer[combat_buffer_index] = data;
				combat_buffer_index = (combat_buffer_index + 1) % combat_buffer.size();
			}
		}
	}
}
