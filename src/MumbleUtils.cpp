#include "Shared.h"

unsigned int GetCurrentMapID()
{
    if (Globals::MumbleData && Globals::MumbleData->Context.MapID != 0)
    {
        return Globals::MumbleData->Context.MapID;
    }

    return 0;
}

bool IsValidMap()
{
    const auto id = GetCurrentMapID();
    return id == 1154 | id == 1155;
}

bool IsInfight()
{
    if (Globals::MumbleData)
        return Globals::MumbleData->Context.IsInCombat != 0;

    return false;
}
