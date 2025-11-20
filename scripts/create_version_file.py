"""
Modern Python script to create a version file with command-line arguments.
Creates a file with VERSION = {NUMBER} format.
Reads version from src/Version.h if no version is provided.
"""

import argparse
import re
from pathlib import Path


def read_version_from_header(header_path: Path) -> str:
    """
    Read version from src/Version.h file by parsing MAJOR, MINOR, BUILD defines.

    Args:
        header_path: Path to the Version.h file

    Returns:
        Version string in format "MAJOR.MINOR.BUILD"
    """
    if not header_path.exists():
        raise FileNotFoundError(f"Version header file not found: {header_path}")

    content = header_path.read_text(encoding="utf-8")

    # Extract version components using regex
    major_match = re.search(r'#define\s+MAJOR\s+(\d+)', content)
    minor_match = re.search(r'#define\s+MINOR\s+(\d+)', content)
    build_match = re.search(r'#define\s+BUILD\s+(\d+)', content)

    if not all([major_match, minor_match, build_match]):
        raise ValueError("Could not parse version defines from Version.h")

    major = major_match.group(1)  # type: ignore
    minor = minor_match.group(1)  # type: ignore
    build = build_match.group(1)  # type: ignore

    return f"{major}.{minor}.{build}"


def create_version_file(version: str, output_dir: Path) -> None:
    """
    Create a version file with the specified version number.

    Args:
        version: Version number as string
        output_dir: Directory where to create the version file
    """
    # Ensure output directory exists
    output_dir.mkdir(parents=True, exist_ok=True)

    # Create the version file path
    version_file = output_dir / "VERSION.txt"

    # Write the version content
    content = f"VERSION = {version}\n"

    try:
        version_file.write_text(content, encoding="utf-8")
        print(f"[OK] Version file created successfully: {version_file}")
        print(f"[INFO] Content: {content.strip()}")
    except OSError as e:
        print(f"[ERROR] Error creating version file: {e}")
        raise


def parse_arguments() -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Create a version file with VERSION = {NUMBER} format",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )

    parser.add_argument(
        "--header_path",
        type=Path,
        default=Path("src/Version.h"),
        help="Path to Version.h file (default: src/Version.h)",
    )

    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=Path("./data"),
        help="Output directory for the version file (default: ./data)",
    )

    return parser.parse_args()


def main() -> None:
    """Main entry point."""
    try:
        args = parse_arguments()

        print(f"[INFO] Reading version from: {args.header_path}")
        version = read_version_from_header(args.header_path)
        print(f"[INFO] Parsed version from header: {version}")

        print("[START] Creating version file...")
        print(f"[INFO] Output directory: {args.output.resolve()}")

        create_version_file(version, args.output)

    except KeyboardInterrupt:
        print("\n[WARNING] Operation cancelled by user")
    except Exception as e:
        print(f"[ERROR] Unexpected error: {e}")
        exit(1)


if __name__ == "__main__":
    main()
