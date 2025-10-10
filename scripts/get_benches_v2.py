import argparse
import json
import logging
from pathlib import Path


class BenchmarkDataExtractor:
    """Extract specific benchmark data from v1 JSON files"""

    def __init__(self, input_dir: Path, output_dir: Path | None = None):
        self.input_dir = input_dir
        self.output_dir = output_dir or input_dir

        # Setup logging
        logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')
        self.logger = logging.getLogger(__name__)

        # Create output directory
        self.output_dir.mkdir(exist_ok=True)

    def _extract_benchmark_data(self, log_data: dict) -> dict | None:
        """Extract specific benchmark data (rotation and skillMap)"""
        try:
            extracted_data = {}

            # Extract rotation data from players[0].details.rotation
            if ("players" in log_data and
                len(log_data["players"]) > 0 and
                "details" in log_data["players"][0] and
                "rotation" in log_data["players"][0]["details"]):
                extracted_data["rotation"] = log_data["players"][0]["details"]["rotation"]
            else:
                self.logger.warning("Could not find rotation data in log")

            # Extract skillMap
            if "skillMap" in log_data:
                extracted_data["skillMap"] = log_data["skillMap"]
            else:
                self.logger.warning("Could not find skillMap in log")

            return extracted_data if extracted_data else None

        except Exception as e:
            self.logger.error(f"Error extracting benchmark data: {e}")
            return None

    def process_json_file(self, json_file: Path) -> bool:
        """Process a single JSON file and extract benchmark data"""
        try:
            self.logger.info(f"Processing: {json_file.name}")

            # Read the v1 JSON file
            with open(json_file, 'r', encoding='utf-8') as f:
                log_data = json.load(f)

            # Extract benchmark data
            benchmark_data = self._extract_benchmark_data(log_data)
            if benchmark_data:
                # Generate output filename (replace _v1.json with _v2.json)
                if json_file.name.endswith('_v1.json'):
                    output_filename = json_file.name.replace('_v1.json', '_v2.json')
                else:
                    # If it doesn't end with _v1.json, just add _v2
                    stem = json_file.stem
                    output_filename = f"{stem}_v2.json"

                output_path = self.output_dir / output_filename

                # Save extracted benchmark data
                with open(output_path, 'w', encoding='utf-8') as f:
                    json.dump(benchmark_data, f, indent=2, ensure_ascii=False)

                self.logger.info(f"Saved extracted data: {output_filename}")
                return True
            else:
                self.logger.warning(f"No benchmark data could be extracted from {json_file.name}")
                return False

        except Exception as e:
            self.logger.error(f"Error processing {json_file.name}: {e}")
            return False

    def process_all_json_files(self, pattern: str = "*_v1.json") -> None:
        """Process all JSON files matching the pattern"""
        json_files = list(self.input_dir.glob(pattern))

        if not json_files:
            self.logger.warning(f"No JSON files found matching pattern '{pattern}' in {self.input_dir}")
            return

        success_count = 0
        total_count = len(json_files)

        self.logger.info(f"Found {total_count} JSON files to process")

        for json_file in json_files:
            if self.process_json_file(json_file):
                success_count += 1

        self.logger.info(f"Processing complete: {success_count}/{total_count} successful")

    def process_single_file(self, filename: str) -> bool:
        """Process a single JSON file by filename"""
        json_file = self.input_dir / filename

        if not json_file.exists():
            self.logger.error(f"File not found: {json_file}")
            return False

        return self.process_json_file(json_file)


def main():
    """Main function with CLI arguments"""
    parser = argparse.ArgumentParser(description='Extract benchmark data from v1 JSON files')
    parser.add_argument('--input', '-i', default='data/bench',
                       help='Input directory containing v1 JSON files (default: data/bench)')
    parser.add_argument('--output', '-o',
                       help='Output directory for v2 JSON files (default: same as input)')
    parser.add_argument('--file', '-f',
                       help='Process single file instead of all files')
    parser.add_argument('--pattern', '-p', default='*_v1.json',
                       help='File pattern to match (default: *_v1.json)')

    args = parser.parse_args()

    input_dir = Path(args.input)
    output_dir = Path(args.output) if args.output else input_dir

    if not input_dir.exists():
        print(f"❌ Input directory does not exist: {input_dir}")
        return

    print(f"📁 Input directory: {input_dir}")
    print(f"📁 Output directory: {output_dir}")

    extractor = BenchmarkDataExtractor(input_dir=input_dir, output_dir=output_dir)

    if args.file:
        # Process single file
        print(f"📄 Processing single file: {args.file}")
        success = extractor.process_single_file(args.file)
        if success:
            print("✅ File processed successfully!")
        else:
            print("❌ Failed to process file!")
    else:
        # Process all files matching pattern
        print(f"📄 Processing files matching pattern: {args.pattern}")
        extractor.process_all_json_files(args.pattern)
        print("✅ Processing completed!")


if __name__ == "__main__":
    main()
