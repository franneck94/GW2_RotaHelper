# GW2 Rotation Helper - Master Your Rotation at the Training Golem

This tool shows in-game the rotation from the Snow Crows benchmark DPS reports.  
It will always show the last 2 skills, the current skill, and the next 7 skills.  
Depending on which skill the user activates, the GUI will proceed onwards in the rotation.

<img src="./media/example.png" alt="Example Screenshot" width="550" height="600" />

## ✨ Installation Guide

1. **Install Nexus**: Install Nexus Addon Manager
2. **Install ArcDPS**: Install arcdps via the Nexus Addon Manager
3. **Install this Addon**: Obtain the DLL either by
   1. Ingame in the nexus addon library
   2. The GitHub release [page](https://github.com/franneck94/GW2_RotaHelper/releases), there you need to place the dll in the GW2/addons dir

## 🎯 User Guide

#### Selecting a Rotation

- The addon ships exported benchmark rotations from the Snow Crows website.
- You can input a filter text; otherwise, all are listed for the currently selected profession.

<div align="left" style="margin-left:50px;">
   <img src="./media/Filter.png" alt="Example Screenshot" />
</div>

<div align="left" style="margin-left:50px;">
   <img src="./media/selection.png" alt="Example Screenshot" />
</div>

- There's a vertical and a horizontal layout depending on your needs.
- Skill names and cast times from the bench log can be turned on.

<div align="left" style="margin-left:50px;">
   <img src="./media/NameAndTime.png" alt="Example Screenshot" />
</div>


#### Important Notes

- The addon is only active in the Aerodrome and in the training area.
- This addon does not do any memory reading by itself it just uses the public ArcDPS API which has its limitations
- The rotation starts at the first time point where damage is dealt to the enemy; hence, pre-cast abilities are excluded.

#### Visual Learning System

- White Border: Current skill to cast
- Purple Border: Final skill in rotation
- Orange Border: Auto-attack skills
- Greyed Icons: Skills that do no damage or are not forwarded by the public ArcDPS API

<div align="left" style="margin-left:50px;">
   <img src="./media/CurrentSkillBorder.png" alt="Current Skill Border" style="display:inline-block; margin-right:10px;" />
   <img src="./media/GreyedOutSkill.png" alt="Greyed Out Skill" style="display:inline-block; margin-right:10px;" />
   <img src="./media/AutoAttackBorder.png" alt="Auto Attack Border" style="display:inline-block;" />
</div>

#### Features and Options

<div align="left" style="margin-left:50px;">
   <img src="./media/options.png" alt="Example Screenshot" />
</div>

Note: Most features and settings have a detailed tooltip if you hover over it in-game.

##### Keybinds

- You can turn on the option to show the default keybinds for the skills.

<div align="left" style="margin-left:50px;">
   <img src="./media/default_keybinds.png" alt="Example Screenshot" />
</div>

- Furthermore, you can even load in your custom keybinds via the XML file.

<div align="left" style="margin-left:50px;">
   <img src="./media/user_keybinds.png" alt="Example Screenshot" />
</div>

##### Copy to Clipboard

You can copy the following data to the clipboard:

- Link to the SC Build Guide
- Link to the Dps.Report

##### Rotation Settings

- Strict Rotation: Every skill has to match exactly, so there is no room for skill swapping like 2->3 instead of 3->2.
   - Not recommended for most builds.
- Easy Skill Mode: On certain classes, some skills are not mandatory (or even not shown). Examples:
   - F-Skills on Mechanist
   - Sword Auto Attack on Condi Renegade/Conduit

#### Tested Builds

In general, you can see in-game via the star/tick/cross icons the state of a build.

- Very good working builds are marked with a ⭐
- Working builds are marked with a <span style="color: #009220ff">green</span> ✔️
- Okayish working builds are marked with a <span style="color: #DAA520">yellow</span> ✔️
- Poorly working builds are marked with an <span style="color: #ffa200ff">orange</span> 🗙
- Very bad working builds are marked with a <span style="color: #e82204ff">red</span> 🗙
- Not yet tested builds are marked with a <span style="color: #43403fff">gray</span>  ❔

NOTE: Some builds have duo logs uploaded (like Condition Daredevil Dagger) - there my web parser script downloaded the wrong log.

Classes that are not properly optimized for:

- Thief
- Guardian

#### Converting your own logs

You can convert your own HTML logs from dps.report or other sources using the provided Python scripts:

1. **Setup Python Environment** (if not already installed):
   - Install Python 3.10+ from [python.org](https://python.org)
   - Install required dependencies: `pip install -r scripts/requirements.txt`

2. **Add manual Log List**
   - Add your dps.report http link to: internal_data/manual_log_list.json

3. **Download HTML Logs**:
   - Run: `python scripts/get_html_logs.py --manual`
   - This will download HTML logs and save them in the `internal_data/html/` directory.

4. **Convert to JSON Format**:
   - Run: `python scripts/html_log_to_json.py`
   - This converts the HTML logs to the JSON format used by the addon.
   - Converted files are saved in the `data/bench/` directory.

5. **Install Your Custom Rotations**:
   - Copy the generated JSON files to your addon's bench folder.

## ⚠️ Known Limitations

Full list of issues: [here](ISSUES.md)

## 💬 Support & Feedback

Encountered an issue or have suggestions for improvement? We'd love to hear from you! Reach out via Discord or submit a detailed GitHub issue to help us enhance your experience.
