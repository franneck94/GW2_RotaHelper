# type: ignore
"""
Snow Crows skill mapping extractor.

This script extracts skill slot mappings from SnowCrows build pages
and saves them to a JSON file for use in rotation helpers.
"""

import argparse
import json
import logging
import re
import time
from pathlib import Path

import requests
from selenium import webdriver
from selenium.webdriver.chrome.options import Options
from selenium.webdriver.common.by import By
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait


SLEEP_DELAY = 10.5


class SnowCrowsSkillExtractor:
    """Extractor for skill slot mappings from SnowCrows build pages"""

    def __init__(
        self,
        output_dir: Path,
        delay: float = 1.0,
        headless: bool = True,
    ) -> None:
        self.output_dir = output_dir
        self.delay = delay
        self.headless = headless
        self.driver = None

        logging.basicConfig(
            level=logging.INFO,
            format="%(levelname)s: %(message)s",
        )
        self.logger = logging.getLogger(__name__)

        self.output_dir.mkdir(parents=True, exist_ok=True)

    def _setup_webdriver(self) -> None:
        """Setup Chrome WebDriver with appropriate options"""
        if self.driver is not None:
            return

        chrome_options = Options()
        if self.headless:
            chrome_options.add_argument("--headless")
        chrome_options.add_argument("--no-sandbox")
        chrome_options.add_argument("--disable-dev-shm-usage")
        chrome_options.add_argument("--disable-gpu")
        chrome_options.add_argument("--window-size=1920,1080")
        chrome_options.add_argument(
            "--user-agent=Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
            "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36",
        )

        try:
            self.driver = webdriver.Chrome(options=chrome_options)
            self.logger.info("Chrome WebDriver initialized successfully")
        except Exception as e:
            self.logger.exception(f"Failed to initialize WebDriver: {e}")
            raise

    def _cleanup_webdriver(self) -> None:
        """Cleanup WebDriver resources"""
        if self.driver:
            self.driver.quit()
            self.driver = None
            self.logger.info("WebDriver cleaned up")

    def _extract_skill_slots(self, url: str) -> dict[str, str]:
        """Extract skill slot information from SnowCrows build page"""
        try:
            if self.driver is None:
                self._setup_webdriver()

            if "snowcrows.com" not in url or self.driver is None:
                return {}

            self.logger.info("Extracting skill slot information from SnowCrows build page...")

            # Make sure we're on the build page
            current_url = self.driver.current_url
            if url != current_url:
                self.driver.get(url)
                WebDriverWait(self.driver, 10).until(
                    EC.presence_of_element_located((By.TAG_NAME, "body")),
                )
                time.sleep(SLEEP_DELAY)

            # Look for the Skills section - find div with "Skills" text
            skills_sections = self.driver.find_elements(
                By.XPATH,
                "//div[contains(@class, 'text-right') and contains(@class, 'flex-grow') and contains(@class, 'text-2xl') and contains(text(), 'Skills')]",
            )

            if not skills_sections:
                self.logger.warning("Could not find Skills section on build page")
                return {}

            self.logger.info(f"Found {len(skills_sections)} Skills section(s)")

            # Get the parent container that has the skill icons
            skills_container = skills_sections[0].find_element(
                By.XPATH, "./ancestor::div[contains(@class, 'relative')]"
            )

            # Find all skill icon divs within this container
            skill_divs = skills_container.find_elements(
                By.XPATH,
                ".//div[contains(@class, 'gw2a--Wvchy') and contains(@class, 'gw2a--s2qls') and contains(@class, 'gw2a--376lb') and contains(@class, 'gw2a--3u_DO')]",
            )

            if not skill_divs:
                self.logger.warning("Could not find skill icon divs in Skills section")
                return {}

            self.logger.info(f"Found {len(skill_divs)} skill icons")

            # Extract skill URLs and map them to slot numbers
            skill_slots = {}

            # Standard GW2 skill slot mapping for utilities:
            slot_mapping = {
                0: "6",  # Heal
                1: "7",  # Utility 1
                2: "8",  # Utility 2
                3: "9",  # Utility 3
                4: "0",  # Elite
            }

            for i, skill_div in enumerate(skill_divs):
                try:
                    # Extract background-image URL from style attribute
                    style = skill_div.get_attribute("style") or ""

                    # Parse background-image URL using regex
                    bg_match = re.search(r"background-image:\s*url\(&quot;([^&]+)&quot;\)", style)
                    if not bg_match:
                        # Try alternative format without &quot;
                        bg_match = re.search(r'background-image:\s*url\(["\']([^"\']+)["\'\))]', style)

                    if bg_match:
                        skill_url = bg_match.group(1)
                        skill_icon_id = skill_url.split("/")[-1].split(".")[0]

                        # Map to skill slot if we have a mapping
                        if i in slot_mapping:
                            slot_number = slot_mapping[i]
                            skill_slots[f"slot_{slot_number}"] = skill_icon_id
                            self.logger.debug(f"Skill slot {slot_number}: {skill_icon_id}")
                        else:
                            # For additional skills beyond the standard 5, use sequential numbering
                            slot_number = str(10 + i - 5)  # Start from slot 10 for extra skills
                            skill_slots[f"slot_{slot_number}"] = skill_icon_id
                            self.logger.debug(f"Extra skill slot {slot_number}: {skill_icon_id}")
                    else:
                        self.logger.warning(f"Could not extract background-image URL from skill div {i}")

                except Exception as e:
                    self.logger.warning(f"Error processing skill div {i}: {e}")
                    continue

            if skill_slots:
                self.logger.info(f"Successfully extracted {len(skill_slots)} skill slots")
                return skill_slots
            self.logger.warning("No skill slots were extracted")
            return {}

        except Exception as e:
            self.logger.warning(f"Error extracting skill slots: {e}")
            return {}

    def get_skill_mappings_from_snowcrows(self) -> dict[str, dict[str, str]]:
        """Extract skill slot mappings from all SnowCrows build pages"""

        benchmark_urls = [
            ("https://snowcrows.com/benchmarks/?filter=dps", "dps"),
            ("https://snowcrows.com/benchmarks/?filter=quickness", "quick"),
            ("https://snowcrows.com/benchmarks/?filter=alacrity", "alac"),
        ]

        skill_mappings = {}

        for url, benchmark_type in benchmark_urls:
            try:
                self.logger.info(f"Fetching {benchmark_type.upper()} benchmarks page: {url}")

                # Use requests to get the page since we only need basic HTML parsing
                session = requests.Session()
                session.headers.update(
                    {
                        "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36",
                    },
                )

                response = session.get(url, timeout=30)
                response.raise_for_status()

                # Extract build URLs
                build_url_pattern = re.compile(
                    r'<a\s+href="(/builds/raids/[^"]+)"[^>]*class="[^"]*flex\s+items-center[^"]*"',
                    re.IGNORECASE,
                )

                build_urls = build_url_pattern.findall(response.text)
                self.logger.info(f"Found {len(build_urls)} {benchmark_type.upper()} build URLs")

                for build_url in build_urls:
                    full_url = f"https://snowcrows.com{build_url}"
                    self.logger.info(f"Fetching for: {full_url}")
                    build_name = build_url.split("/")[-1]

                    # Extract skill slots for this build
                    skill_slots = self._extract_skill_slots(full_url)
                    if skill_slots:
                        skill_mappings[f"{benchmark_type}-{build_name}"] = skill_slots
                        self.logger.info(f"Extracted {len(skill_slots)} skill slots for {build_name}")

                    # Add delay between requests
                    time.sleep(self.delay)

            except Exception as e:
                self.logger.exception(f"Error fetching {benchmark_type} page: {e}")

        self.logger.info(f"Extracted skill mappings for {len(skill_mappings)} builds")
        return skill_mappings

    def save_skill_mappings(self, skill_mappings: dict[str, dict[str, str]]) -> None:
        """Save skill mappings to JSON file, merging with existing data"""

        skill_mappings_file = self.output_dir / "skill_mappings.json"

        existing_data = {}
        if skill_mappings_file.exists():
            try:
                with skill_mappings_file.open("r", encoding="utf-8") as f:
                    existing_data = json.load(f)
                self.logger.info(f"Loaded existing skill mappings: {len(existing_data)} builds")
            except Exception as e:
                self.logger.warning(f"Could not load existing skill mappings: {e}")

        required_slots = {"slot_6", "slot_7", "slot_8", "slot_9", "slot_0"}
        complete_builds = {}
        incomplete_builds = {}

        for build_name, skills in skill_mappings.items():
            if required_slots.issubset(skills.keys()):
                complete_builds[build_name] = skills
                self.logger.debug(f"Build {build_name} has complete skill data")
            else:
                missing_slots = required_slots - skills.keys()
                incomplete_builds[build_name] = skills
                self.logger.warning(f"Build {build_name} missing slots: {missing_slots}")

        merged_data = existing_data.copy()
        merged_data.update(complete_builds)

        for build_name, keybind_dct in incomplete_builds.items():
            if build_name not in merged_data:
                merged_data[build_name] = keybind_dct
            else:
                for slot, skill_id in keybind_dct.items():
                    if (
                        slot not in merged_data[build_name]
                        or merged_data[build_name][slot] == "-1"
                    ):
                        merged_data[build_name][slot] = skill_id

        with skill_mappings_file.open("w", encoding="utf-8") as f:
            json.dump(merged_data, f, indent=2, ensure_ascii=False)

        self.logger.info(f"Saved skill mappings to {skill_mappings_file}")
        self.logger.info(f"Complete builds from current run: {len(complete_builds)}")
        self.logger.info(f"Total builds in file: {len(merged_data)}")

    def run(self) -> None:
        """Main execution method"""
        try:
            skill_mappings = self.get_skill_mappings_from_snowcrows()

            if not skill_mappings:
                self.logger.warning("No skill mappings found")
                return

            self.save_skill_mappings(skill_mappings)

        finally:
            self._cleanup_webdriver()


def main() -> None:
    """Main function with CLI interface"""
    parser = argparse.ArgumentParser(
        description="Extract build metadata and DPS report links from SnowCrows",
    )
    parser.add_argument(
        "--output",
        "-o",
        type=Path,
        default="internal_data/html",
        help="Output directory for HTML files (default: internal_data/html)",
    )
    parser.add_argument(
        "--delay",
        "-d",
        type=float,
        default=1.0,
        help="Delay between requests in seconds (default: 1.0)",
    )
    parser.add_argument(
        "--visible",
        action="store_true",
        help="Run browser in visible mode (default: headless)",
    )

    args = parser.parse_args()

    print(f"📁 Output directory: {args.output}")
    print(f"⏱️  Delay between requests: {args.delay}s")
    print(f"🌐 Browser mode: {'Visible' if args.visible else 'Headless'}")

    extractor = SnowCrowsSkillExtractor(
        output_dir=args.output,
        delay=args.delay,
        headless=not args.visible,
    )

    print("🔍 Extracting skill mappings from SnowCrows...")
    extractor.run()
    print("✅ Skill mapping extraction completed!")
    print(f"🎯 Skill mappings saved to: {extractor.output_dir}/skill_mappings.json")


if __name__ == "__main__":
    main()
