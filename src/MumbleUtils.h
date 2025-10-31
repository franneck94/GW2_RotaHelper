#include "Shared.h"

unsigned int GetCurrentMapID();

bool IsValidMap();

bool IsInfight();

std::string get_current_profession_name();

Mumble::Identity ParseMumbleIdentity(const wchar_t *identityString);
