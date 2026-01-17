<!-- markdownlint-disable MD024  -->
# Changelog

All notable changes to GW2 RotaHelper will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Versioning

An increase in the first number indicates an update of the rotation logs/builds.  
An increase of the second number indicates changes in only the code logic.  
An increase in the third number indicates bugfixes only.

## 1.1.0 - 2026-01-16

## Improvements

- Added the Device Field User Keybind read in
  - Mouse keybins are now properly parsed
- Fix for loading keybind file not loaded

## 1.0.0 - 2026-01-15

## Improvements

- Optimization for Power Revenant (Vindi, Conduit)
- Fixed crash on input bind file selection
- Fixed issues with keybind modifiers

### Notes

- Builds fetched from Balance Update

## 0.28.0 - 2026-01-14

## Features

- Rotation display number of icons is now a setting (left and right of current skill)
- Addon can now be displayed in all PvE content

### Improvements

- Fixed dispaly of canceled skills in the logs

## 0.27.0 - 2026-01-09

## Features

- Rotation Icon Overview has now the option to highlight a skill (like in online dps.reports)

### Improvements

- Optimization for Power Luminary
- Very bad working builds are not shown anymore

## 0.26.0 - 2026-01-05

### Improvements

- Optimization for Power Luminary
- Added weapon_type_to_string at the start of each line of the rotation overview text
- Fixed incorrect AA's in the rotation overview text
- Added date on when the builds were fetched from SC website

### Notes

- Builds fetched on 5th Jan 2026
- Updated categories for:
  - Warrior
  - Engineer (except Tempest)
  - Elementalist
  - Necromancer
  - Revenant
  - Guardian
  - Mesmer (except Chronomancer)
  - Ranger

## 0.25.0 - 2026-01-02

### Notes

- Changed from buttons to checkboxes for some options
- Fixed alignment for rotation overview icons

## 0.24.0 - 2025-12-25

### Improvements

- Rotation overview now shows where the player is currently in the rota
- Condensed rotation text shows number of aa's in the chain

## 0.23.0 - 2025-12-24

### Improvements

- Imporvements for skill icons overview window
- Fixed "weapon swaps" for weaver builds

## 0.22.0 - 2025-12-21

### Features

- Show all skill icons in a condensed overview.  
  - A newline indicates a weapon swap.

## Notes

- Fixed incorrect utility skill -> key mapping

## 0.21.0 - 2025-12-12

### Features

- Show all keybinds (skills) in a condensed overview.  
  - A newline indicates a weapon swap.

### Improvements

- Scripts download the on the SC website defined utility skill order

## Notes

- Re-run scripts to get current benches -> Not up to date builds are removed

## 0.20.1 - 2025-12-12

## Notes

- Updated bench data

## 0.20.0 - 2025-12-05

### Improvements

- Fixed addon shortcut (ctrl+q)
- Fixed bug for multi hit skill filtering

## Notes

- Updated to newest commits for mumble/rtapi/nexus

## 0.19.1 - 2025-12-05

### Improvements

- Fixed addon shortcut (ctrl+q)
- Fixed bug for multi hit skill filtering

## Notes

- Updated to newest commits for mumble/rtapi/nexus

## 0.19.1 - 2025-12-03

### Features

- Added Skip Update Option for users where update failed
  - When enabled, the addon will skip checking for benchmark file updates on startup

### Notes

- Added status code logging and extended user guide for ZIP file download/extraction failure

## 0.19.0 - 2025-11-30

### Improvements

- Updated python scripts to parse dps reports with multiple players
  - Fixed logs and started optimization for:
    - Power Dragonhunter
    - Power Willbender
    - Power Luminary
    - Condition Daredevil
    - Condition Antiquary

### Notes

- Manual Log List:
  - Optional: Added field for SnowCrows link to guide the user for equipment/traits

## 0.18.0 - 2025-11-29

### Features

- Added working addon icon image (a park bench)
  - If you have any good idea come back to me

### Improvements

- SC Link/Dps.Report Links are now opened in default browser
- Loads correct dodge image
- Mech F-skills do show correct keybind
- Extended Skill Enum

- Skill Logic Optimization for:
  - Cond Quick Scrapper
  - Cond Spear Holo
  - Power Evoker

## 0.17.0 - 2025-11-26

### Features

- Added option to show number of auto attacks in a chain (of at least 2 aa's)
  - Active when keybind option is active
- Show the Overall DPS from the report on selected build

### Improvements

- Build Selection is now a scrollable
- Improved for Evoker and Catalyst
- Fixed skill duplicates

### Notes

- Dropped Vertical Layout
- Dropped names and times option (is now activated by default, since its only shown when hovered over skill icons)

## Known Issues

- Power Guardian Builds Parsing Error for Logs

## 0.16.2 - 2025-11-25

### Improvements

- Improved for Power Tempest

### Notes

- Added the logic to have gray-skill/easy mode logic for specific builds

## 0.16.1 - 2025-11-24

### Improvements

- Improved for Condition Weaver and Power Scrapper

## 0.16.0 - 2025-11-23

### Improvements

- Improved logic for when to go to next skill in rota for greyed out skills

### Notes

- Updated logs for power/condition conduit

## 0.15.0 - 2025-11-21

### Improvements

- Added the auto download (and update) of the bench data (ZIP) from github, so there is no need to do it manually anymore

### Notes

- Updated logs regarding the bloodstone nerf
- Changed logic for power ritu shroud skills (unfortunately not working proper atm)
- Updated power conduit build and moved it from not working at all to okay-ish working build category
- Added nexus logging messages

## 0.14.0 - 2025-11-20

### Improvements

- Better User Guide in README
- Added info that users also have to download the ZIP file from GitHub
- Cosmetic Changes to the UI
- Added versioning for the bench data files and a message in-game if they are out of sync

## 0.13.0 - 2025-11-17

### Improvements

- Started optimization for
  - Condi Druid
  - Power Untamed
  - Power Chronomancer
  - Condi Berserker
  - Power Tempest builds in general
  - Condition Conduit/Renegade
- Changed weapon swaps to non-damage skill category
- Added weapon type info to skills struct (not used in code yet)

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

- Strict rotation option deactivates weapon swap steps
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

- Added copy to clipboard for Snow Crows link

### Improvements

- Improved non-damage skill logic

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

- Better skill detection in non-strict mode

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
- Auto reload if OOC

### Improvements

- Don't show Cascading Corruption
- Show Rushing Justice only once per cast
- Added debug info for the mumble context
- Options window is now collapsible
- Added adjust UI option where the skills are movable and resizable, otherwise it's fixed
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
