"""
Modern Python script to create a version file with command-line arguments.
Creates a file with VERSION = {NUMBER} format.
"""

import argparse
from pathlib import Path


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
        "-v",
        "--version",
        help="Version number to write to the file",
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

        print("[START] Creating version file...")
        print(f"[INFO] Version: {args.version}")
        print(f"[INFO] Output directory: {args.output.resolve()}")

        create_version_file(args.version, args.output)

    except KeyboardInterrupt:
        print("\n[WARNING] Operation cancelled by user")
    except Exception as e:
        print(f"[ERROR] Unexpected error: {e}")
        exit(1)


if __name__ == "__main__":
    main()
