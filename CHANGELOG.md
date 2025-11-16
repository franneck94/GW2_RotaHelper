# Changelog

All notable changes to GW2 RotaHelper will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## 0.13.0

### Improvements

- Started optimization for
  - Condi Druid
  - Power Untamed
  - Power Chronomancer
  - Condi berserker

## 0.12.0 - 2025-11-14

### Features

- Added copy to clipboard for Dps.Report
- Working builds are marked with a green tick
- Okayish working builds are marked with a yellow tick
- Poorly working builds are marked with an orange cross
- Not yet tested builds are marked with a gray question mark

### Improvements

- Optimized for Power Troubadour
- Optimized for Power Paragon
- Optimized for Power Virt GS/Spear
- Added updated builds for condition conduit
- Started optimization for condition conduit and revenant, power vindicator, power herald
  - Always show dodge on vindicator

## 0.11.0 - 2025-11-12

### Improvements

- Strict rotation options deactivates weapon swap steps
- Good working builds are marked with a star in the dropdown list
- Very bad working builds are marked with a red cross in the dropdown list
- Optimized auto attack detection
- Added option for easy skill mode (for more info hover over the checkbox)

### Notes

- Identified more non damaging skills
- Added the following builds to working:
  - Power/Quickness Harbinger
  - Power/Quickness Ritualist
  - Power Spear Reaper
  - Power/Quickness Berserker GS Axe/Axe
  - and many more (full list in the README)

## 0.10.0 - 2025-11-10

### Features

- Added copy to clipboard for snow crows link

### Improvements

- Improved non damage skill logic

## 0.9.0 - 2025-11-07

### Improvements

- Improved same cast detection logic (important for skills that do multiple hits)

### Notes

- Added guide how to add your own rotations
- Added debug info for same skill chain
- Added weakening whirl cast time
- Added SkillID enum

## 0.8.0 - 2025-11-04

### Features

- Added read of XML InputBindings
- Added Strict Mode Flag (see tooltip for more info)

### Improvements

- Better skill detection in non strict mode

## 0.7.0 - 2025-10-31

### Improvements

- Made skill icons resizable
- Show dodge when "weapon swap" option is active

### Notes

- Dropped Ctrl+Q Keybind

## 0.6.0 - 2025-10-31

### Features

- Added unload rota button
- Reset bench selection on relog
- Added the option to show weapon swaps
- Added the option to show the (default) keybind for the skills

### Improvements

- Added some new spec builds
- Added logic for unknown skills to show placeholder
- Dropped canceled skills from the rotation

## 0.5.0 - 2025-10-25

### Improvements

- If no filter text is selected, only show builds of the current profession
- Improved skill activation logic based on recharge time
- Added more non-damage skills to "gray" out
- Do not show additional trait skills
- Addon unload now works properly
- Added logs that are wingman sites

### Development

- Added recharge time to skill struct
- Added Event ID to Debug info

## 0.4.0 - 2025-10-21

### Features

- Skill info on icon hover in horizontal mode
- Only draw in training area and aerodrome
- Auto reload if ooc

### Improvements

- Don't show Cascading Corruption
- Show Rushing Justice only once per cast
- Added debug info for the mumble context
- Options window is now collapsible
- Added adjust UI option where the skills are movable and resizable, otherwise its fixed
- Added better README

## 0.3.0 - 2025-10-19

### Improvements

- Integrated GW2 API data for profession and elite specialization IDs to prevent unnecessary event logging
- Better skill detection, auto attack chains will not skip skills afterwards
- When the Options Window is focused (clicked) you can see where the rotation window will be drawn to adjust the position and size
- Skills that do no damage in the rotation are shown with a greyed out tint
- Skills in the rotation that are auto attacks have an orange border

## 0.2.0 - 2025-10-17

### Features

- Support for Quick and Alacrity benchmark builds
- Automated dropdown menu activation on Enter key press in filter field
- Show Skill Name and Cast Time is now an option
- Horizontal skill layout (names and cast times are omitted)

### Improvements

- Improved file organization structure for different build types
- Split windows for:
  1. Select bench log
  2. The rotation itself

### Bug Fixes

- Tab key navigation behavior in filter text input field
- Filter field focus management and keyboard interactions
- Fixed selection of builds with same name in cond/power

## 0.1.0 - 2025-10-01

### Initial Release

- Initial alpha release of GW2 RotaHelper
- Core rotation tracking functionality
- Basic benchmark file loading and processing
- Real-time skill activation monitoring
- Nexus addon framework support
