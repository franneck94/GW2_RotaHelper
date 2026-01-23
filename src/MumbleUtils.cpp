#include <codecvt>
#include <conio.h>
#include <locale>

#include "mumble/Mumble.h"

#include "Shared.h"
#include "TypesUtils.h"

unsigned int GetCurrentMapID()
{
    if (Globals::MumbleData && Globals::MumbleData->Context.MapID != 0)
        return Globals::MumbleData->Context.MapID;

    return 0;
}

Mumble::EMapType GetCurrentMapType()
{
    return Globals::MumbleData->Context.MapType;
}

bool IsValidMap()
{
    const auto id = GetCurrentMapID();
    if (id == 1154 || id == 1155)
        return true;

    const auto map_type = GetCurrentMapType();
    return map_type == Mumble::EMapType::Public || map_type == Mumble::EMapType::Instance ||
           map_type == Mumble::EMapType::Public_Mini || map_type == Mumble::EMapType::BigBattle;
}

bool IsInfight()
{
    if (Globals::MumbleData)
        return Globals::MumbleData->Context.IsInCombat != 0;

    return false;
}

std::string get_current_profession_name()
{
    if (!Globals::MumbleData)
        return "";

    try
    {
        return profession_to_string(static_cast<ProfessionID>(Globals::Identity.Profession));
    }
    catch (...)
    {
        return "";
    }
}

std::string get_current_spec_name()
{
    if (!Globals::MumbleData)
        return "";

    try
    {
        return elite_spec_to_string(static_cast<EliteSpecID>(Globals::Identity.Specialization));
    }
    catch (...)
    {
        return "";
    }
}

Mumble::Identity ParseMumbleIdentity(const wchar_t *identityString)
{
    static auto identity = Mumble::Identity{};

    try
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::string jsonString = converter.to_bytes(identityString);

        auto json = nlohmann::json::parse(jsonString);

        if (json.contains("profession") && json["profession"].is_number_integer())
        {
            const auto _profession = static_cast<Mumble::EProfession>(json["profession"].get<int>());

            if (_profession != Mumble::EProfession::None)
                identity.Profession = _profession;
        }

        auto _specialization = 0;
        if (json.contains("spec") && json["spec"].is_number_unsigned())
            _specialization = json["spec"].get<unsigned>();
        else if (json.contains("specialization") && json["specialization"].is_number_unsigned())
            _specialization = json["specialization"].get<unsigned>();

        if (_specialization != 0)
            identity.Specialization = _specialization;

        auto _map_id = 0;
        if (json.contains("map_id") && json["map_id"].is_number_unsigned())
            _map_id = json["map_id"].get<unsigned>();
        else if (json.contains("map") && json["map"].is_number_unsigned())
            _map_id = json["map"].get<unsigned>();

        if (_map_id != 0)
            identity.MapID = _map_id;
    }
    catch (const std::exception &e)
    {
    }

    return identity;
}
