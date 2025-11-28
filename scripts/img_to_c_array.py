#!/usr/bin/env python3
"""
Convert PNG image to C++ char array for embedding in source code.

This script reads a PNG image file and converts it to a C++ char array
that can be embedded directly in source code as a binary resource.
"""

import argparse
import sys
from pathlib import Path


def png_to_c_array(input_path: Path, array_name: str = "image_data") -> bool:
    """Convert PNG image to C++ char array.

    Args:
        input_path: Path to the input PNG file
        array_name: Name for the generated array variable

    Returns:
        True if successful, False otherwise
    """
    try:
        # Generate output path in same directory as input
        output_path = input_path.parent / f"{input_path.stem}_data.h"
        # Read the PNG file as binary data
        image_data = input_path.read_bytes()

        if not image_data:
            print(f"Error: Empty file or failed to read {input_path}")
            return False

        # Generate C++ header content
        header_content = f"""// Auto-generated from {input_path.name}
// Total size: {len(image_data)} bytes

#pragma once

#include <cstdint>

// Image data as char array
constexpr std::size_t {array_name}_size = {len(image_data)};
constexpr unsigned char {array_name}[{len(image_data)}] = {{
"""

        # Convert bytes to C++ array format
        bytes_per_line = 16
        for i in range(0, len(image_data), bytes_per_line):
            line_data = image_data[i:i + bytes_per_line]
            hex_values = [f"0x{byte:02x}" for byte in line_data]

            # Add indentation and format
            if i + bytes_per_line < len(image_data):
                header_content += "    " + ", ".join(hex_values) + ",\n"
            else:
                # Last line, no trailing comma
                header_content += "    " + ", ".join(hex_values) + "\n"

        header_content += "};\n"

        # Write to output file
        output_path.write_text(header_content, encoding="utf-8")

        print(f"✅ Successfully converted {input_path.name} to {output_path.name}")
        print(f"   Array name: {array_name}")
        print(f"   Array size: {len(image_data)} bytes")
        return True

    except FileNotFoundError:
        print(f"Error: Input file not found: {input_path}")
        return False
    except PermissionError:
        print("Error: Permission denied accessing files")
        return False
    except Exception as e:
        print(f"Error: Failed to convert image: {e}")
        return False


def main() -> int:
    """Main function with CLI interface."""
    parser = argparse.ArgumentParser(
        description="Convert 2 PNG images to C++ char arrays for embedding in source code",
    )
    parser.add_argument(
        "input1",
        type=Path,
        help="First PNG image file",
    )
    parser.add_argument(
        "input2",
        type=Path,
        help="Second PNG image file",
    )
    parser.add_argument(
        "--array-name1",
        "-n1",
        default="image_data1",
        help="Name for the first array variable (default: image_data1)",
    )
    parser.add_argument(
        "--array-name2",
        "-n2",
        default="image_data2",
        help="Name for the second array variable (default: image_data2)",
    )

    args = parser.parse_args()

    # Process both input files
    inputs = [(args.input1, args.array_name1), (args.input2, args.array_name2)]
    all_success = True

    for i, (input_path, array_name) in enumerate(inputs, 1):
        print(f"\n--- Processing image {i} ---")

        # Validate input file
        if not input_path.exists():
            print(f"Error: Input file does not exist: {input_path}")
            all_success = False
            continue

        if input_path.suffix.lower() != ".png":
            print(f"Warning: Input file does not have .png extension: {input_path}")

        # Validate array name
        if not array_name.isidentifier():
            print(f"Error: Invalid array name '{array_name}'. Must be a valid C++ identifier.")
            all_success = False
            continue

        # Convert the image
        success = png_to_c_array(input_path, array_name)
        if not success:
            all_success = False

    return 0 if all_success else 1


if __name__ == "__main__":
    sys.exit(main())
