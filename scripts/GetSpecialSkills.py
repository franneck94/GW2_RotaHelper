import json
import re

with open('GW2EI/Content/SkillList.json', 'r', encoding='utf-8') as f:
    content = f.read()

# Find all skills with Mesmer profession and Weapon_4 slot
skills = json.loads(content)
mesmer_weapon4_skills = []
berserker_f1_skills = []
mesmer_names_seen = set()
berserker_names_seen = set()

for skill in skills:
    # Mesmer Weapon 4 skills
    if (skill.get('professions') == ['Mesmer'] and
        skill.get('slot') == 'Weapon_4'):
        skill_name = skill.get('name')
        if skill_name not in mesmer_names_seen:
            mesmer_names_seen.add(skill_name)
            mesmer_weapon4_skills.append({
                'name': skill_name,
                'id': skill.get('id'),
                'weapon_type': skill.get('weapon_type', 'unknown')
            })

    # Berserker F1 skills (Primal Bursts)
    if (skill.get('professions') == ['Warrior'] and
        skill.get('slot') == 'Profession_1'):
        skill_name = skill.get('name')
        if skill_name not in berserker_names_seen:
            berserker_names_seen.add(skill_name)
            berserker_f1_skills.append({
                'name': skill_name,
                'id': skill.get('id'),
                'weapon_type': skill.get('weapon_type', 'unknown')
            })

print('Mesmer Weapon 4 Skills:')
for skill in mesmer_weapon4_skills:
    print(f"  {skill['name']} (ID: {skill['id']}, Weapon: {skill['weapon_type']})")

print("\n\n")

print('\nBerserker F1 Skills (Primal Bursts):')
for skill in berserker_f1_skills:
    print(f"  {skill['name']} (ID: {skill['id']}, Weapon: {skill['weapon_type']})")
