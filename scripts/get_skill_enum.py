#!/usr/bin/env python3
"""
Script to generate C++ enum class for GW2 skills from the raw skills JSON data.
Creates an enum where SKILL_NAME = ID is stored.
"""

import json
import re
from pathlib import Path


def sanitize_name(name: str) -> str:
    """
    Convert skill name to a valid C++ enum identifier.
    """
    # Remove special characters and replace with underscores
    sanitized = re.sub(r"[^a-zA-Z0-9_]", "_", name)

    # Remove multiple consecutive underscores
    sanitized = re.sub(r"_+", "_", sanitized)

    # Remove leading/trailing underscores
    sanitized = sanitized.strip("_")

    # Ensure it doesn't start with a number
    if sanitized and sanitized[0].isdigit():
        sanitized = f"SKILL_{sanitized}"

    # Convert to uppercase for enum convention
    sanitized = sanitized.upper()

    # Handle empty names
    if not sanitized:
        sanitized = "UNKNOWN_SKILL"

    return sanitized


def load_skills_data(json_path: Path) -> dict:
    """Load the GW2 skills raw JSON data."""
    try:
        with open(json_path, encoding="utf-8") as f:
            return json.load(f)
    except Exception as e:
        print(f"Error loading JSON: {e}")
        raise


def generate_skill_enum(skills_data: dict, output_path: Path):
    """Generate C++ enum class for skills."""

    # Sort skills by ID for consistent output
    try:
        sorted_skills = sorted(skills_data.items(), key=lambda x: int(x[0]))
    except ValueError as e:
        print(f"Error sorting skills by ID: {e}")
        # Fallback to string sorting
        sorted_skills = sorted(skills_data.items())

    enum_content = []
    enum_content.append("// Auto-generated skill enum from GW2 API data")
    enum_content.append("// DO NOT EDIT MANUALLY")
    enum_content.append("")
    enum_content.append("#pragma once")
    enum_content.append("")
    enum_content.append("#include <cstdint>")
    enum_content.append("")
    enum_content.append("enum class SkillID : uint32_t")
    enum_content.append("{")

    # Track duplicate names to avoid enum conflicts
    used_names = set()
    valid_skills = 0

    for skill_id, skill_data in sorted_skills:
        try:
            skill_name = skill_data.get("name", f"Unknown_{skill_id}")
            if not skill_name:
                skill_name = f"Unknown_{skill_id}"

            sanitized_name = sanitize_name(skill_name)

            # Handle duplicate names
            original_name = sanitized_name
            counter = 1
            while sanitized_name in used_names:
                sanitized_name = f"{original_name}_{counter}"
                counter += 1

            used_names.add(sanitized_name)

            # No comment after the enum value
            enum_content.append(f"    {sanitized_name} = {skill_id},")
            valid_skills += 1

        except Exception as e:
            print(f"Error processing skill {skill_id}: {e}")
            continue

    enum_content.append("};")
    enum_content.append("")

    # Write to file
    try:
        with open(output_path, "w", encoding="utf-8") as f:
            f.write("\n".join(enum_content))

        print(f"Generated skill enum with {valid_skills} skills")
        print(f"Output written to: {output_path}")

    except Exception as e:
        print(f"Error writing output file: {e}")
        raise


def main():
    """Main function to generate skill enum."""
    # Get project root directory
    script_dir = Path(__file__).parent
    project_root = script_dir.parent

    # Input and output paths
    json_path = project_root / "data" / "skills" / "gw2_skills_en.json"
    output_path = project_root / "src" / "SkillIDs.h"

    # Check if input file exists
    if not json_path.exists():
        print(f"Error: Skills JSON file not found at {json_path}")
        return

    print(f"Loading skills data from: {json_path}")

    try:
        # Load skills data
        skills_data = load_skills_data(json_path)
        print(f"Loaded {len(skills_data)} skills from JSON")

        # Generate enum
        generate_skill_enum(skills_data, output_path)

        print("Skill enum generation completed successfully!")

    except Exception as e:
        print(f"Error: {e}")
        import traceback

        traceback.print_exc()


if __name__ == "__main__":
    main()
