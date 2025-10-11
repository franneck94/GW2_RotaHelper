"""
V3 is for locally converted arcdps -> GWEI JSON files
"""

import argparse
import json
import logging
from pathlib import Path
from typing import Dict, Any


def setup_logging():
    logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")


def extract_rotation_data(arcdps_data: Dict[str, Any]) -> Dict[str, Any]:
    extracted_data = {
        "rotation": [],
        "skillMap": {}
    }

    try:
        # Navigate to rotation data
        players = arcdps_data.get("players", [])
        if not players:
            logging.warning("No players found in the data")
            return extracted_data

        # Get first player's rotation data (TODO: make player selectable)
        player = players[0]
        rotation = player.get("rotation", [])

        if not rotation:
            logging.warning("No rotation data found for first player")
        else:
            logging.info(f"Found rotation data with {len(rotation)} skill entries")

            # Process each skill in the rotation
            for skill_entry in rotation:
                skill_id = skill_entry.get("id")
                if skill_id is None:
                    logging.warning("Skill entry missing 'id' field, skipping")
                    continue

                skills = skill_entry.get("skills", [])

                for skill_cast in skills:
                    cast_time = skill_cast.get("castTime", 0)
                    duration = skill_cast.get("duration", 0)
                    quickness = skill_cast.get("quickness", 0)
                    status = 0  # Default status value as requested

                    array_entry = [
                        cast_time / 1000,
                        skill_id,
                        duration,
                        status,
                        quickness
                    ]
                    extracted_data["rotation"].append(array_entry)

            logging.info(f"Extracted {len(extracted_data['rotation'])} rotation entries")

        # Extract skillMap data
        skill_map = arcdps_data.get("skillMap", {})
        if skill_map:
            extracted_data["skillMap"] = skill_map
            logging.info(f"Extracted skillMap with {len(skill_map)} skills")
        else:
            logging.warning("No skillMap found in the data")

    except KeyError as e:
        logging.error(f"Missing required key in JSON structure: {e}")
    except Exception as e:
        logging.error(f"Error processing rotation data: {e}")

    return extracted_data


def save_rotation_data(extracted_data: Dict[str, Any], output_path: Path):
    """
    Save extracted rotation and skillMap data to JSON file.

    Args:
        extracted_data: Dictionary containing rotation and skillMap data
        output_path: Path to save the output file
    """
    try:
        with open(output_path, "w", encoding="utf-8") as f:
            json.dump(extracted_data, f, indent=2, ensure_ascii=False)
        logging.info(f"Saved {len(extracted_data.get('rotation', []))} rotation entries and {len(extracted_data.get('skillMap', {}))} skills to: {output_path}")
    except Exception as e:
        logging.error(f"Error saving rotation data: {e}")
        raise


def main():
    """Main function with CLI interface"""
    parser = argparse.ArgumentParser(
        description="Extract rotation data from GW2 ArcDPS report JSON files"
    )
    parser.add_argument("input_file", type=Path, help="Input ArcDPS report JSON file")
    parser.add_argument(
        "--output",
        "-o",
        type=Path,
        default="data/bench",
        help="Output file path (default: input_file_rotation.json)",
    )
    parser.add_argument(
        "--player",
        "-p",
        type=int,
        default=0,
        help="Player index to extract rotation from (default: 0)",
    )

    args = parser.parse_args()

    setup_logging()

    # Validate input file
    if not args.input_file.exists():
        logging.error(f"Input file does not exist: {args.input_file}")
        return 1

    if not args.input_file.suffix.lower() == ".json":
        logging.warning(f"Input file doesn't have .json extension: {args.input_file}")

    # Determine output file path
    output_path = args.output / f"{args.input_file.stem}_v3.json"

    try:
        # Read input file
        logging.info(f"Reading input file: {args.input_file}")
        with open(args.input_file, "r", encoding="utf-8") as f:
            arcdps_data = json.load(f)

        # Extract rotation and skillMap data
        extracted_data = extract_rotation_data(arcdps_data)

        if not extracted_data["rotation"]:
            logging.warning("No rotation data extracted")
            return 1

        # Sort rotation data by castTime (first element in array)
        extracted_data["rotation"] = sorted(
            extracted_data["rotation"],
            key=lambda x: x[0]  # Sort by castTime (first element)
        )

        # Wrap rotation in an additional array level for v3 format
        extracted_data["rotation"] = [extracted_data["rotation"]]

        # Save extracted data
        save_rotation_data(extracted_data, output_path)

        logging.info("✅ Rotation extraction completed successfully!")
        return 0

    except json.JSONDecodeError as e:
        logging.error(f"Invalid JSON in input file: {e}")
        return 1
    except FileNotFoundError:
        logging.error(f"Input file not found: {args.input_file}")
        return 1
    except Exception as e:
        logging.error(f"Unexpected error: {e}")
        return 1


if __name__ == "__main__":
    exit(main())
