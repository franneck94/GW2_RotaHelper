# Changelog

All notable changes to GW2 RotaHelper will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## 0.4.0 - 2025-10-21

### Features

- Skill info on icon hover in horizontal mode
- Only draw in training area and aerodome
- Auto reload if ooc

### Improvements

- Don't show Cascading Corruption
- Show Rushing Justice only once per cast
- Added debug info for the mumble context
- Options window is now collapsible

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
