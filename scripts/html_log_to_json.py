"""
Extract rotation data from downloaded HTML files containing benchmark report data.

This script reads HTML files from data/html directory and extracts rotation data
from the Player Summary -> Simple Rotation sections for DPS, Quick, and Alac benchmarks,
outputting in v4 format.
"""

import argparse
import html
import json
import logging
import operator
import re
import sys
from pathlib import Path
from typing import Any


def setup_logging() -> None:
    logging.basicConfig(level=logging.DEBUG, format="%(levelname)s: %(message)s")


class HTMLRotationExtractor:
    """Extract rotation data from DPS report HTML files"""

    def __init__(self, input_dir: Path, output_dir: Path) -> None:
        self.input_dir = input_dir
        self.output_dir = output_dir

        # Setup logging
        setup_logging()
        self.logger = logging.getLogger(__name__)

        # Create output directory
        self.output_dir.mkdir(parents=True, exist_ok=True)

        # Define skills where cancellation is acceptable (icon_id: reason)
        self.cancellation_allowed_skills = {
            "Spatial Surge",  # 10219 - Mesmer GS AA
            "Arc Lightning",  # 5526 -  Ele Air Scepter AA
        }

        # Load build metadata
        self.build_metadata = self._load_build_metadata()

    def _load_build_metadata(self) -> dict[str, dict[str, Any]]:
        """Load build metadata from build_metadata.json"""
        metadata_file = self.input_dir / "build_metadata.json"

        if not metadata_file.exists():
            self.logger.warning(f"Build metadata file not found: {metadata_file}")
            return {}

        try:
            with metadata_file.open(encoding="utf-8") as f:
                metadata_list = json.load(f)

            # Create a lookup dictionary with multiple keys for quick access
            metadata_dict = {}
            for build_info in metadata_list:
                # Use the html_file_path as key for lookup
                if "html_file_path" in build_info:
                    html_path = build_info["html_file_path"]
                    metadata_dict[html_path] = build_info
                    # Also normalize the path separators
                    normalized_path = html_path.replace("\\", "/")
                    metadata_dict[normalized_path] = build_info

                # Also create lookup by url_name with and without .html
                if "url_name" in build_info:
                    url_name = build_info["url_name"]
                    metadata_dict[url_name] = build_info
                    metadata_dict[url_name + ".html"] = build_info

                # Create lookup by name field as well
                if "name" in build_info:
                    name = build_info["name"]
                    # Convert spaces and special chars to match likely filenames
                    name_as_filename = name.lower().replace(" ", "_").replace("-", "_")
                    metadata_dict[name_as_filename] = build_info
                    metadata_dict[name_as_filename + ".html"] = build_info

            self.logger.info(
                f"Loaded metadata for {len(metadata_list)} builds with {len(metadata_dict)} lookup keys",
            )

            # Debug: show some sample keys
            sample_keys = list(metadata_dict.keys())[:10]
            self.logger.debug(f"Sample metadata keys: {sample_keys}")

            return metadata_dict

        except Exception as e:
            self.logger.exception(f"Error loading build metadata: {e}")
            return {}

    def _get_build_metadata_for_file(self, html_file: Path) -> dict[str, Any]:
        """Get build metadata for a specific HTML file"""
        # Debug logging
        self.logger.debug(f"Looking for metadata for file: {html_file}")

        # Try to find metadata by relative path
        try:
            relative_path = html_file.relative_to(self.input_dir)
            relative_path_str = str(relative_path).replace("\\", "/")

            self.logger.debug(f"Trying relative path: {relative_path_str}")
            if relative_path_str in self.build_metadata:
                self.logger.debug(
                    f"Found metadata by relative path: {relative_path_str}",
                )
                return self.build_metadata[relative_path_str]
        except ValueError:
            pass  # File is not relative to input_dir

        # Try to find by filename
        filename = html_file.name
        self.logger.debug(f"Trying filename: {filename}")
        if filename in self.build_metadata:
            self.logger.debug(f"Found metadata by filename: {filename}")
            return self.build_metadata[filename]

        # Try to find by stem (filename without extension)
        stem_html = html_file.stem + ".html"
        self.logger.debug(f"Trying stem + .html: {stem_html}")
        if stem_html in self.build_metadata:
            self.logger.debug(f"Found metadata by stem: {stem_html}")
            return self.build_metadata[stem_html]

        # Try to find by just the stem (url_name without .html)
        stem = html_file.stem
        self.logger.debug(f"Trying stem: {stem}")
        if stem in self.build_metadata:
            self.logger.debug(f"Found metadata by stem: {stem}")
            return self.build_metadata[stem]

        # Show available keys for debugging
        self.logger.debug(
            f"Available metadata keys: {list(self.build_metadata.keys())[:5]}...",
        )

        # Return empty dict if no metadata found
        self.logger.debug(f"No metadata found for {html_file.name}")
        return {}

    def _parse_skill_tooltip(self, tooltip: str) -> tuple[str, float, float]:
        """Parse skill information from tooltip text"""
        try:
            tooltip = html.unescape(tooltip)

            lines = tooltip.split("<br>")
            if not lines:
                return "Unknown", 0.0, 0.0

            first_line = lines[0]

            # Extract skill name (everything before " at ")
            skill_name = first_line.split(" at ")[0].strip() if " at " in first_line else "Unknown"

            # Extract cast time (e.g., "0.725s")
            cast_time = 0.0
            time_match = re.search(r"at\s+([-+]?\d+\.?\d*)s", first_line)
            if time_match:
                cast_time = float(time_match.group(1))

            # Extract duration (e.g., "3204ms")
            duration = 0.0
            duration_match = re.search(r"for\s+(\d+\.?\d*)ms", first_line)
            if duration_match:
                duration = float(duration_match.group(1))

            return skill_name, cast_time, duration

        except Exception as e:
            self.logger.warning(f"Error parsing tooltip '{tooltip}': {e}")
            return "Unknown", 0.0, 0.0

    def _extract_icon_id_from_url(self, img_src: str) -> int:
        """Extract skill ID from image URL"""
        try:
            # For weapon swap and utility skills, try to extract from the path
            if "render.guildwars2.com/file/" in img_src:
                # Split by '/' and get the last part (filename)
                parts = img_src.split("/")
                if len(parts) >= 2:
                    filename = parts[-1]  # e.g., "Button.png" or "12345.png"
                    hash_part = parts[-2]  # e.g., "c_ce_Weapon_Swap" or "hash"

                    # Try to extract number from filename first
                    filename_match = re.search(r"(\d+)\.png$", filename)
                    if filename_match:
                        return int(filename_match.group(1))

                    # For special cases like weapon swap, extract from hash part
                    hash_match = re.search(r"(\d+)", hash_part)
                    if hash_match:
                        return int(hash_match.group(1))

                    # For weapon swap specifically, use a known ID
                    if "weapon_swap" in hash_part.lower() or "weaponswap" in hash_part.lower():
                        return 9999  # Use a special ID for weapon swap

                    # Generate a hash-based ID for other special skills
                    return abs(hash(hash_part + filename)) % 1000000

            # Old format fallback: "/cache/https_render.guildwars2.com_file_hash_skill_id.png"
            match = re.search(r"_(\d+)\.png$", img_src)
            if match:
                return int(match.group(1))

            # Fallback: try to find any number in the URL
            numbers = re.findall(r"\d+", img_src)
            if numbers:
                return int(numbers[-1])  # Use the last number found

            return 0

        except Exception as e:
            self.logger.warning(f"Error extracting skill ID from URL '{img_src}': {e}")
            return 0

    def extract_rotation_from_html(self, html_file: Path) -> dict[str, Any]:
        """Extract rotation data from a single HTML file using regex"""
        try:
            self.logger.info(f"Processing HTML file: {html_file.name}")

            with html_file.open(encoding="utf-8") as f:
                html_content = f.read()

            # Use regex to find all rotation skill images, capturing class attribute to filter out cancelled skills
            # Pattern: <img src="https://render.guildwars2.com/..." data-original-title="..." class="rot-icon...">
            pattern = r'<img[^>]*src="(https:\/\/render\.guildwars2\.com\/[^"]*)"[^>]*data-original-title="([^"]*)"[^>]*class="([^"]*rot-icon[^"]*)"[^>]*>'
            matches = re.findall(pattern, html_content, re.DOTALL | re.IGNORECASE)

            if not matches:
                # Try alternative pattern with different attribute order
                pattern2 = r'<img[^>]*data-original-title="([^"]*)"[^>]*src="(https:\/\/render\.guildwars2\.com\/[^"]*)"[^>]*class="([^"]*rot-icon[^"]*)"[^>]*>'
                matches = [
                    (src, title, class_attr)
                    for title, src, class_attr in re.findall(
                        pattern2,
                        html_content,
                        re.DOTALL | re.IGNORECASE,
                    )
                ]

            if not matches:
                # Fallback pattern - any img with rot-icon class and gw2 render URL
                pattern3 = r'<img[^>]*class="([^"]*rot-icon[^"]*)"[^>]*src="(https:\/\/render\.guildwars2\.com\/[^"]*)"[^>]*data-original-title="([^"]*)"[^>]*>'
                matches = [
                    (src, title, class_attr)
                    for class_attr, src, title in re.findall(
                        pattern3,
                        html_content,
                        re.DOTALL | re.IGNORECASE,
                    )
                ]

            if not matches:
                self.logger.warning(f"No rotation skills found in {html_file.name}")
                return {"rotation": [[]], "skillMap": {}}

            rotation_entries = []
            skill_map = {}

            for img_src, tooltip, class_attr in matches:
                if not tooltip:
                    continue

                # Parse skill information first to get skill_id for cancellation check
                skill_name, cast_time, duration = self._parse_skill_tooltip(tooltip)
                icon_id = self._extract_icon_id_from_url(img_src)

                if icon_id == 0:
                    continue

                # Check if skill is cancelled and if cancellation is allowed for this skill
                if (
                    "rot-cancelled" in class_attr
                    and skill_name in self.cancellation_allowed_skills
                    and skill_name == "Spatial Surge"
                    and duration < 300.0
                ):
                    self.logger.debug(
                        f"Skipping {skill_name} (ID: {icon_id}) - duration {duration}ms below 40ms threshold",
                    )
                    continue

                # Create rotation entry in v3 format: [castTime, icon_id, duration, status, quickness]
                # We don't have status or quickness info from HTML, so use defaults
                rotation_entry = [
                    cast_time,  # Cast time in seconds
                    icon_id,  # Skill ID
                    duration,  # Duration in ms
                    1,  # Status (default to 1, meaning successful cast)
                    1.0,  # Quickness (default to 1.0, no quickness effect)
                ]

                rotation_entries.append(rotation_entry)

                # Add to skill map if not already present
                skill_key = f"s{icon_id}"
                if skill_key not in skill_map:
                    # Try to construct skill map entry
                    # We don't have all the info from HTML, so create minimal entry
                    skill_map[skill_key] = {
                        "name": skill_name,
                        "icon": img_src if img_src.startswith("http") else f"https://dps.report{img_src}",
                        "traitProc": False,  # Default values since we don't have this info
                        "gearProc": False,
                    }

            # Sort rotation by cast time
            rotation_entries.sort(key=operator.itemgetter(0))

            # Get build metadata for this file
            build_metadata = self._get_build_metadata_for_file(html_file)

            # Create the output structure with build metadata
            result = {
                "rotation": [rotation_entries],  # Wrapped in array for v3 format
                "skillMap": skill_map,
            }

            # Add build metadata if available
            if build_metadata:
                # For manual logs, prefer sc_link_url over url if available
                url_value = build_metadata.get("sc_link_url", "") or build_metadata.get("url", "")
                skill_slots = build_metadata.get("skill_slots", {})
                if len(skill_slots) != 5:
                    skill_slots = {}

                result["buildMetadata"] = {
                    "name": build_metadata.get("name", "Unknown"),
                    "profession": build_metadata.get("profession", "Unknown"),
                    "elite_spec": build_metadata.get("elite_spec", ""),
                    "build_type": build_metadata.get("build_type", "power"),
                    "benchmark_type": build_metadata.get("benchmark_type", "dps"),
                    "url": url_value,
                    "dps_report_url": build_metadata.get("dps_report_url", ""),
                    "overall_dps": build_metadata.get("overall_dps"),
                    "skill_slots": skill_slots,
                }
            else:
                raise ValueError(f"No build metadata found for file: {html_file.name}")

            # Log extraction results with profession info
            profession_info = result["buildMetadata"]["profession"]  # type: ignore
            elite_spec = result["buildMetadata"]["elite_spec"]  # type: ignore
            if elite_spec:
                profession_info += f" ({elite_spec})"

            self.logger.info(
                f"Extracted {len(rotation_entries)} rotation entries from {html_file.name} - {profession_info}",
            )

            return result

        except Exception as e:
            self.logger.exception(f"Error processing {html_file.name}: {e}")
            return {"rotation": [[]], "skillMap": {}}

    def _determine_benchmark_type(self, html_file: Path) -> str:
        """Determine benchmark type (dps/quick/alac) from file path"""
        path_parts = html_file.parts

        # Check if file is in a subdirectory that indicates benchmark type
        if "dps" in path_parts:
            return "dps"
        if "quick" in path_parts:
            return "quick"
        if "alac" in path_parts:
            return "alac"

        # Default to dps if cannot determine
        return "dps"

    def _determine_build_type(self, html_file: Path) -> str:
        """Determine build type (power/condition) from file path or filename"""
        # Check if file is in a subdirectory that indicates build type
        if "power" in str(html_file.parent):
            return "power"
        if "condition" in str(html_file.parent):
            return "condition"

        # Check filename for build type indicators
        filename = html_file.stem.lower()
        if filename.startswith("condition") or "condition" in filename:
            return "condition"
        if filename.startswith("power") or "power" in filename:
            return "power"

        # Default to power if cannot determine
        return "power"

    def process_all_html_files(self, pattern: str = "*.html") -> None:
        """Process all HTML files in the input directory and subdirectories"""
        # First, try to process files from metadata if available
        html_files = []
        metadata_files_processed = set()

        if self.build_metadata:
            # Process files that have metadata first
            for build_info in self.build_metadata.values():
                if "html_file_path" in build_info:
                    html_path = self.input_dir / build_info["html_file_path"]
                    if html_path.exists() and html_path.suffix.lower() == ".html":
                        html_files.append(html_path)
                        metadata_files_processed.add(str(html_path))

            self.logger.info(f"Found {len(html_files)} HTML files from metadata")

        # unique lements in html_files
        html_files = list({str(f): f for f in html_files}.values())
        # Then find any additional HTML files not in metadata
        additional_files: list[Path] = []
        benchmark_types = ["dps", "quick", "alac"]
        build_types = ["power", "condition"]

        # Check each benchmark type directory
        for benchmark_type in benchmark_types:
            benchmark_dir = self.input_dir / benchmark_type
            if benchmark_dir.exists():
                # Check each build type subdirectory
                for build_type in build_types:
                    build_dir = benchmark_dir / build_type
                    if build_dir.exists():
                        additional_files.extend(
                            file_path
                            for file_path in build_dir.glob(pattern)
                            if str(file_path) not in metadata_files_processed
                        )

        # Also check main directory for any standalone files
        additional_files.extend(
            file_path for file_path in self.input_dir.glob(pattern) if str(file_path) not in metadata_files_processed
        )

        html_files.extend(additional_files)

        if additional_files:
            self.logger.info(
                f"Found {len(additional_files)} additional HTML files without metadata",
            )

        if not html_files:
            self.logger.warning(
                f"No HTML files found matching pattern '{pattern}' in {self.input_dir} or subdirectories",
            )
            return

        success_count = 0
        total_count = len(html_files)

        self.logger.info(f"Processing {total_count} HTML files total")

        for html_file in html_files:
            try:
                # Extract rotation data
                extracted_data = self.extract_rotation_from_html(html_file)

                if not extracted_data["rotation"][0]:  # Check if rotation array is empty
                    self.logger.warning(
                        f"No rotation data extracted from {html_file.name}",
                    )
                    print("Invalid Rotation Data for file:", html_file.name)
                    continue

                # Use metadata for benchmark and build types if available, otherwise determine from path
                build_metadata = extracted_data.get("buildMetadata", {})
                benchmark_type = build_metadata.get(
                    "benchmark_type",
                    self._determine_benchmark_type(html_file),
                )
                build_type = build_metadata.get(
                    "build_type",
                    self._determine_build_type(html_file),
                )

                # Create appropriate output subdirectory
                output_subdir = self.output_dir / benchmark_type / build_type
                output_subdir.mkdir(parents=True, exist_ok=True)

                # Generate output filename (replace .html with _v4.json)
                output_filename = html_file.stem + "_v4.json"
                output_path = output_subdir / output_filename

                # Save extracted data
                with output_path.open("w", encoding="utf-8") as f:
                    json.dump(extracted_data, f, indent=2, ensure_ascii=False)

                profession_info = build_metadata.get("profession", "Unknown")
                elite_spec = build_metadata.get("elite_spec", "")
                if elite_spec:
                    profession_info += f" ({elite_spec})"

                self.logger.info(
                    f"Saved: {benchmark_type}/{build_type}/{output_filename} - {profession_info}",
                )
                success_count += 1

            except Exception as e:
                self.logger.exception(f"Error processing {html_file.name}: {e}")
                continue

        self.logger.info(
            f"Processing complete: {success_count}/{total_count} successful",
        )

    def process_single_file(self, filename: str) -> bool:
        """Process a single HTML file by filename"""
        html_file = None

        # Try to find the file in various locations
        benchmark_types = ["dps", "quick", "alac"]
        build_types = ["power", "condition"]

        possible_paths = [self.input_dir / filename]

        # Add all possible benchmark/build type combinations
        for benchmark_type in benchmark_types:
            possible_paths.extend(self.input_dir / benchmark_type / build_type / filename for build_type in build_types)

        for path in possible_paths:
            if path.exists():
                html_file = path
                break

        if html_file is None:
            self.logger.error(
                f"File not found: {filename} (searched in input directory and subdirectories)",
            )
            return False

        if html_file.suffix.lower() != ".html":
            self.logger.error(f"File is not an HTML file: {html_file}")
            return False

        try:
            # Extract rotation data
            extracted_data = self.extract_rotation_from_html(html_file)

            if not extracted_data["rotation"][0]:  # Check if rotation array is empty
                self.logger.warning(f"No rotation data extracted from {html_file.name}")
                return False

            # Use metadata for benchmark and build types if available, otherwise determine from path
            build_metadata = extracted_data.get("buildMetadata", {})
            benchmark_type = build_metadata.get(
                "benchmark_type",
                self._determine_benchmark_type(html_file),
            )
            build_type = build_metadata.get(
                "build_type",
                self._determine_build_type(html_file),
            )

            # Create appropriate output subdirectory
            output_subdir = self.output_dir / benchmark_type / build_type
            output_subdir.mkdir(parents=True, exist_ok=True)

            # Generate output filename
            output_filename = html_file.stem + "_v4.json"
            output_path = output_subdir / output_filename

            # Save extracted data
            with output_path.open("w", encoding="utf-8") as f:
                json.dump(extracted_data, f, indent=2, ensure_ascii=False)

            profession_info = build_metadata.get("profession", "Unknown")
            elite_spec = build_metadata.get("elite_spec", "")
            if elite_spec:
                profession_info += f" ({elite_spec})"

            self.logger.info(
                f"Saved: {benchmark_type}/{build_type}/{output_filename} - {profession_info}",
            )
            return True

        except Exception as e:
            self.logger.exception(f"Error processing {html_file.name}: {e}")
            return False


def main() -> int:
    """Main function with CLI interface"""
    parser = argparse.ArgumentParser(
        description="Extract rotation data from benchmark report HTML files (DPS, Quick, Alac)",
    )
    parser.add_argument(
        "--input",
        "-i",
        type=Path,
        default="internal_data/html",
        help="Input directory containing HTML files (default: internal_data/html)",
    )
    parser.add_argument(
        "--output",
        "-o",
        type=Path,
        default="data/bench",
        help="Output directory for JSON files (default: data/bench)",
    )
    parser.add_argument("--file", "-f", help="Process single file instead of all files")
    parser.add_argument(
        "--pattern",
        "-p",
        default="*.html",
        help="File pattern to match (default: *.html)",
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
        print("❌ Failed to process file!")
        return 1
    # Process all files matching pattern
    print(f"📄 Processing files matching pattern: {args.pattern}")
    extractor.process_all_html_files(args.pattern)
    print("✅ Processing completed!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
