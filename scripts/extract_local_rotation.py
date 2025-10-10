#!/usr/bin/env python3
"""
Extract rotation data from GW2 ArcDPS report JSON files.

This script reads a GW2 ArcDPS report JSON file and extracts rotation data,
converting it to a flattened format with skill_id included in each skill entry.
"""

import argparse
import json
import logging
from pathlib import Path
from typing import Dict, List, Any


def setup_logging():
    logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")


def extract_rotation_data(arcdps_data: Dict[str, Any]) -> List[Dict[str, Any]]:
    extracted_rotations = []

    try:
        # Navigate to rotation data
        players = arcdps_data.get("players", [])
        if not players:
            logging.warning("No players found in the data")
            return extracted_rotations

        # Get first player's rotation data (TODO: make player selectable)
        player = players[0]
        rotation = player.get("rotation", [])

        if not rotation:
            logging.warning("No rotation data found for first player")
            return extracted_rotations

        logging.info(f"Found rotation data with {len(rotation)} skill entries")

        # Process each skill in the rotation
        for skill_entry in rotation:
            skill_id = skill_entry.get("id")
            if skill_id is None:
                logging.warning("Skill entry missing 'id' field, skipping")
                continue

            skills = skill_entry.get("skills", [])

            # Flatten each skill cast
            for skill_cast in skills:
                # Create new entry with skill_id and all skill cast data
                flattened_entry = {
                    "skill_id": skill_id,
                    **skill_cast,  # Unpack all fields from the skill cast
                }
                extracted_rotations.append(flattened_entry)

        logging.info(f"Extracted {len(extracted_rotations)} rotation entries")

    except KeyError as e:
        logging.error(f"Missing required key in JSON structure: {e}")
    except Exception as e:
        logging.error(f"Error processing rotation data: {e}")

    return extracted_rotations


def save_rotation_data(rotation_data: List[Dict[str, Any]], output_path: Path):
    """
    Save extracted rotation data to JSON file.

    Args:
        rotation_data: List of extracted rotation entries
        output_path: Path to save the output file
    """
    try:
        with open(output_path, "w", encoding="utf-8") as f:
            json.dump(rotation_data, f, indent=2, ensure_ascii=False)
        logging.info(f"Saved {len(rotation_data)} rotation entries to: {output_path}")
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
    output_path = args.output / f"{args.input_file.stem}_rotation.json"

    try:
        # Read input file
        logging.info(f"Reading input file: {args.input_file}")
        with open(args.input_file, "r", encoding="utf-8") as f:
            arcdps_data = json.load(f)

        # Extract rotation data
        rotation_data = extract_rotation_data(arcdps_data)

        if not rotation_data:
            logging.warning("No rotation data extracted")
            return 1

        sort_rotation = sorted(rotation_data, key=lambda x: x.get("castTime", 0))

        # Save extracted data
        save_rotation_data(sort_rotation, output_path)

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
