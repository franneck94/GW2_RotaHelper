# type: ignore
"""
Snow Crows metadata extractor.

This script extracts build metadata and skill information from SnowCrows pages,
including DPS report links and skill slot mappings.
"""

import argparse
import json
import logging
import re
import time
from pathlib import Path

import requests
from selenium import webdriver
from selenium.common.exceptions import TimeoutException
from selenium.webdriver.chrome.options import Options
from selenium.webdriver.common.by import By
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait


SLEEP_DELAY = 1.5


class SnowCrowsMetadataExtractor:
    """Extractor for SnowCrows build metadata and DPS report links"""

    def __init__(
        self,
        output_dir: Path,
        delay: float = 1.0,
        headless: bool = True,
    ) -> None:
        self.output_dir = output_dir
        self.delay = delay
        self.headless = headless
        self.session = requests.Session()
        self.driver = None

        self.session.headers.update(
            {
                "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36",
            },
        )

        logging.basicConfig(
            level=logging.INFO,
            format="%(levelname)s: %(message)s",
        )
        self.logger = logging.getLogger(__name__)

        self.output_dir.mkdir(parents=True, exist_ok=True)

        self.elite_spec_to_profession = {
            # Guardian
            "dragonhunter": "guardian",
            "firebrand": "guardian",
            "willbender": "guardian",
            "guardian": "guardian",
            "luminary": "guardian",
            # Warrior
            "berserker": "warrior",
            "spellbreaker": "warrior",
            "bladesworn": "warrior",
            "warrior": "warrior",
            "paragon": "warrior",
            # Engineer
            "scrapper": "engineer",
            "holosmith": "engineer",
            "mechanist": "engineer",
            "engineer": "engineer",
            "amalgam": "engineer",
            # Ranger
            "druid": "ranger",
            "soulbeast": "ranger",
            "untamed": "ranger",
            "ranger": "ranger",
            "galeshot": "ranger",
            # Thief
            "daredevil": "thief",
            "deadeye": "thief",
            "specter": "thief",
            "spectre": "thief",
            "thief": "thief",
            "antiquary": "thief",
            # Elementalist
            "tempest": "elementalist",
            "weaver": "elementalist",
            "catalyst": "elementalist",
            "elementalist": "elementalist",
            "evoker": "elementalist",
            # Mesmer
            "chronomancer": "mesmer",
            "mirage": "mesmer",
            "virtuoso": "mesmer",
            "mesmer": "mesmer",
            "troubadour": "mesmer",
            # Necromancer
            "reaper": "necromancer",
            "scourge": "necromancer",
            "harbinger": "necromancer",
            "necromancer": "necromancer",
            "ritualist": "necromancer",
            # Revenant
            "herald": "revenant",
            "renegade": "revenant",
            "vindicator": "revenant",
            "revenant": "revenant",
            "conduit ": "revenant",
        }

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

    def _sanitize_build_name(self, build_name: str) -> str:
        """Convert build name to safe filename"""
        filename = re.sub(r"[^\w\-_.]", "_", build_name.lower())
        filename = re.sub(r"_+", "_", filename)
        filename = filename.strip("_")
        return filename

    def _url_to_readable_name(self, url_name: str) -> str:
        """Convert URL format build name to readable format"""
        readable = url_name.replace("-", " ").title()

        # Handle special cases
        readable = readable.replace("Ih", "IH")  # Infinite Horizon
        readable = readable.replace("Gs", "GS")  # Greatsword
        readable = readable.replace("Lb", "LB")  # Longbow
        return readable.replace("Sb", "SB")  # Shortbow

    def _deduce_profession_from_build_name(
        self,
        build_name: str,
        url_path: str,
    ) -> tuple[str, str]:
        """Deduce profession and elite spec from build name and URL"""
        build_name_lower = build_name.lower()
        url_path_lower = url_path.lower()

        # Sort by length of elite spec name (longest first) to match most specific first
        sorted_specs = sorted(self.elite_spec_to_profession.items(), key=lambda x: len(x[0]), reverse=True)

        for elite_spec, profession in sorted_specs:
            if elite_spec in build_name_lower or elite_spec in url_path_lower.split("/")[-1]:
                return profession.title(), elite_spec.title()

        return "Unknown", ""

    def _extract_class_info(self, build_url: str) -> dict:
        """Extract class/profession information from build name and URL"""
        # Determine build type from URL
        build_type = "condition" if "condition" in build_url.lower() else "power"

        # Extract build name from URL for analysis
        url_parts = build_url.strip("/").split("/")
        if len(url_parts) >= 1:
            build_name_from_url = url_parts[-1]  # Last part is the build name
            profession, elite_spec = self._deduce_profession_from_build_name(
                build_name_from_url,
                build_url,
            )
        else:
            profession, elite_spec = "Unknown", ""

        return {
            "profession": profession,
            "elite_spec": elite_spec,
            "build_type": build_type,
        }

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
                "//div[contains(@class, 'text-right') and contains(@class, 'flex-grow') and contains(@class, 'text-2xl') and contains(text(), 'Skills')]"
            )

            if not skills_sections:
                self.logger.warning("Could not find Skills section on build page")
                return {}

            self.logger.info(f"Found {len(skills_sections)} Skills section(s)")

            # Get the parent container that has the skill icons
            skills_container = skills_sections[0].find_element(By.XPATH, "./ancestor::div[contains(@class, 'relative')]")

            # Find all skill icon divs within this container
            skill_divs = skills_container.find_elements(
                By.XPATH,
                ".//div[contains(@class, 'gw2a--Wvchy') and contains(@class, 'gw2a--s2qls') and contains(@class, 'gw2a--376lb') and contains(@class, 'gw2a--3u_DO')]"
            )

            if not skill_divs:
                self.logger.warning("Could not find skill icon divs in Skills section")
                return {}

            self.logger.info(f"Found {len(skill_divs)} skill icons")

            # Extract skill URLs and map them to slot numbers
            skill_slots = {}

            # Standard GW2 skill slot mapping for utilities:
            # Index 0 = Heal skill (slot 5)
            # Index 1-3 = Utility skills (slots 6, 7, 8)
            # Index 4 = Elite skill (slot 9)
            slot_mapping = {
                0: "5",  # Heal
                1: "6",  # Utility 1
                2: "7",  # Utility 2
                3: "8",  # Utility 3
                4: "9",  # Elite
            }

            for i, skill_div in enumerate(skill_divs):
                try:
                    # Extract background-image URL from style attribute
                    style = skill_div.get_attribute("style") or ""

                    # Parse background-image URL using regex
                    bg_match = re.search(r'background-image:\s*url\(&quot;([^&]+)&quot;\)', style)
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

    def get_snowcrows_builds_and_metadata(self) -> tuple[list[dict], list[str]]:
        """Extract build metadata and DPS report links from SnowCrows build pages"""

        benchmark_urls = [
            ("https://snowcrows.com/benchmarks/?filter=dps", "dps"),
            ("https://snowcrows.com/benchmarks/?filter=quickness", "quick"),
            ("https://snowcrows.com/benchmarks/?filter=alacrity", "alac"),
        ]
        builds_info = []
        dps_report_links = []

        for url, benchmark_type in benchmark_urls:
            try:
                self.logger.info(
                    f"Fetching SnowCrows {benchmark_type.upper()} benchmarks page: {url}",
                )
                response = self.session.get(url, timeout=30)
                response.raise_for_status()

                build_url_pattern = re.compile(
                    r'<a\s+href="(/builds/raids/[^"]+)"[^>]*class="[^"]*flex\s+items-center[^"]*"',
                    re.IGNORECASE,
                )

                build_urls: list[str] = build_url_pattern.findall(response.text)
                self.logger.info(
                    f"Found {len(build_urls)} {benchmark_type.upper()} build URLs",
                )

                for build_url in build_urls:
                    url_parts = build_url.strip("/").split("/")
                    if len(url_parts) >= 4:
                        build_name = url_parts[-1]  # Last part is the build name

                        readable_name = self._url_to_readable_name(build_name)
                        full_url = f"https://snowcrows.com{build_url}"

                        # Extract class/specialization info from build page
                        class_info = self._extract_class_info(full_url)

                        # Extract skill slot information from the build page
                        skill_slots = self._extract_skill_slots(full_url)

                        # Extract DPS report link from build page
                        dps_report_url = self._extract_dps_report_link(full_url)
                        if dps_report_url:
                            dps_report_links.append(dps_report_url)

                        build_info = {
                            "name": readable_name,
                            "url": full_url,
                            "benchmark_type": benchmark_type,
                            "profession": class_info.get("profession", "Unknown"),
                            "elite_spec": class_info.get("elite_spec", ""),
                            "build_type": class_info.get("build_type", "power"),
                            "url_name": build_name,
                            "dps_report_url": dps_report_url,
                            "skill_slots": skill_slots,
                            "overall_dps": None,  # Will be populated later if needed
                        }

                        if skill_slots:
                            self.logger.info(f"Extracted skill slots for {readable_name}: {list(skill_slots.keys())}")

                        builds_info.append(build_info)

            except Exception as e:
                self.logger.exception(
                    f"Error fetching SnowCrows {benchmark_type} page: {e}",
                )

        # Remove duplicates based on URL and benchmark type
        seen = set()
        unique_builds = []
        for build_info in builds_info:
            key = (build_info["url"], build_info["benchmark_type"])
            if key not in seen:
                seen.add(key)
                unique_builds.append(build_info)

        self.logger.info(
            f"Found {len(unique_builds)} total unique builds across all benchmark types",
        )
        self.logger.info(f"Found {len(dps_report_links)} DPS report links")

        return unique_builds, dps_report_links

    def _extract_dps_report_link(self, url: str) -> str | None:
        """Extract DPS report link from SnowCrows build page"""
        try:
            if self.driver is None:
                self._setup_webdriver()

            self.driver.get(url)
            WebDriverWait(self.driver, 10).until(
                EC.presence_of_element_located((By.TAG_NAME, "body")),
            )
            time.sleep(SLEEP_DELAY)

            try:
                # First try to find dps.report link
                dps_report_link = WebDriverWait(self.driver, 5).until(
                    EC.element_to_be_clickable(
                        (By.XPATH, "//a[contains(@href, 'dps.report')]"),
                    ),
                )
                dps_report_url = dps_report_link.get_attribute("href")
                self.logger.debug(f"Found DPS report URL: {dps_report_url}")
                return dps_report_url
            except TimeoutException:
                # If dps.report not found, try wingman proxy as backup
                try:
                    dps_report_link = WebDriverWait(self.driver, 5).until(
                        EC.element_to_be_clickable(
                            (
                                By.XPATH,
                                "//a[contains(@href, 'gw2wingman.nevermindcreations.de')]",
                            ),
                        ),
                    )
                    dps_report_url = dps_report_link.get_attribute("href")
                    self.logger.debug(f"Found wingman proxy URL: {dps_report_url}")
                    return dps_report_url
                except TimeoutException:
                    self.logger.warning(f"Could not find DPS report link on {url}")
                    return None

        except Exception as e:
            self.logger.warning(f"Error extracting DPS report link from {url}: {e}")
            return None

    def save_metadata(self, builds_info: list[dict], dps_report_links: list[str]) -> None:
        """Save build metadata and DPS report links to JSON files"""

        # Save build metadata
        metadata_file = self.output_dir / "build_metadata.json"
        with metadata_file.open("w", encoding="utf-8") as f:
            json.dump(builds_info, f, indent=2, ensure_ascii=False)

        self.logger.info(f"Saved build metadata to {metadata_file} ({len(builds_info)} builds)")

        # Save DPS report links
        dps_links_file = self.output_dir / "dps_report_links.json"
        unique_links = list(set(dps_report_links))  # Remove duplicates
        with dps_links_file.open("w", encoding="utf-8") as f:
            json.dump(unique_links, f, indent=2, ensure_ascii=False)

        self.logger.info(f"Saved DPS report links to {dps_links_file} ({len(unique_links)} unique links)")

    def run(self) -> None:
        """Main execution method"""
        try:
            builds_info, dps_report_links = self.get_snowcrows_builds_and_metadata()

            if not builds_info:
                self.logger.warning("No builds found")
                return

            self.save_metadata(builds_info, dps_report_links)

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

    extractor = SnowCrowsMetadataExtractor(
        output_dir=args.output,
        delay=args.delay,
        headless=not args.visible,
    )

    print("🔍 Extracting SnowCrows metadata...")
    extractor.run()
    print("✅ Metadata extraction completed!")
    print(f"📄 Build metadata saved to: {extractor.output_dir}/build_metadata.json")
    print(f"🔗 DPS report links saved to: {extractor.output_dir}/dps_report_links.json")


if __name__ == "__main__":
    main()
