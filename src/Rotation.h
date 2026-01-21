#pragma once

#include <chrono>
#include <cstdint>

#include "LogData.h"
#include "Types.h"
#include "TypesUtils.h"

void KeypressSkillDetectionLogic(RotationLogType &RotationRun);

void SkillDetectionLogic(uint32_t &num_skills_wo_match,
                         std::chrono::steady_clock::time_point &time_since_last_match,
                         RotationLogType &rotation_run,
                         const EvCombatDataPersistent &skill_ev);
