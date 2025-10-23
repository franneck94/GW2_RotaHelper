#include "Shared.h"

#include <codecvt>
#include <conio.h>
#include <locale>

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


Mumble::Identity ParseMumbleIdentity(const wchar_t *identityString)
{
    Mumble::Identity identity = {};

    try
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::string jsonString = converter.to_bytes(identityString);

        auto json = nlohmann::json::parse(jsonString);

        if (json.contains("profession") &&
            json["profession"].is_number_integer())
            identity.Profession =
                static_cast<Mumble::EProfession>(json["profession"].get<int>());

        if (json.contains("spec") && json["spec"].is_number_unsigned())
            identity.Specialization = json["spec"].get<unsigned>();
        else if (json.contains("specialization") &&
                 json["specialization"].is_number_unsigned())
            identity.Specialization = json["specialization"].get<unsigned>();

        if (json.contains("map_id") && json["map_id"].is_number_unsigned())
            identity.MapID = json["map_id"].get<unsigned>();
        else if (json.contains("map") && json["map"].is_number_unsigned())
            identity.MapID = json["map"].get<unsigned>();
    }
    catch (const std::exception &e)
    {
    }

    return identity;
}
