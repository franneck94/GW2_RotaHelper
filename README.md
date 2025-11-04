# GW2 Rotation Helper - Master Your Rotation at the Training Golem

This tool shows ingame the rotation from the snow crows benchmark dps reports.  
It will always show the last 2 skills, the current skill and the next 7 skills.  
Depending on what skill the user activates the GUI will go onwards in the rotation.

## ✨ Installation Guide

1. **Download Components**: Obtain both the DLL and companion ZIP file from our [official releases page](https://github.com/franneck94/GW2_RotaHelper/releases)
2. **Install Addon**: Place the DLL file into your Nexus addons directory
3. **Deploy Resources**: Extract the GW2RotaHelper ZIP archive and copy its contents to your Nexus addons folder

## 🎯 User Guide

#### Selecting a Rotation

- The addon ships exported benchmark rotations from the snow crows website
- You can input a filter text, otherwise all are listed for the currently selected profession
- There's a vertical and a horizontal layout depending on your needs
- Skill names and cast times (from the bench) can be turned on

#### Important Notes

- The addon is only active in aerodrome and in the training area
- The rotation starts at the first time point where damage is dealt to the enemy, hence pre-cast abilities are excluded

#### Visual Learning System

- White Border: Current skill to cast
- Purple Border: Final skill in rotation
- Orange Border: Auto-attack skills
- Greyed Icons: Special skills (attunements, stances - in general non-damaging skills)

![Example Screenshot](./media/example.png)

#### Tested Builds

Builds Working properly:

- Power Scrapper
- Power Soulbeast Hammer
- Power Spellbreaker Hammer
- Power Berserker Hammer
- Power Mechanist Rifle
- Power Mechanist Sword
- Power Galeshot

Builds that are working ok-ish:

- Power Weaver
- Condition Weaver Scepter
- Power Harbinger
- Condition Harbinger
- Condition 1kit Mechanist

Builds are not working proper:

- Condition Willbender
- Power Bladesworn

NOTE: Some builds have duo logs uplaoded (like Condition Daredevil Dagger) - there my webparser script downloaded the wrong log.

## ⚠️ Known Limitations

Full list of Issues: [here](ISSUES.md)

## 💬 Support & Feedback

Encountered an issue or have suggestions for improvement? We'd love to hear from you! Reach out via Discord or submit a detailed GitHub issue to help us enhance your experience.
