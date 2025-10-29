#!/usr/bin/env python3
"""
Web scraper to visit all benchmark report sites listed on SnowCrows pages.

This script scrapes SnowCrows benchmarks (DPS, Quick, Alac) to find all
dps.report links, then visits each one and downloads the HTML content.
"""

import argparse
import logging
import re
import time
from pathlib import Path

import requests
from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.chrome.options import Options
from selenium.common.exceptions import TimeoutException

SLEEP_DELAY = 1.5


class SnowCrowsScraper:
    """Scraper for SnowCrows benchmark DPS report links"""

    def __init__(self, output_dir: Path, delay: float = 1.0, headless: bool = True):
        self.output_dir = output_dir
        self.delay = delay  # Delay between requests to be respectful
        self.headless = headless
        self.session = requests.Session()
        self.driver = None

        self.session.headers.update(
            {
                "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"
            }
        )

        logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")
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

    def _setup_webdriver(self):
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
            "--user-agent=Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"
        )

        try:
            self.driver = webdriver.Chrome(options=chrome_options)
            self.logger.info("Chrome WebDriver initialized successfully")
        except Exception as e:
            self.logger.error(f"Failed to initialize WebDriver: {e}")
            raise

    def _cleanup_webdriver(self):
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

        if not filename.endswith(".html"):
            filename += ".html"

        return filename

    def get_snowcrows_builds_and_links(self) -> list[dict]:
        """Extract build info including class/profession from SnowCrows build pages"""

        benchmark_urls = [
            ("https://snowcrows.com/benchmarks/?filter=dps", "dps"),
            ("https://snowcrows.com/benchmarks/?filter=quickness", "quick"),
            ("https://snowcrows.com/benchmarks/?filter=alacrity", "alac"),
        ]
        builds_info = []

        for url, benchmark_type in benchmark_urls:
            try:
                self.logger.info(
                    f"Fetching SnowCrows {benchmark_type.upper()} benchmarks page: {url}"
                )
                response = self.session.get(url, timeout=30)
                response.raise_for_status()

                build_url_pattern = re.compile(
                    r'<a\s+href="(/builds/raids/[^"]+)"[^>]*class="[^"]*flex\s+items-center[^"]*"',
                    re.IGNORECASE,
                )

                build_urls = build_url_pattern.findall(response.text)
                self.logger.info(
                    f"Found {len(build_urls)} {benchmark_type.upper()} build URLs"
                )

                for build_url in build_urls:
                    url_parts = build_url.strip("/").split("/")
                    if len(url_parts) >= 4:
                        build_name = url_parts[-1]  # Last part is the build name

                        readable_name = self._url_to_readable_name(build_name)
                        full_url = f"https://snowcrows.com{build_url}"

                        # Extract class/specialization info from build page
                        class_info = self._extract_class_info(full_url)

                        build_info = {
                            "name": readable_name,
                            "url": full_url,
                            "benchmark_type": benchmark_type,
                            "profession": class_info.get("profession", "Unknown"),
                            "elite_spec": class_info.get("elite_spec", ""),
                            "build_type": class_info.get("build_type", "power"),
                            "url_name": build_name,
                        }
                        builds_info.append(build_info)

            except Exception as e:
                self.logger.error(
                    f"Error fetching SnowCrows {benchmark_type} page: {e}"
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
            f"Found {len(unique_builds)} total unique builds across all benchmark types"
        )
        return unique_builds

    def get_manual_builds_and_links(self, manual_file_path: Path) -> list[dict]:
        """Extract build info from manual JSON file"""
        import json

        if not manual_file_path.exists():
            self.logger.error(f"Manual log list file not found: {manual_file_path}")
            return []

        try:
            with open(manual_file_path, "r", encoding="utf-8") as f:
                manual_logs: dict[str, str] = json.load(f)

            self.logger.info(
                f"Loaded {len(manual_logs)} manual log entries from {manual_file_path}"
            )

            builds_info = []
            for build_name, dps_report_url in manual_logs.items():
                # Parse build name to extract info
                build_name_lower = build_name.lower()

                # Determine build type
                build_type = "condition" if "condition" in build_name_lower else "power"

                # Determine benchmark type based on build name
                benchmark_type = "dps"  # Default to dps
                if "quick" in build_name_lower or "quickness" in build_name_lower:
                    benchmark_type = "quick"
                elif "alac" in build_name_lower or "alacrity" in build_name_lower:
                    benchmark_type = "alac"

                # Extract profession info from build name
                profession, elite_spec = self._deduce_profession_from_build_name(
                    build_name, build_name
                )

                build_info = {
                    "name": build_name.replace("_", " ").title(),
                    "url": f"manual:{build_name}",  # Mark as manual entry
                    "benchmark_type": benchmark_type,
                    "profession": profession,
                    "elite_spec": elite_spec,
                    "build_type": build_type,
                    "url_name": build_name.lower().replace(" ", "_"),
                    "dps_report_url": dps_report_url,
                }
                builds_info.append(build_info)

            self.logger.info(f"Processed {len(builds_info)} manual build entries")
            return builds_info

        except Exception as e:
            self.logger.error(
                f"Error loading manual log list from {manual_file_path}: {e}"
            )
            return []

    def _url_to_readable_name(self, url_name: str) -> str:
        """Convert URL format build name to readable format"""
        readable = url_name.replace("-", " ").title()

        # Handle special cases
        readable = readable.replace("Ih", "IH")  # Infinite Horizon
        readable = readable.replace("Gs", "GS")  # Greatsword
        readable = readable.replace("Lb", "LB")  # Longbow
        readable = readable.replace("Sb", "SB")  # Shortbow

        return readable

    def _deduce_profession_from_build_name(
        self, build_name: str, url_path: str
    ) -> tuple[str, str]:
        """Deduce profession and elite spec from build name and URL"""
        build_name_lower = build_name.lower()
        url_path_lower = url_path.lower()

        for elite_spec, profession in self.elite_spec_to_profession.items():
            if elite_spec in build_name_lower or elite_spec in url_path_lower:
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
                build_name_from_url, build_url
            )
        else:
            profession, elite_spec = "Unknown", ""

        return {
            "profession": profession,
            "elite_spec": elite_spec,
            "build_type": build_type,
        }

    def _get_cache_remainder(self, cache_url: str, cache_prefix: str) -> str:
        """Helper method to get remainder after cache prefix"""
        return cache_url[len(cache_prefix) :]

    def convert_cache_url(self, cache_url: str) -> str:
        """Convert cache URLs to proper Guild Wars 2 render URLs"""
        cache1 = "/cache/https_render.guildwars2.com_file_"
        cache2 = "/cache/https_wiki.guildwars2.com_images_"

        if not cache_url.startswith(cache1) and not cache_url.startswith(cache2):
            return cache_url

        remainder = ""
        if cache_url.startswith(cache1):
            remainder = self._get_cache_remainder(cache_url, cache1)
        elif cache_url.startswith(cache2):
            remainder = self._get_cache_remainder(cache_url, cache2)
        else:
            return cache_url

        last_underscore = remainder.rfind("_")
        if last_underscore == -1:
            return cache_url

        hash_part = remainder[:last_underscore]
        skill_id_with_ext = remainder[last_underscore + 1 :]

        dot_pos = skill_id_with_ext.rfind(".")
        skill_id = skill_id_with_ext[:dot_pos] if dot_pos != -1 else skill_id_with_ext

        return f"https://render.guildwars2.com/file/{hash_part}/{skill_id}.png"

    def _process_html_content(self, html_content: str) -> str:
        """Process HTML content to replace cache URLs with proper image URLs"""
        cache_url_pattern = re.compile(
            r'src="([^"]*(?:/cache/https_render\.guildwars2\.com_file_|/cache/https_wiki\.guildwars2\.com_images_)[^"]*)"',
            re.IGNORECASE,
        )

        def replace_cache_url(match):
            cache_url = match.group(1)
            converted_url = self.convert_cache_url(cache_url)
            return f'src="{converted_url}"'

        processed_content = cache_url_pattern.sub(replace_cache_url, html_content)

        self.logger.info(
            "Processed HTML content: replaced cache URLs with proper image URLs"
        )
        return processed_content

    def download_snowcrows_build_with_metadata(self, build_info: dict) -> bool:
        """Download HTML content and store with metadata"""
        try:
            build_name = build_info["name"]
            url = build_info["url"]
            benchmark_type = build_info["benchmark_type"]

            self.logger.info(f"Processing {build_name}: {url}")

            # Check if this is a manual entry (already has dps_report_url)
            if url.startswith("manual:") and "dps_report_url" in build_info:
                dps_report_url = build_info["dps_report_url"]
                self.logger.info(f"Using manual DPS report URL: {dps_report_url}")
            else:
                # Original SnowCrows processing
                if self.driver is None:
                    self._setup_webdriver()

                self.driver.get(url)  # type: ignore
                WebDriverWait(self.driver, 10).until(  # type: ignore
                    EC.presence_of_element_located((By.TAG_NAME, "body"))
                )
                time.sleep(SLEEP_DELAY)

                try:
                    self.logger.info("Looking for DPS report link...")
                    try:
                        # First try to find dps.report link
                        dps_report_link = WebDriverWait(self.driver, 10).until(  # type: ignore
                            EC.element_to_be_clickable(
                                (By.XPATH, "//a[contains(@href, 'dps.report')]")
                            )
                        )
                        dps_report_url = dps_report_link.get_attribute("href")
                        self.logger.info(f"Found DPS report URL: {dps_report_url}")
                    except TimeoutException:
                        # If dps.report not found, try wingman proxy as backup
                        self.logger.info(
                            "DPS report link not found, trying wingman proxy..."
                        )
                        dps_report_link = WebDriverWait(self.driver, 10).until(  # type: ignore
                            EC.element_to_be_clickable(
                                (
                                    By.XPATH,
                                    "//a[contains(@href, 'gw2wingman.nevermindcreations.de')]",
                                )
                            )
                        )
                        dps_report_url = dps_report_link.get_attribute("href")
                        self.logger.info(f"Found wingman proxy URL: {dps_report_url}")

                    # Add DPS report URL to build info
                    build_info["dps_report_url"] = dps_report_url

                except TimeoutException:
                    self.logger.warning(
                        f"Could not find DPS report link on {url}, skipping..."
                    )
                    return False

            # Now process the DPS report URL
            if self.driver is None:
                self._setup_webdriver()

            self.driver.get(dps_report_url)  # type: ignore
            WebDriverWait(self.driver, 10).until(  # type: ignore
                EC.presence_of_element_located((By.TAG_NAME, "body"))
            )

            try:
                self.logger.info("Looking for Player Summary tab...")
                try:
                    # First try to find and click Player Summary tab
                    player_summary_link = WebDriverWait(self.driver, 5).until(  # type: ignore
                        EC.element_to_be_clickable(
                            (
                                By.XPATH,
                                "//a[contains(text(), 'Player Summary')]",
                            )
                        )
                    )
                    player_summary_link.click()
                    self.logger.info("Clicked Player Summary tab")
                except TimeoutException:
                    # If we can't click it, try switching to iframe (wingman proxy case)
                    try:
                        self.logger.info(
                            "Trying to switch to iframe for embedded dps.report content..."
                        )
                        iframe = self.driver.find_element(By.NAME, "mainContent")  # type: ignore
                        self.driver.switch_to.frame(iframe)  # type: ignore
                        self._in_iframe = True

                        # Now try to find Player Summary tab in the iframe
                        player_summary_link = WebDriverWait(self.driver, 5).until(  # type: ignore
                            EC.element_to_be_clickable(
                                (
                                    By.XPATH,
                                    "//a[contains(text(), 'Player Summary')]",
                                )
                            )
                        )
                        player_summary_link.click()
                        self.logger.info("Clicked Player Summary tab in iframe")
                    except TimeoutException:
                        # If still can't find it, just log and continue
                        player_summary_elements = self.driver.find_elements(  # type: ignore
                            By.XPATH, "//a[contains(text(), 'Player Summary')]"
                        )
                        if player_summary_elements:
                            self.logger.info(
                                "Found Player Summary tab (may already be active)"
                            )
                        else:
                            self.logger.warning(
                                "Could not find Player Summary tab in iframe either"
                            )
                    except Exception as e:
                        self.logger.warning(f"Could not switch to iframe: {e}")
                        # Switch back to default content if iframe switch failed
                        self.driver.switch_to.default_content()  # type: ignore

                time.sleep(SLEEP_DELAY)

                self.logger.info("Looking for Simple Rotation tab...")
                try:
                    # First try to find and click Simple Rotation tab (if not active)
                    rotation_link = WebDriverWait(self.driver, 5).until(  # type: ignore
                        EC.element_to_be_clickable(
                            (
                                By.XPATH,
                                "//a[@class='nav-link' and contains(., 'Simple') and contains(., 'Rotation')]",
                            )
                        )
                    )
                    rotation_link.click()
                    self.logger.info("Clicked Simple Rotation tab")
                except TimeoutException:
                    # Try to find Simple Rotation tab in iframe if not found in main page
                    try:
                        # If we're not already in iframe, switch to it
                        if not hasattr(self, "_in_iframe") or not self._in_iframe:
                            self.logger.info(
                                "Trying to switch to iframe for Simple Rotation tab..."
                            )
                            iframe = self.driver.find_element(By.NAME, "mainContent")  # type: ignore
                            self.driver.switch_to.frame(iframe)  # type: ignore
                            self._in_iframe = True

                        rotation_link = WebDriverWait(self.driver, 5).until(  # type: ignore
                            EC.element_to_be_clickable(
                                (
                                    By.XPATH,
                                    "//a[contains(., 'Simple') and contains(., 'Rotation')]",
                                )
                            )
                        )
                        rotation_link.click()
                        self.logger.info("Clicked Simple Rotation tab in iframe")
                    except TimeoutException:
                        # Check if Simple Rotation tab is already active in iframe
                        active_rotation = self.driver.find_elements(  # type: ignore
                            By.XPATH,
                            "//a[contains(@class, 'active') and contains(., 'Simple') and contains(., 'Rotation')]",
                        )
                        if active_rotation:
                            self.logger.info(
                                "Simple Rotation tab is already active in iframe"
                            )
                        else:
                            self.logger.warning(
                                "Could not find Simple Rotation tab in iframe either"
                            )
                    except Exception as e:
                        self.logger.warning(
                            f"Could not access Simple Rotation tab in iframe: {e}"
                        )

                time.sleep(SLEEP_DELAY)

            except TimeoutException:
                self.logger.warning(
                    f"Could not find navigation elements for {build_name}, saving current page content"
                )

            html_content = self.driver.page_source  # type: ignore
            html_content = self._process_html_content(html_content)

            # Create appropriate subdirectory structure
            build_subdir = self.output_dir / benchmark_type / build_info["build_type"]
            build_subdir.mkdir(parents=True, exist_ok=True)

            # Generate filename from the build info, converting hyphens to underscores
            url_name_with_underscores = build_info["url_name"].replace("-", "_")
            filename = self._sanitize_build_name(url_name_with_underscores)
            file_path = build_subdir / filename

            # Save HTML content
            with open(file_path, "w", encoding="utf-8") as f:
                f.write(html_content)

            # Add file path to build info
            build_info["html_file_path"] = str(file_path.relative_to(self.output_dir))

            self.logger.info(
                f"Saved: {build_info['html_file_path']} ({build_info['profession']} - {build_info['elite_spec']})"
            )
            return True

        except Exception as e:
            self.logger.error(
                f"Error downloading {build_info['name']} from {build_info['url']}: {e}"
            )
            return False

    def download_all_reports(self, builds_info: list[dict]) -> None:
        """Download all benchmark reports (DPS, Quick, Alac) with rate limiting and save metadata"""
        import json

        success_count = 0
        total_count = len(builds_info)
        successful_builds = []

        for i, build_info in enumerate(builds_info, 1):
            build_name = build_info["name"]
            benchmark_type = build_info["benchmark_type"]

            self.logger.info(
                f"Processing {i}/{total_count}: {build_name} ({benchmark_type.upper()}) - {build_info['profession']}"
            )

            if self.download_snowcrows_build_with_metadata(build_info):
                success_count += 1
                successful_builds.append(build_info)

            if i < total_count:  # Don't delay after the last request
                time.sleep(self.delay)

        # Save build metadata to JSON file
        metadata_file = self.output_dir / "build_metadata.json"
        with open(metadata_file, "w", encoding="utf-8") as f:
            json.dump(successful_builds, f, indent=2, ensure_ascii=False)

        self.logger.info(f"Saved build metadata to {metadata_file}")
        self.logger.info(f"Download complete: {success_count}/{total_count} successful")

    def run(self, manual_file_path: Path | None = None) -> None:
        """Main execution method"""
        try:
            if manual_file_path:
                self.logger.info(f"Using manual log list from: {manual_file_path}")
                builds_and_links = self.get_manual_builds_and_links(manual_file_path)
            else:
                builds_and_links = self.get_snowcrows_builds_and_links()

            if not builds_and_links:
                self.logger.warning("No builds with DPS report links found")
                return

            self.download_all_reports(builds_and_links)

        finally:
            self._cleanup_webdriver()


def main():
    """Main function with CLI interface"""
    parser = argparse.ArgumentParser(
        description="Download benchmark report HTML files from SnowCrows (DPS, Quick, Alac) or manual list"
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
        "--list-only",
        "-l",
        action="store_true",
        help="Only list the benchmark report URLs without downloading",
    )
    parser.add_argument(
        "--visible",
        action="store_true",
        help="Run browser in visible mode (default: headless)",
    )
    parser.add_argument(
        "--manual",
        "-m",
        type=Path,
        nargs="?",
        const=Path("internal_data/manual_log_list.json"),
        help="Use manual log list JSON file instead of scraping SnowCrows (default: internal_data/manual_log_list.json)",
    )

    args = parser.parse_args()

    print(f"📁 Output directory: {args.output}")
    print(f"⏱️  Delay between requests: {args.delay}s")
    print(f"🌐 Browser mode: {'Visible' if args.visible else 'Headless'}")
    if args.manual:
        print(f"📋 Using manual log list: {args.manual}")

    scraper = SnowCrowsScraper(
        output_dir=args.output, delay=args.delay, headless=not args.visible
    )

    if args.list_only:
        # Just list the builds and URLs
        if args.manual:
            print(f"🔍 Loading builds from manual log list: {args.manual}...")
            builds_and_links = scraper.get_manual_builds_and_links(args.manual)
        else:
            print("🔍 Finding builds with benchmark report links (DPS, Quick, Alac)...")
            builds_and_links = scraper.get_snowcrows_builds_and_links()

        if builds_and_links:
            print(
                f"\n📋 Found {len(builds_and_links)} builds with benchmark report links:"
            )
            for i, build_info in enumerate(builds_and_links, 1):
                filename = scraper._sanitize_build_name(build_info["url_name"])
                print(
                    f"{i:2d}. {build_info['name']} ({build_info['benchmark_type'].upper()}) -> {filename}"
                )
                print(
                    f"     Profession: {build_info['profession']} ({build_info['elite_spec']})"
                )
                print(f"     Type: {build_info['build_type']}")
                if args.manual:
                    print(f"     DPS Report: {build_info['dps_report_url']}")
                else:
                    print(f"     URL: {build_info['url']}")
        else:
            print("❌ No builds with benchmark report links found")
    else:
        # Run the full scraper
        if args.manual:
            print(f"🌐 Starting manual log list processing from: {args.manual}...")
        else:
            print("🌐 Starting SnowCrows benchmark scraper (DPS, Quick, Alac)...")
        scraper.run(args.manual)
        print("✅ Scraping completed!")
        print(f"📄 Build metadata saved to: {scraper.output_dir}/build_metadata.json")


if __name__ == "__main__":
    main()
