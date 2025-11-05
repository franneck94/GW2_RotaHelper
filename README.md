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

#### Converting your own logs

You can convert your own HTML logs from dps.report or other sources using the provided Python scripts:

1. **Setup Python Environment** (if not already installed):
   - Install Python 3.10+ from [python.org](https://python.org)
   - Install required dependencies: `pip install -r scripts/requirements.txt`

2. **Add manual Log List**
   - Add you dps.report http link to: internal_data/manual_log_list.json

3. **Download HTML Logs**:
   - Run: `python scripts/get_html_logs.py --manual`
   - This will download HTML logs and save them in the `internal_data/html/` directory

4. **Convert to JSON Format**:
   - Run: `python scripts/html_log_to_json.py`
   - This converts the HTML logs to the JSON format used by the addon
   - Converted files are saved in the `data/bench/` directory

5. **Install Your Custom Rotations**:
   - Copy the generated JSON files to your addon's bench folder

## ⚠️ Known Limitations

Full list of Issues: [here](ISSUES.md)

## 💬 Support & Feedback

Encountered an issue or have suggestions for improvement? We'd love to hear from you! Reach out via Discord or submit a detailed GitHub issue to help us enhance your experience.
