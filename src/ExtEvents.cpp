///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - Licensed under the MIT License.
///
/// Name         :  ExtEvents.cpp
/// Description  :  Contains the logic for extended events.
///----------------------------------------------------------------------------------------------------

#include "ExtEvents.h"

#include <vector>
#include <mutex>

#include "Shared.h"

namespace Ext
{
	static std::mutex                 s_Mutex;
	static EvAgentUpdate              s_Self;
	static EvAgentUpdate              s_LastTarget;
	static std::vector<EvAgentUpdate> s_Squad;

	void AgentUpdate(EvCombatData* evCbtData)
	{
		if (evCbtData->ev) { return; }

		EvAgentUpdate evAgentUpdate
		{
				"",
				"",
				evCbtData->src ? evCbtData->src->ID : 0,
				evCbtData->dst ? evCbtData->dst->ID : 0,
				evCbtData->src ? evCbtData->src->Profession : 0,
				evCbtData->src ? evCbtData->src->Specialization : 0,
				evCbtData->dst ? evCbtData->dst->IsSelf : 0,
				evCbtData->dst ? evCbtData->dst->Profession : 0,
				evCbtData->dst ? evCbtData->dst->Specialization : 0,
				evCbtData->src ? evCbtData->src->Team : uint16_t(0),
				evCbtData->dst ? evCbtData->dst->Team : uint16_t(0),
		};
		if (evCbtData->dst && evCbtData->dst->Name && evCbtData->dst->Name[0] != '\0')
		{
			strcpy_s(evAgentUpdate.account, evCbtData->dst->Name);
		}
		if (evCbtData->src && evCbtData->src->Name && evCbtData->src->Name[0] != '\0')
		{
			strcpy_s(evAgentUpdate.character, evCbtData->src->Name);
		}

		if (evAgentUpdate.target)
		{
			std::scoped_lock lck(s_Mutex);
			s_LastTarget = evAgentUpdate;

			APIDefs->Events.Raise(EV_ARCDPS_TARGET_CHANGED, &evAgentUpdate);

			return;
		}

		if (!evAgentUpdate.added)
		{
			std::scoped_lock lck(s_Mutex);
			if (s_Self.id != 0 && s_Self.id == evAgentUpdate.id)
			{
				strcpy_s(evAgentUpdate.account, s_Self.account);
				strcpy_s(evAgentUpdate.character, s_Self.character);
				evAgentUpdate.Self = 1;
				s_Squad.clear();

				APIDefs->Events.Raise(EV_ARCDPS_SELF_LEAVE, &evAgentUpdate);
			}
			else
			{
				s_Squad.erase(std::remove_if(
					s_Squad.begin(), s_Squad.end(),
					[&](EvAgentUpdate const& member)
				{
					if (member.id == evAgentUpdate.id)
					{
						strcpy_s(evAgentUpdate.account, member.account);
						strcpy_s(evAgentUpdate.character, member.character);

						APIDefs->Events.Raise(EV_ARCDPS_SQUAD_LEAVE, &evAgentUpdate);

						return true;
					}
					return false;
				}),
					s_Squad.end()
				);
			}
			return;
		}

		if (evAgentUpdate.account == 0 || evAgentUpdate.character == 0) { return; }

		if (!evAgentUpdate.Self)
		{
			std::scoped_lock lck(s_Mutex);
			s_Squad.push_back(evAgentUpdate);

			APIDefs->Events.Raise(EV_ARCDPS_SQUAD_JOIN, &evAgentUpdate);

			return;
		}

		{
			std::scoped_lock lck(s_Mutex);
			s_Self = evAgentUpdate;

			APIDefs->Events.Raise(EV_ARCDPS_SELF_JOIN, (void*)&s_Self);

			if (AccountName.empty())
			{
				AccountName = s_Self.account;
				APIDefs->Events.Raise(EV_ACCOUNT_NAME, (void*)AccountName.c_str());
			}
		}
	}

	void OnAccountNameRequest(void* eventArgs)
	{
		if (AccountName.empty()) { return; }

		std::thread([]()
		{
			APIDefs->Events.Raise(EV_ACCOUNT_NAME, (void*)AccountName.c_str());
		}).detach();
	}

	void OnSelfRequest(void* eventArgs)
	{
		std::scoped_lock lck(s_Mutex);

		if (s_Self.id == 0) { return; }

		std::thread([]()
		{
			APIDefs->Events.Raise(EV_ARCDPS_SELF_JOIN, (void*)&s_Self);
		}).detach();
	}

	void OnSquadRequest(void* eventArgs)
	{
		std::scoped_lock lck(s_Mutex);

		if (s_Squad.size() == 0) { return; }

		std::thread([]()
		{
			for (EvAgentUpdate& member : s_Squad)
			{
				APIDefs->Events.Raise(EV_ARCDPS_SQUAD_JOIN, (void*)&member);
			}
		}).detach();
	}

	void OnTargetRequest(void* eventArgs)
	{
		std::scoped_lock lck(s_Mutex);
		if (s_LastTarget.id == NULL) return;
		std::thread([]()
		{
			APIDefs->Events.Raise(EV_ARCDPS_TARGET_CHANGED, (void*)&s_LastTarget);
		}).detach();
	}
}
