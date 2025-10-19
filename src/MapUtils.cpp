unsigned int GetCurrentMapID()
{
    if (Globals::MumbleData && Globals::MumbleData->Context.MapID != 0)
    {
        return Globals::MumbleData->Context.MapID;
    }

    return 0;
}

bool IsInTrainingArea()
{
    return GetCurrentMapID() == 1154;
}
