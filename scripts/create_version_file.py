import argparse
import re
from pathlib import Path


def read_version_from_header(
    header_path: Path,
) -> str:
    if not header_path.exists():
        raise FileNotFoundError(f"Version header file not found: {header_path}")

    content = header_path.read_text(encoding="utf-8")

    # Extract version components using regex
    major_match = re.search(r"#define\s+MAJOR\s+(\d+)", content)
    minor_match = re.search(r"#define\s+MINOR\s+(\d+)", content)
    build_match = re.search(r"#define\s+BUILD\s+(\d+)", content)
    revision_match = re.search(r"#define\s+REVISION\s+(\d+)", content)

    if not all([major_match, minor_match, build_match, revision_match]):
        raise ValueError("Could not parse version defines from Version.h")

    major = major_match.group(1)  # type: ignore
    minor = minor_match.group(1)  # type: ignore
    build = build_match.group(1)  # type: ignore
    revision = revision_match.group(1)  # type: ignore

    return f"{major}.{minor}.{build}.{revision}"


def update_header_with_version_ranges(
    lower_version: str,
    upper_version: str,
    header_path: Path,
) -> None:
    if not header_path.exists():
        raise FileNotFoundError(f"Header file not found: {header_path}")

    try:
        content = header_path.read_text(encoding="utf-8")

        # Remove existing version range defines if they exist
        content = re.sub(
            r'#define\s+LOWER_VERSION_RANGE\s+"[\d\.]+"\s*\n?', "", content
        )
        content = re.sub(
            r'#define\s+UPPER_VERSION_RANGE\s+"[\d\.]+"\s*\n?', "", content
        )

        # Add the new version range defines after the VERSION_STRING define
        version_string_pattern = r"(#define VERSION_STRING[^\n]+\n)"
        replacement = f'\\1\n#define LOWER_VERSION_RANGE "{lower_version}"\n#define UPPER_VERSION_RANGE "{upper_version}"\n'

        content = re.sub(version_string_pattern, replacement, content)

        header_path.write_text(content, encoding="utf-8")
        print(f"[OK] Header file updated successfully: {header_path}")
        print("[INFO] Added version ranges:")
        print(f'[INFO]   LOWER_VERSION_RANGE = "{lower_version}"')
        print(f'[INFO]   UPPER_VERSION_RANGE = "{upper_version}"')

    except OSError as e:
        print(f"[ERROR] Error updating header file: {e}")
        raise


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Update header file with version ranges",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )

    parser.add_argument(
        "--lower_version",
        help="Lower version bound (e.g., 0.14.0.0)",
    )

    parser.add_argument(
        "--header_path",
        type=Path,
        default=Path("src/Version.h"),
        help="Path to Version.h file (default: src/Version.h)",
    )

    return parser.parse_args()


def main() -> None:
    try:
        args = parse_arguments()

        # Read current version from header
        print(f"[INFO] Reading current version from: {args.header_path}")
        current_version = read_version_from_header(args.header_path)
        print(f"[INFO] Current version from header: {current_version}")

        # Use provided lower version
        lower_version = args.lower_version
        print(f"[INFO] Using lower version: {lower_version}")

        print("[START] Updating header file with version ranges...")

        update_header_with_version_ranges(
            lower_version,
            current_version,
            args.header_path,
        )

    except KeyboardInterrupt:
        print("\n[WARNING] Operation cancelled by user")
    except Exception as e:
        print(f"[ERROR] Unexpected error: {e}")
        exit(1)


if __name__ == "__main__":
    main()
