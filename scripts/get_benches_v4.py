#!/usr/bin/env python3
"""
Extract rotation data from downloaded HTML files containing DPS report data.

This script reads HTML files from data/html directory and extracts rotation data
from the Player Summary -> Simple Rotation sections, outputting in v3 format.
"""

import argparse
import json
import logging
import re
from pathlib import Path
from typing import Dict, Any, Tuple


def setup_logging():
    logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")


class HTMLRotationExtractor:
    """Extract rotation data from DPS report HTML files"""

    def __init__(self, input_dir: Path, output_dir: Path):
        self.input_dir = input_dir
        self.output_dir = output_dir

        # Setup logging
        setup_logging()
        self.logger = logging.getLogger(__name__)

        # Create output directory
        self.output_dir.mkdir(parents=True, exist_ok=True)

    def _parse_skill_tooltip(self, tooltip: str) -> Tuple[str, float, float]:
        """Parse skill information from tooltip text"""
        try:
            # Example: "Overload Air at -3.083s for 3204ms<br>Overload Air cast 1 of 4<br>Skill cast 1 of 158"
            lines = tooltip.split('<br>')
            if not lines:
                return "Unknown", 0.0, 0.0

            first_line = lines[0]

            # Extract skill name (everything before " at ")
            if " at " in first_line:
                skill_name = first_line.split(" at ")[0].strip()
            else:
                skill_name = "Unknown"

            # Extract cast time (e.g., "-3.083s")
            cast_time = 0.0
            time_match = re.search(r'at\s+([-+]?\d+\.?\d*)s', first_line)
            if time_match:
                cast_time = float(time_match.group(1))

            # Extract duration (e.g., "3204ms")
            duration = 0.0
            duration_match = re.search(r'for\s+(\d+\.?\d*)ms', first_line)
            if duration_match:
                duration = float(duration_match.group(1))

            return skill_name, cast_time, duration

        except Exception as e:
            self.logger.warning(f"Error parsing tooltip '{tooltip}': {e}")
            return "Unknown", 0.0, 0.0

    def _extract_skill_id_from_url(self, img_src: str) -> int:
        """Extract skill ID from image URL"""
        try:
            # Example: "/cache/https_render.guildwars2.com_file_592CC2A120322000BC7234B8522BBE7BAF2F4A57_1029987.png"
            # The skill ID is typically the number at the end
            match = re.search(r'_(\d+)\.png$', img_src)
            if match:
                return int(match.group(1))

            # Fallback: try to find any number in the URL
            numbers = re.findall(r'\d+', img_src)
            if numbers:
                return int(numbers[-1])  # Use the last number found

            return 0

        except Exception as e:
            self.logger.warning(f"Error extracting skill ID from URL '{img_src}': {e}")
            return 0

    def extract_rotation_from_html(self, html_file: Path) -> Dict[str, Any]:
        """Extract rotation data from a single HTML file using regex"""
        try:
            self.logger.info(f"Processing HTML file: {html_file.name}")

            with open(html_file, 'r', encoding='utf-8') as f:
                html_content = f.read()

            # Use regex to find all rotation skill spans
            # Pattern: <span class="rot-skill"><img src="..." data-original-title="..." class="rot-icon"></span>
            pattern = r'<span[^>]*class="rot-skill"[^>]*>.*?<img[^>]*src="([^"]*)"[^>]*data-original-title="([^"]*)"[^>]*class="rot-icon"[^>]*>.*?</span>'
            matches = re.findall(pattern, html_content, re.DOTALL | re.IGNORECASE)

            if not matches:
                # Try alternative pattern in case the order is different
                pattern2 = r'<span[^>]*class="rot-skill"[^>]*>.*?<img[^>]*class="rot-icon"[^>]*src="([^"]*)"[^>]*data-original-title="([^"]*)"[^>]*>.*?</span>'
                matches = re.findall(pattern2, html_content, re.DOTALL | re.IGNORECASE)

            if not matches:
                self.logger.warning(f"No rotation skills found in {html_file.name}")
                return {"rotation": [[]], "skillMap": {}}

            rotation_entries = []
            skill_map = {}

            for img_src, tooltip in matches:
                if not tooltip:
                    continue

                # Parse skill information
                skill_name, cast_time, duration = self._parse_skill_tooltip(tooltip)
                skill_id = self._extract_skill_id_from_url(img_src)

                if skill_id == 0:
                    continue

                # Create rotation entry in v3 format: [castTime, skill_id, duration, status, quickness]
                # We don't have status or quickness info from HTML, so use defaults
                rotation_entry = [
                    cast_time,      # Cast time in seconds
                    skill_id,       # Skill ID
                    duration,       # Duration in ms
                    1,              # Status (default to 1, meaning successful cast)
                    1.0             # Quickness (default to 1.0, no quickness effect)
                ]

                rotation_entries.append(rotation_entry)

                # Add to skill map if not already present
                skill_key = f"s{skill_id}"
                if skill_key not in skill_map:
                    # Try to construct skill map entry
                    # We don't have all the info from HTML, so create minimal entry
                    skill_map[skill_key] = {
                        "name": skill_name,
                        "icon": img_src if img_src.startswith('http') else f"https://dps.report{img_src}",
                        "traitProc": False,  # Default values since we don't have this info
                        "gearProc": False
                    }

            # Sort rotation by cast time
            rotation_entries.sort(key=lambda x: x[0])

            self.logger.info(f"Extracted {len(rotation_entries)} rotation entries from {html_file.name}")

            # Return in v3 format (wrapped in an additional array)
            return {
                "rotation": [rotation_entries],  # Wrapped in array for v3 format
                "skillMap": skill_map
            }

        except Exception as e:
            self.logger.error(f"Error processing {html_file.name}: {e}")
            return {"rotation": [[]], "skillMap": {}}

    def _determine_build_type(self, html_file: Path) -> str:
        """Determine build type (power/condition) from file path or filename"""
        # Check if file is in a subdirectory that indicates build type
        if "power" in str(html_file.parent):
            return "power"
        elif "condition" in str(html_file.parent):
            return "condition"

        # Check filename for build type indicators
        filename = html_file.stem.lower()
        if filename.startswith("condition") or "condition" in filename:
            return "condition"
        elif filename.startswith("power") or "power" in filename:
            return "power"

        # Default to power if cannot determine
        return "power"

    def process_all_html_files(self, pattern: str = "*.html") -> None:
        """Process all HTML files in the input directory and subdirectories"""
        # Look for HTML files in the main directory and subdirectories
        html_files = []

        # Check main directory
        html_files.extend(list(self.input_dir.glob(pattern)))

        # Check dps subdirectories
        dps_dir = self.input_dir / "dps"
        if dps_dir.exists():
            # Check power subdirectory
            power_dir = dps_dir / "power"
            if power_dir.exists():
                html_files.extend(list(power_dir.glob(pattern)))

            # Check condition subdirectory
            condition_dir = dps_dir / "condition"
            if condition_dir.exists():
                html_files.extend(list(condition_dir.glob(pattern)))

        if not html_files:
            self.logger.warning(f"No HTML files found matching pattern '{pattern}' in {self.input_dir} or subdirectories")
            return

        success_count = 0
        total_count = len(html_files)

        self.logger.info(f"Found {total_count} HTML files to process")

        for html_file in html_files:
            try:
                # Extract rotation data
                extracted_data = self.extract_rotation_from_html(html_file)

                if not extracted_data["rotation"][0]:  # Check if rotation array is empty
                    self.logger.warning(f"No rotation data extracted from {html_file.name}")
                    continue

                # Determine build type from filename or parent directory
                build_type = self._determine_build_type(html_file)

                # Create appropriate output subdirectory
                output_subdir = self.output_dir / "dps" / build_type
                output_subdir.mkdir(parents=True, exist_ok=True)

                # Generate output filename (replace .html with _v4.json)
                output_filename = html_file.stem + "_v4.json"
                output_path = output_subdir / output_filename

                # Save extracted data
                with open(output_path, 'w', encoding='utf-8') as f:
                    json.dump(extracted_data, f, indent=2, ensure_ascii=False)

                self.logger.info(f"Saved: dps/{build_type}/{output_filename}")
                success_count += 1

            except Exception as e:
                self.logger.error(f"Error processing {html_file.name}: {e}")
                continue

        self.logger.info(f"Processing complete: {success_count}/{total_count} successful")

    def process_single_file(self, filename: str) -> bool:
        """Process a single HTML file by filename"""
        html_file = None

        # Try to find the file in various locations
        possible_paths = [
            self.input_dir / filename,
            self.input_dir / "dps" / "power" / filename,
            self.input_dir / "dps" / "condition" / filename,
        ]

        for path in possible_paths:
            if path.exists():
                html_file = path
                break

        if html_file is None:
            self.logger.error(f"File not found: {filename} (searched in input directory and subdirectories)")
            return False

        if not html_file.suffix.lower() == '.html':
            self.logger.error(f"File is not an HTML file: {html_file}")
            return False

        try:
            # Extract rotation data
            extracted_data = self.extract_rotation_from_html(html_file)

            if not extracted_data["rotation"][0]:  # Check if rotation array is empty
                self.logger.warning(f"No rotation data extracted from {html_file.name}")
                return False

            # Determine build type from filename or parent directory
            build_type = self._determine_build_type(html_file)

            # Create appropriate output subdirectory
            output_subdir = self.output_dir / "dps" / build_type
            output_subdir.mkdir(parents=True, exist_ok=True)

            # Generate output filename
            output_filename = html_file.stem + "_v4.json"
            output_path = output_subdir / output_filename

            # Save extracted data
            with open(output_path, 'w', encoding='utf-8') as f:
                json.dump(extracted_data, f, indent=2, ensure_ascii=False)

            self.logger.info(f"Saved: dps/{build_type}/{output_filename}")
            return True

        except Exception as e:
            self.logger.error(f"Error processing {html_file.name}: {e}")
            return False


def main():
    """Main function with CLI interface"""
    parser = argparse.ArgumentParser(
        description="Extract rotation data from DPS report HTML files"
    )
    parser.add_argument(
        "--input", "-i",
        type=Path,
        default="data/html",
        help="Input directory containing HTML files (default: data/html)"
    )
    parser.add_argument(
        "--output", "-o",
        type=Path,
        default="data/bench",
        help="Output directory for JSON files (default: data/bench)"
    )
    parser.add_argument(
        "--file", "-f",
        help="Process single file instead of all files"
    )
    parser.add_argument(
        "--pattern", "-p",
        default="*.html",
        help="File pattern to match (default: *.html)"
    )

    args = parser.parse_args()

    input_dir = Path(args.input)
    output_dir = Path(args.output)

    if not input_dir.exists():
        print(f"❌ Input directory does not exist: {input_dir}")
        return 1

    print(f"📁 Input directory: {input_dir}")
    print(f"📁 Output directory: {output_dir}")

    extractor = HTMLRotationExtractor(input_dir=input_dir, output_dir=output_dir)

    if args.file:
        # Process single file
        print(f"📄 Processing single file: {args.file}")
        success = extractor.process_single_file(args.file)
        if success:
            print("✅ File processed successfully!")
            return 0
        else:
            print("❌ Failed to process file!")
            return 1
    else:
        # Process all files matching pattern
        print(f"📄 Processing files matching pattern: {args.pattern}")
        extractor.process_all_html_files(args.pattern)
        print("✅ Processing completed!")
        return 0


if __name__ == "__main__":
    exit(main())
