import json
import re

with open('GW2EI/Content/SkillList.json', 'r', encoding='utf-8') as f:
    content = f.read()

# Find all skills with Mesmer profession and Weapon_4 slot
skills = json.loads(content)
mesmer_weapon4_skills = []

for skill in skills:
    if (skill.get('professions') == ['Mesmer'] and
        skill.get('slot') == 'Weapon_4'):
        mesmer_weapon4_skills.append({
            'name': skill.get('name'),
            'id': skill.get('id'),
            'weapon_type': skill.get('weapon_type', 'unknown')
        })

print('Mesmer Weapon 4 Skills:')
for skill in mesmer_weapon4_skills:
    print(f"  {skill['name']} (ID: {skill['id']}, Weapon: {skill['weapon_type']})")
