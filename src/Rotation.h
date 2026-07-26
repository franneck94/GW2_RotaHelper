#pragma once

#include <chrono>
#include <cstdint>

#include "LogData.h"
#include "Types.h"
#include "TypesUtils.h"

void KeypressSkillDetectionLogic(RotationLogType &rotation_run, SkillDetectionTimers &timers);

SkillSlot GetExpectedInputSlot(const RotationStep &step, const RotationLogType &rotation_run);

bool IsRotationStepDetectable(const RotationStep &step, const RotationLogType &rotation_run);

bool IsRotationStepGreyedOut(const RotationStep &step, const RotationLogType &rotation_run);

void SkillDetectionLogic(uint32_t &num_skills_wo_match,
                         std::chrono::steady_clock::time_point &time_since_last_match,
                         SkillDetectionTimers &timers,
                         RotationLogType &rotation_run,
                         const EvCombatDataPersistent &skill_ev);
