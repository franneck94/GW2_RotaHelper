import argparse
import json
import logging
import re
import time
from pathlib import Path

import requests
from bs4 import BeautifulSoup
from selenium import webdriver
from selenium.common.exceptions import TimeoutException
from selenium.webdriver.chrome.options import Options
from selenium.webdriver.common.by import By
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait


SLEEP_DELAY = 1.5
SCRIPT_DIR = Path(__file__).resolve().parent


class SnowCrowsScraper:
    """Scraper for SnowCrows benchmark DPS report links"""

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
        self.driver: webdriver.Chrome | None = None

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

        self.output_dir.mkdir(
            parents=True,
            exist_ok=True,
        )

        self.elite_spec_to_profession = {
            "dragonhunter": "guardian",
            "firebrand": "guardian",
            "willbender": "guardian",
            "guardian": "guardian",
            "luminary": "guardian",
            "berserker": "warrior",
            "spellbreaker": "warrior",
            "bladesworn": "warrior",
            "warrior": "warrior",
            "paragon": "warrior",
            "scrapper": "engineer",
            "holosmith": "engineer",
            "mechanist": "engineer",
            "engineer": "engineer",
            "amalgam": "engineer",
            "druid": "ranger",
            "soulbeast": "ranger",
            "untamed": "ranger",
            "ranger": "ranger",
            "galeshot": "ranger",
            "daredevil": "thief",
            "deadeye": "thief",
            "specter": "thief",
            "spectre": "thief",
            "thief": "thief",
            "antiquary": "thief",
            "tempest": "elementalist",
            "weaver": "elementalist",
            "catalyst": "elementalist",
            "elementalist": "elementalist",
            "evoker": "elementalist",
            "chronomancer": "mesmer",
            "mirage": "mesmer",
            "virtuoso": "mesmer",
            "mesmer": "mesmer",
            "troubadour": "mesmer",
            "reaper": "necromancer",
            "scourge": "necromancer",
            "harbinger": "necromancer",
            "necromancer": "necromancer",
            "ritualist": "necromancer",
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
        """Convert build_info name to safe filename"""
        filename = re.sub(r"[^\w\-_.]", "_", build_name.lower())
        filename = re.sub(r"_+", "_", filename)
        filename = filename.strip("_")

        if not filename.endswith(".html"):
            filename += ".html"

        return filename

    def get_snowcrows_builds_and_links(self, only_new: bool = False) -> list[dict]:
        benchmark_urls = [
            ("https://snowcrows.com/benchmarks?b=none", "dps"),
            ("https://snowcrows.com/benchmarks?b=quickness", "quick"),
            ("https://snowcrows.com/benchmarks?b=alacrity", "alac"),
        ]
        builds_info: list[dict] = []
        if only_new:
            self.logger.info("Only processing new builds, ignoring previously processed ones")
            prev_metadata_file = self.output_dir / "build_metadata.json"
            if prev_metadata_file.exists():
                with prev_metadata_file.open(encoding="utf-8") as f:
                    builds_info = json.load(f)

        for url, benchmark_type in benchmark_urls:
            try:
                self.logger.info(
                    f"Fetching SnowCrows {benchmark_type.upper()} benchmarks page: {url}",
                )
                response = self.session.get(url, timeout=30)
                response.raise_for_status()

                text = response.text
                builds_data = self._extract_builds_from_page(text)
                self.logger.info(
                    f"Found {len(builds_data)} {benchmark_type.upper()} builds",
                )

                for build_data in builds_data:
                    build_url = build_data["url"]
                    build_name = build_data["name"]

                    url_parts = build_url.strip("/").split("/")
                    if len(url_parts) >= 4:
                        url_name: str = url_parts[-1]

                        readable_name = build_name
                        full_url = f"https://snowcrows.com{build_url}"
                        class_info = self._extract_class_info(build_name)
                        build_key = f"{readable_name}_{benchmark_type}"

                        build_subdir: Path = self.output_dir / benchmark_type / class_info.get("build_type", "Unknown")
                        build_subdir.mkdir(parents=True, exist_ok=True)
                        url_name_with_underscores = url_name.replace("-", "_")
                        filename = self._sanitize_build_name(url_name_with_underscores)
                        file_path = build_subdir / filename
                        html_file_path = str(file_path.relative_to(self.output_dir))

                        if build_key not in {f"{b['name']}_{b['benchmark_type']}" for b in builds_info}:
                            build_info = {
                                "name": readable_name,
                                "url": full_url,
                                "benchmark_type": benchmark_type,
                                "profession": class_info.get("profession", "Unknown"),
                                "elite_spec": class_info.get("elite_spec", "Unknown"),
                                "build_type": class_info.get("build_type", "Unknown"),
                                "url_name": url_name,
                                "overall_dps": None,
                                "html_file_path": html_file_path,
                            }
                            builds_info.append(build_info)
                            build_info = self.get_dps_reports_for_builds(build_info)
                            self.download_dps_report(build_info)

            except Exception as e:
                self.logger.exception(f"Error processing {benchmark_type} builds: {e}")
                return []
        return builds_info

    def _extract_builds_from_page(self, html_content: str) -> list[dict]:
        builds_data = []

        try:
            soup = BeautifulSoup(html_content, "html.parser")

            build_cards = soup.find_all(
                "a",
                class_="font-medium line-clamp-1",
            )

            self.logger.info(f"Found {len(build_cards)} build_info cards in HTML")

            for card in build_cards:
                try:
                    build_url: str = card.get("href")  # type: ignore
                    build_name = build_url[build_url.rfind("/") + 1 :].replace("-", " ").title()

                    if build_name and build_url:
                        build_data = {"name": build_name, "url": build_url}
                        builds_data.append(build_data)
                        self.logger.debug(f"Extracted build_info: {build_name} -> {build_url}")

                except Exception as e:
                    self.logger.warning(f"Error extracting build_info from card: {e}")
                    continue

            self.logger.info(f"Successfully extracted {len(builds_data)} builds")

        except Exception as e:
            self.logger.exception(f"Error parsing HTML content: {e}")

        return builds_data

    def _extract_class_info(self, buiild_name: str) -> dict:
        class_info = {"profession": "Unknown", "elite_spec": "", "build_type": "power"}

        try:
            name_parts = buiild_name.lower().split()
            for part in name_parts:
                if part in self.elite_spec_to_profession:
                    class_info["elite_spec"] = part
                    class_info["profession"] = self.elite_spec_to_profession[part]
                    break

            if "condi" in name_parts or "condition" in name_parts:
                class_info["build_type"] = "condition"
            elif "power" in name_parts:
                class_info["build_type"] = "power"
            else:
                class_info["build_type"] = "power"
        except Exception as e:
            self.logger.warning(f"Error extracting class info from build_info name '{buiild_name}': {e}")

        return class_info

    def get_dps_reports_for_builds(self, build_info: dict) -> dict:
        try:
            self._setup_webdriver()
            if not self.driver:
                self.logger.error("WebDriver not initialized, skipping DPS report extraction")
                return build_info

            try:
                self.logger.info(f"Processing build_info: {build_info['name']} ({build_info['url']})")
                self.driver.get(build_info["url"])

                WebDriverWait(self.driver, 30).until(
                    EC.presence_of_element_located((By.CLASS_NAME, "flex-col")),
                )

                time.sleep(self.delay)

                page_source = self.driver.page_source
                soup = BeautifulSoup(page_source, "html.parser")

                dps_report_link = None
                icon_elements = soup.find_all("i", class_="fa-file-chart-column")
                for icon in icon_elements:
                    parent_link = icon.find_parent("a")
                    if parent_link:
                        href = parent_link.get("href", "")  # type: ignore
                        if href and "dps.report" in href:
                            dps_report_link = href
                            break

                dps_value = None
                benchmark_divs = soup.find_all("div", string="Last Benchmark Max")
                for div in benchmark_divs:
                    parent_container = div.find_parent()
                    if parent_container:
                        dps_icon = parent_container.find("i", class_="fas fa-angle-double-up mr-1")  # type: ignore
                        if dps_icon:
                            dps_text = dps_icon.find_next_sibling(text=True)
                            if not dps_text:
                                next_elem = dps_icon.find_next_sibling()
                                if next_elem:
                                    dps_text = next_elem.get_text(strip=True)
                            if dps_text:
                                dps_text = dps_text.strip()
                                dps_value = int(dps_text.replace(",", ""))

                if dps_report_link:
                    build_info["dps_report"] = dps_report_link
                    self.logger.info(f"Found DPS report link: {dps_report_link}")
                else:
                    build_info["dps_report"] = None
                    self.logger.warning("DPS report link not found")

                if dps_value:
                    build_info["overall_dps"] = dps_value
                    self.logger.info(f"Found DPS value: {dps_value}")
                else:
                    build_info["overall_dps"] = None
                    self.logger.warning("DPS value not found")

            except TimeoutException:
                self.logger.warning(f"Timeout while loading page for build_info: {build_info['name']}")
                build_info["dps_report"] = None
                build_info["overall_dps"] = None
            except Exception as e:
                self.logger.exception(f"Error processing build_info '{build_info['name']}': {e}")
                build_info["dps_report"] = None
                build_info["overall_dps"] = None

        finally:
            self._cleanup_webdriver()

        return build_info

    def download_dps_report(self, build_info: dict) -> bool:
        try:
            build_name = build_info["name"]
            benchmark_type = build_info["benchmark_type"]
            dps_report = build_info.get("dps_report")

            if not dps_report:
                self.logger.warning(f"No DPS report URL for build_info: {build_name}")
                return False

            if self.driver is None:
                self._setup_webdriver()
            if not self.driver:
                self.logger.error("WebDriver not initialized, cannot download DPS report")
                return False

            self.logger.info(f"Navigating to DPS report: {dps_report}")
            self.driver.get(dps_report)

            WebDriverWait(self.driver, 30).until(
                lambda driver: driver.execute_script("return document.readyState") == "complete",
            )
            time.sleep(self.delay)

            self.logger.info("Looking for Player Summary tab...")
            try:
                player_summary_link = WebDriverWait(self.driver, 10).until(
                    EC.element_to_be_clickable((By.XPATH, "//a[contains(text(), 'Player Summary')]")),
                )
                player_summary_link.click()
                self.logger.info("Clicked on Player Summary")
                time.sleep(self.delay)
            except TimeoutException:
                self.logger.warning("Player Summary tab not found")
                return False

            self.logger.info("Looking for Simple Rotation tab...")
            try:
                simple_rotation_link = WebDriverWait(self.driver, 10).until(
                    EC.element_to_be_clickable((By.XPATH, "//a[contains(text(), 'Simple')]")),
                )
                simple_rotation_link.click()
                self.logger.info("Clicked on Simple Rotation")
                time.sleep(self.delay)
            except TimeoutException:
                self.logger.warning("Simple Rotation tab not found")
                return False

            WebDriverWait(self.driver, 10).until(
                lambda driver: driver.execute_script("return document.readyState") == "complete",
            )
            time.sleep(self.delay)

            html_content = self.driver.page_source

            build_subdir: Path = self.output_dir / benchmark_type / build_info["build_type"]
            build_subdir.mkdir(parents=True, exist_ok=True)
            url_name_with_underscores = build_info["url_name"].replace("-", "_")
            filename = self._sanitize_build_name(url_name_with_underscores)
            file_path: Path = build_subdir / filename

            with file_path.open("w", encoding="utf-8") as f:
                f.write(html_content)

            self.logger.info(f"Successfully downloaded DPS report HTML to: {file_path}")
            return True

        except Exception as e:
            self.logger.exception(
                f"Error downloading {build_info['name']} from {build_info.get('dps_report', 'No URL')}: {e}",
            )
            return False


def main() -> int:
    parser = argparse.ArgumentParser(description="Scrape SnowCrows benchmark builds and metadata")

    parser.add_argument(
        "--output-dir",
        type=Path,
        default=SCRIPT_DIR.parent / "internal_data" / "html",
        help="Output directory for scraped HTML files (default: ../internal_data/html)",
    )

    parser.add_argument(
        "--delay",
        type=float,
        default=1.5,
        help="Delay between requests in seconds (default: 1.5)",
    )

    parser.add_argument(
        "--no-headless",
        action="store_true",
        help="Run browser in non-headless mode (visible window)",
    )

    parser.add_argument(
        "--manual",
        action="store_true",
        help="Manual mode - use manually curated list of builds",
    )

    parser.add_argument(
        "--only_new",
        action="store_true",
        help="Only process new builds, ignoring previously processed ones",
    )

    args = parser.parse_args()

    scraper = SnowCrowsScraper(
        output_dir=args.output_dir,
        delay=args.delay,
        headless=not args.no_headless,
    )

    try:
        if args.manual:
            manual_file = SCRIPT_DIR.parent / "internal_data" / "manual_log_list.json"
            if manual_file.exists():
                with manual_file.open(encoding="utf-8") as f:
                    builds_info = json.load(f)
                scraper.logger.info(f"Loaded {len(builds_info)} builds from manual list")
            else:
                scraper.logger.error(f"Manual build_info list not found: {manual_file}")
                return 1
        else:
            builds_info = scraper.get_snowcrows_builds_and_links(args.only_new)

        if not builds_info:
            scraper.logger.error("No builds found or extracted")
            return 1

        prev_metadata_file = args.output_dir / "build_metadata.json"
        if prev_metadata_file.exists():
            with prev_metadata_file.open(encoding="utf-8") as f:
                prev_builds_info = json.load(f)
        else:
            prev_builds_info = {}

        if prev_builds_info:
            prev_builds_dict = {build_info["name"]: build_info for build_info in prev_builds_info}
            current_builds_dict = {build_info["name"]: build_info for build_info in builds_info}

            merged_builds = {}

            for name, current_build in current_builds_dict.items():
                if name in prev_builds_dict:
                    prev_build = prev_builds_dict[name]
                    merged_build = {**prev_build, **current_build}
                    merged_builds[name] = merged_build
                    scraper.logger.debug(f"Merged existing build_info: {name}")
                else:
                    merged_builds[name] = current_build
                    scraper.logger.debug(f"Added new build_info: {name}")

            for name, prev_build in prev_builds_dict.items():
                if name not in current_builds_dict:
                    merged_builds[name] = prev_build
                    scraper.logger.debug(f"Kept previous build_info: {name}")

            builds_info = list(merged_builds.values())
            scraper.logger.info(
                f"Merged {len(prev_builds_dict)} previous builds with {len(current_builds_dict)} "
                f"current builds, result: {len(builds_info)} total builds",
            )

        metadata_file = args.output_dir / "build_metadata.json"
        with metadata_file.open("w", encoding="utf-8") as f:
            json.dump(builds_info, f, indent=2, ensure_ascii=False)

        scraper.logger.info(f"Saved metadata for {len(builds_info)} builds to {metadata_file}")
        scraper.logger.info("SnowCrows scraper completed successfully")
        return 0

    except KeyboardInterrupt:
        scraper.logger.info("Scraper interrupted by user")
        return 1
    except Exception as e:
        scraper.logger.exception(f"Scraper failed with error: {e}")
        return 1
    finally:
        scraper._cleanup_webdriver()


if __name__ == "__main__":
    exit(main())
