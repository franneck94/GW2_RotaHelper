# type: ignore
"""
Web scraper to visit all benchmark report sites listed on SnowCrows pages.

This script scrapes SnowCrows benchmarks (DPS, Quick, Alac) to find all
dps.report links, then visits each one and downloads the HTML content.
"""

import argparse
import json
import logging
import re
import time
from datetime import datetime
from pathlib import Path

import requests
from selenium import webdriver
from selenium.common.exceptions import TimeoutException
from selenium.webdriver.chrome.options import Options
from selenium.webdriver.common.by import By
from selenium.webdriver.remote.webelement import WebElement
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait


SLEEP_DELAY = 1.5


class SnowCrowsScraper:
    """Scraper for SnowCrows benchmark DPS report links"""

    def __init__(
        self,
        output_dir: Path,
        delay: float = 1.0,
        headless: bool = True,
    ) -> None:
        self.output_dir = output_dir
        self.delay = delay  # Delay between requests to be respectful
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

        self.output_dir.mkdir(
            parents=True,
            exist_ok=True,
        )

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

        metadata_file = self.output_dir / "build_metadata.json"
        if not metadata_file.exists():
            self.logger.warning(f"Build metadata file not found: {metadata_file}")
            builds_info = []
        else:
            with metadata_file.open(encoding="utf-8") as f:
                builds_info = json.load(f)

        for url, benchmark_type in benchmark_urls:
            try:
                self.logger.info(
                    f"Fetching SnowCrows {benchmark_type.upper()} benchmarks page: {url}",
                )
                response = self.session.get(url, timeout=30)
                response.raise_for_status()

                # New approach: Extract build cards with name and weapon information
                builds_data = self._extract_builds_from_page(response.text)
                self.logger.info(
                    f"Found {len(builds_data)} {benchmark_type.upper()} builds",
                )

                for build_data in builds_data:
                    build_url = build_data["url"]
                    build_name = build_data["name"]
                    weapons = build_data["weapons"]

                    url_parts = build_url.strip("/").split("/")
                    if len(url_parts) >= 4:
                        url_name = url_parts[-1]  # Last part is the build name

                        # Combine build name with weapons for readable name
                        if weapons:
                            readable_name = f"{build_name} {weapons}"
                        else:
                            readable_name = build_name

                        full_url = f"https://snowcrows.com{build_url}"

                        # Extract class/specialization info from build page
                        class_info = self._extract_class_info(full_url)

                        # Create a unique key to avoid duplicates
                        build_key = f"{readable_name}_{benchmark_type}"
                        if build_key not in {f"{b['name']}_{b['benchmark_type']}" for b in builds_info}:
                            build_info = {
                                "name": readable_name,
                                "url": full_url,
                                "benchmark_type": benchmark_type,
                                "profession": class_info.get("profession", "Unknown"),
                                "elite_spec": class_info.get("elite_spec", ""),
                                "build_type": class_info.get("build_type", "power"),
                                "url_name": url_name,
                                "weapons": weapons,  # Store weapons separately
                                "overall_dps": None,  # Will be populated during download
                            }
                            builds_info.append(build_info)
            except Exception as e:
                self.logger.exception(f"Error processing {benchmark_type} builds: {e}")
                return []
        return builds_info

    def get_manual_builds_and_links(
        self,
        manual_file_path: Path,
    ) -> list[dict]:
        """Extract build info from manual JSON file"""
        if not manual_file_path.exists():
            self.logger.error(f"Manual log list file not found: {manual_file_path}")
            return []

        try:
            with manual_file_path.open(encoding="utf-8") as f:
                manual_logs: dict[str, dict[str, str]] = json.load(f)

            self.logger.info(
                f"Loaded {len(manual_logs)} manual log entries from {manual_file_path}",
            )

            builds_info = []
            for build_name, build_data in manual_logs.items():
                # Handle both old format (string) and new format (dict with Dps.Report and ScLink)
                if isinstance(build_data, str):
                    # Old format: build_data is directly the DPS report URL
                    dps_report_url = build_data
                    sc_link_url = None
                else:
                    # New format: build_data is a dict with "Dps.Report" and "ScLink"
                    dps_report_url = build_data.get("Dps.Report", "")
                    sc_link_url = build_data.get("ScLink", "")

                if not dps_report_url:
                    self.logger.warning(f"Skipping {build_name}: no DPS report URL found")
                    continue

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
                    build_name,
                    build_name,
                )

                build_info = {
                    "name": build_name.replace("_", " ").title(),
                    "url": sc_link_url or f"manual:{build_name}",  # Use SC link if available
                    "benchmark_type": benchmark_type,
                    "profession": profession,
                    "elite_spec": elite_spec,
                    "build_type": build_type,
                    "url_name": build_name.lower().replace(" ", "_"),
                    "weapons": "",  # No weapon info available for manual entries
                    "dps_report_url": dps_report_url,
                    "sc_link_url": sc_link_url,  # Store SC link separately for metadata
                    "overall_dps": None,  # Will be populated during download
                }
                builds_info.append(build_info)

            self.logger.info(f"Processed {len(builds_info)} manual build entries")
            return builds_info

        except Exception as e:
            self.logger.exception(
                f"Error loading manual log list from {manual_file_path}: {e}",
            )
            return []

    def _url_to_readable_name(self, url_name: str) -> str:
        """Convert URL format build name to readable format"""
        readable = url_name.replace("-", " ").title()

        # Handle special cases
        readable = readable.replace("Ih", "IH")  # Infinite Horizon
        readable = readable.replace("Gs", "GS")  # Greatsword
        readable = readable.replace("Lb", "LB")  # Longbow
        return readable.replace("Sb", "SB")  # Shortbow

    def _extract_builds_from_page(self, html_content: str) -> list[dict]:
        """Extract build information from SnowCrows page with new layout"""
        builds = []

        try:
            # Pattern to match the build card structure with both name and weapons
            # This pattern captures build cards that contain both a link and weapon information
            build_card_pattern = re.compile(
                r'<div class="md:flex-grow">\s*'
                r'<div class="uppercase text-xs font-bold[^"]*">[^<]*</div>\s*'
                r'<a href="(/builds/raids/[^"]+)"[^>]*>([^<]+)</a>\s*'
                r"(?:[^<]*<[^>]*>[^<]*)*?"  # Skip any intermediate elements
                r'<div class="text-xs">([^<]+)</div>',
                re.IGNORECASE | re.DOTALL,
            )

            matches = build_card_pattern.findall(html_content)

            for match in matches:
                build_url, build_name, weapons = match

                # Clean up the extracted data
                build_name = build_name.strip()
                weapons = weapons.strip()

                # Clean up HTML entities in weapons
                weapons = weapons.replace("&amp;", "&")

                builds.append({"url": build_url, "name": build_name, "weapons": weapons})

            self.logger.info(f"Extracted {len(builds)} builds using new layout pattern")

            # Fallback: if the new pattern doesn't work, try the old approach
            if not builds:
                self.logger.warning("New pattern failed, trying fallback approach")
                builds = self._extract_builds_fallback(html_content)

        except Exception as e:
            self.logger.exception(f"Error extracting builds from page: {e}")
            # Try fallback approach
            builds = self._extract_builds_fallback(html_content)

        return builds

    def _extract_builds_fallback(self, html_content: str) -> list[dict]:
        """Fallback method for extracting builds using the old approach"""
        builds = []

        try:
            # Old pattern for build URLs
            build_url_pattern = re.compile(
                r'<a\s+href="(/builds/raids/[^"]+)"',
                re.IGNORECASE,
            )

            build_urls = build_url_pattern.findall(html_content)
            self.logger.info(f"Fallback: Found {len(build_urls)} build URLs")

            for build_url in build_urls:
                url_parts = build_url.strip("/").split("/")
                if len(url_parts) >= 4:
                    url_name = url_parts[-1]
                    readable_name = self._url_to_readable_name(url_name)

                    builds.append(
                        {
                            "url": build_url,
                            "name": readable_name,
                            "weapons": "",  # No weapon info available in fallback
                        }
                    )

        except Exception as e:
            self.logger.exception(f"Error in fallback extraction: {e}")

        return builds

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

        def replace_cache_url(match: re.Match) -> str:
            cache_url = match.group(1)
            converted_url = self.convert_cache_url(cache_url)
            return f'src="{converted_url}"'

        processed_content = cache_url_pattern.sub(replace_cache_url, html_content)

        self.logger.info(
            "Processed HTML content: replaced cache URLs with proper image URLs",
        )
        return processed_content

    def _extract_overall_dps(self, html_content: str) -> float | None:
        """Extract overall DPS value from HTML content"""
        try:
            # Debug: Log a snippet of the HTML to see the structure
            if 'class="active"' in html_content:
                active_row_start = html_content.find('class="active"')
                if active_row_start != -1:
                    snippet = html_content[active_row_start : active_row_start + 800]
                    self.logger.debug(f"Found active row snippet: {snippet[:400]}...")
            elif "data-original-title" in html_content and "damage" in html_content:
                # Find first damage tooltip for debugging
                damage_start = html_content.find("data-original-title")
                if damage_start != -1:
                    snippet = html_content[damage_start : damage_start + 300]
                    self.logger.debug(f"Found damage tooltip snippet: {snippet}")

            # Pattern to match td elements with data-original-title containing damage info
            # Example: <td data-original-title="4023510 damage&lt;br&gt;100% of total&lt;br&gt;100% of group" class="sorted">62855</td>
            # More flexible patterns to catch various formats
            dps_patterns = [
                # Pattern 1: Handle exact structure from user's HTML with class="sorted"
                r'<td[^>]*data-original-title="([^"]*damage[^"]*)"[^>]*class="sorted"[^>]*>\s*(\d{1,6})\s*</td>',
                # Pattern 2: Handle reversed attribute order (class before data-original-title)
                r'<td[^>]*class="sorted"[^>]*data-original-title="([^"]*damage[^"]*)"[^>]*>\s*(\d{1,6})\s*</td>',
                # Pattern 3: More liberal - any td with damage, handle multiline whitespace
                r'<td[^>]*data-original-title="([^"]*damage[^"]*)"[^>]*>\s*(\d{1,6})\s*</td>',
                # Pattern 4: Ultra flexible for cases with newlines between > and number
                r'<td[^>]*data-original-title="([^"]*damage[^"]*)"[^>]*>.*?(\d{1,6}).*?</td>',
            ]

            for pattern in dps_patterns:
                matches = re.findall(pattern, html_content, re.IGNORECASE | re.DOTALL)

                if matches:
                    dps_values = []
                    for tooltip, dps_text in matches:
                        try:
                            dps_val = float(dps_text)
                            # Filter reasonable DPS values (between 1000 and 100000)
                            if 1000 <= dps_val <= 100000:
                                # Additional validation: check if tooltip contains "100% of total"
                                # This indicates it's the main player's total DPS
                                if "100% of total" in tooltip:
                                    self.logger.info(f"Found total DPS: {dps_val} (tooltip: {tooltip[:50]}...)")
                                    return dps_val
                                dps_values.append(dps_val)
                        except ValueError:
                            continue

                    # If no "100% of total" found, return the highest DPS value
                    if dps_values:
                        overall_dps = max(dps_values)
                        self.logger.info(f"Extracted overall DPS: {overall_dps}")
                        return overall_dps

            # Fallback: try to extract from WebDriver if available
            if hasattr(self, "driver") and self.driver:
                return self._extract_dps_from_webdriver()

            self.logger.warning("Could not extract overall DPS from HTML content")
            return None

        except Exception as e:
            self.logger.warning(f"Error extracting overall DPS: {e}")
            return None

    def _extract_dps_from_webdriver(self) -> float | None:
        """Extract DPS value using WebDriver to find specific elements"""
        try:
            # Look for td elements with data-original-title containing "damage" and "100% of total"
            # Target the specific structure from the user's HTML
            dps_selectors = [
                # Primary: match the exact structure with "sorted" class and "100% of total"
                "//tr[@class='active']//td[@class='sorted' and contains(@data-original-title, 'damage') and contains(@data-original-title, '100% of total')]",
                # Fallback 1: any td in active row with damage and "100% of total"
                "//tr[@class='active']//td[contains(@data-original-title, 'damage') and contains(@data-original-title, '100% of total')]",
                # Fallback 2: any td with "sorted" class and damage
                "//td[@class='sorted' and contains(@data-original-title, 'damage')]",
                # Fallback 3: first td with damage in any row
                "//td[contains(@data-original-title, 'damage')]",
                # Fallback 4: most liberal - any td with damage
                "//td[contains(@data-original-title, 'damage')]",
            ]

            for selector in dps_selectors:
                try:
                    elements = self.driver.find_elements(By.XPATH, selector)  # type: ignore
                    self.logger.debug(f"Found {len(elements)} elements with selector: {selector}")

                    for i, element in enumerate(elements):
                        text = element.text.strip()
                        tooltip = element.get_attribute("data-original-title") or ""

                        self.logger.debug(f"Element {i + 1}: text='{text}', tooltip='{tooltip[:100]}...'")

                        # Extract numeric value from text
                        dps_match = re.search(r"(\d{1,6})", text)
                        if dps_match:
                            dps_val = float(dps_match.group(1))
                            if 1000 <= dps_val <= 10000000:
                                # Prefer elements with "100% of total" in tooltip
                                if "100% of total" in tooltip:
                                    self.logger.info(f"Extracted total DPS from WebDriver: {dps_val}")
                                    return dps_val
                                # If this is the first element and no "100% of total" requirement met yet
                                if i == 0:  # First element is often the main player DPS
                                    self.logger.info(f"Extracted first element DPS from WebDriver: {dps_val}")
                                    return dps_val
                                self.logger.info(f"Extracted DPS from WebDriver: {dps_val}")
                                return dps_val
                except Exception as e:
                    self.logger.debug(f"Error parsing element for DPS: {e}")
                    continue

            return None

        except Exception as e:
            self.logger.warning(f"Error extracting DPS from WebDriver: {e}")
            return None

    def _select_correct_player(self, build_info: dict[str, str]) -> bool:
        """Select the correct player in the DPS report that matches the build"""
        try:
            elite_spec = build_info.get("elite_spec", "").lower()
            profession = build_info.get("profession", "").lower()

            # Build possible class names to look for
            possible_classes = []
            if elite_spec and elite_spec != "unknown":
                possible_classes.append(elite_spec)
            if profession and profession != "unknown":
                possible_classes.append(profession)

            if not possible_classes:
                self.logger.warning("No class information available to select correct player")
                return False

            self.logger.info(f"Looking for player with classes: {possible_classes}")

            # Try each possible class name
            for class_name in possible_classes:
                if self._try_select_player_by_class(class_name):
                    return True

            self.logger.warning(f"Could not find a suitable player with classes {possible_classes}")
            return False

        except Exception as e:
            self.logger.warning(f"Error in player selection: {e}")
            return False

    def _try_select_player_by_class(self, class_name: str) -> bool:
        """Try to select a player by their class name"""
        try:
            # Find player cells with the matching class image/tooltip
            player_cells = self.driver.find_elements(  # type: ignore
                By.XPATH,
                f"//div[contains(@class, 'player-cell')]//img[contains(@alt, '{class_name.title()}') or contains(@data-original-title, '{class_name.title()}')]",
            )

            if not player_cells:
                return False

            # Try each matching player
            return any(self._try_select_player_element(img_element, class_name) for img_element in player_cells)

        except Exception as e:
            self.logger.debug(f"Error selecting player for class {class_name}: {e}")
            return False

    def _try_select_player_element(self, img_element: WebElement, class_name: str) -> bool:
        """Try to select a specific player element and verify they have valid DPS"""
        try:
            player_cell = img_element.find_element(By.XPATH, "./ancestor::div[contains(@class, 'player-cell')]")

            # Get player name
            player_name_element = player_cell.find_element(By.XPATH, ".//span[contains(@class, 'player-cell-shorten')]")
            player_name = player_name_element.text.strip()

            # Click on this player to make them active
            self.driver.execute_script("arguments[0].click();", player_cell)  # type: ignore
            self.logger.info(f"Selected player: {player_name} ({class_name.title()})")

            # Wait a moment for the selection to take effect
            time.sleep(1)

            # Verify that this player has non-zero DPS
            html_content = self.driver.page_source  # type: ignore
            test_dps = self._extract_overall_dps(html_content)

            if test_dps and test_dps > 10_000:  # Valid DPS found
                self.logger.info(f"Found valid DPS ({test_dps}) for {player_name}")
                return True

            self.logger.debug(f"Player {player_name} has low/zero DPS ({test_dps}), trying next...")
            return False

        except Exception as e:
            self.logger.debug(f"Error selecting player element: {e}")
            return False

    def download_snowcrows_build_with_metadata(self, build_info: dict) -> bool:
        """Download HTML content and store with metadata"""
        try:
            build_name = build_info["name"]
            url = build_info["url"]
            benchmark_type = build_info["benchmark_type"]

            self.logger.info(f"Processing {build_name}: {url}")

            # Check if this is a manual entry or if we already have the dps_report_url
            if build_info.get("dps_report_url"):
                dps_report_url = build_info["dps_report_url"]
                self.logger.info(f"Using existing DPS report URL: {dps_report_url}")
            else:
                # Original SnowCrows processing - need to scrape the build page for DPS report link
                if self.driver is None:
                    self._setup_webdriver()

                self.driver.get(url)  # type: ignore
                WebDriverWait(self.driver, 10).until(  # type: ignore
                    EC.presence_of_element_located((By.TAG_NAME, "body")),
                )
                time.sleep(SLEEP_DELAY)

                try:
                    self.logger.info("Looking for DPS report link...")
                    try:
                        # First try to find dps.report link
                        dps_report_link = WebDriverWait(self.driver, 10).until(  # type: ignore
                            EC.element_to_be_clickable(
                                (By.XPATH, "//a[contains(@href, 'dps.report')]"),
                            ),
                        )
                        dps_report_url = dps_report_link.get_attribute("href")
                        self.logger.info(f"Found DPS report URL: {dps_report_url}")
                    except TimeoutException:
                        # If dps.report not found, try wingman proxy as backup
                        self.logger.info(
                            "DPS report link not found, trying wingman proxy...",
                        )
                        dps_report_link = WebDriverWait(self.driver, 10).until(  # type: ignore
                            EC.element_to_be_clickable(
                                (
                                    By.XPATH,
                                    "//a[contains(@href, 'gw2wingman.nevermindcreations.de')]",
                                ),
                            ),
                        )
                        dps_report_url = dps_report_link.get_attribute("href")
                        self.logger.info(f"Found wingman proxy URL: {dps_report_url}")

                    # Add DPS report URL to build info
                    build_info["dps_report_url"] = dps_report_url

                except TimeoutException:
                    self.logger.warning(
                        f"Could not find DPS report link on {url}, skipping...",
                    )
                    return False

            # Now process the DPS report URL
            if self.driver is None:
                self._setup_webdriver()

            self.logger.info(f"Loading DPS report page: {dps_report_url}")
            self.driver.get(dps_report_url)  # type: ignore
            WebDriverWait(self.driver, 10).until(  # type: ignore
                EC.presence_of_element_located((By.TAG_NAME, "body")),
            )

            # Wait a bit for the page to fully load
            time.sleep(SLEEP_DELAY)

            # Try to select the correct player that matches our build
            self.logger.info("Attempting to select correct player...")
            player_selected = self._select_correct_player(build_info)
            if player_selected:
                self.logger.info("Successfully selected matching player")
            else:
                self.logger.warning("Could not select matching player, using default active player")

            # Extract overall DPS from the initial page load (before tab navigation)
            initial_html_content = self.driver.page_source  # type: ignore
            overall_dps = self._extract_overall_dps(initial_html_content)
            if overall_dps is not None:
                build_info["overall_dps"] = overall_dps
                self.logger.info(f"Found overall DPS: {overall_dps}")
            else:
                self.logger.warning("Could not extract overall DPS value")

            try:
                self.logger.info("Looking for Player Summary tab...")
                try:
                    # First try to find and click Player Summary tab
                    player_summary_link = WebDriverWait(self.driver, 5).until(  # type: ignore
                        EC.element_to_be_clickable(
                            (
                                By.XPATH,
                                "//a[contains(text(), 'Player Summary')]",
                            ),
                        ),
                    )
                    player_summary_link.click()
                    self.logger.info("Clicked Player Summary tab")
                except TimeoutException:
                    # If we can't click it, try switching to iframe (wingman proxy case)
                    try:
                        self.logger.info(
                            "Trying to switch to iframe for embedded dps.report content...",
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
                                ),
                            ),
                        )
                        player_summary_link.click()
                        self.logger.info("Clicked Player Summary tab in iframe")
                    except TimeoutException:
                        # If still can't find it, just log and continue
                        player_summary_elements = self.driver.find_elements(  # type: ignore
                            By.XPATH,
                            "//a[contains(text(), 'Player Summary')]",
                        )
                        if player_summary_elements:
                            self.logger.info(
                                "Found Player Summary tab (may already be active)",
                            )
                        else:
                            self.logger.warning(
                                "Could not find Player Summary tab in iframe either",
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
                            ),
                        ),
                    )
                    rotation_link.click()
                    self.logger.info("Clicked Simple Rotation tab")
                except TimeoutException:
                    # Try to find Simple Rotation tab in iframe if not found in main page
                    try:
                        # If we're not already in iframe, switch to it
                        if not hasattr(self, "_in_iframe") or not self._in_iframe:
                            self.logger.info(
                                "Trying to switch to iframe for Simple Rotation tab...",
                            )
                            iframe = self.driver.find_element(By.NAME, "mainContent")  # type: ignore
                            self.driver.switch_to.frame(iframe)  # type: ignore
                            self._in_iframe = True

                        rotation_link = WebDriverWait(self.driver, 5).until(  # type: ignore
                            EC.element_to_be_clickable(
                                (
                                    By.XPATH,
                                    "//a[contains(., 'Simple') and contains(., 'Rotation')]",
                                ),
                            ),
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
                                "Simple Rotation tab is already active in iframe",
                            )
                        else:
                            self.logger.warning(
                                "Could not find Simple Rotation tab in iframe either",
                            )
                    except Exception as e:
                        self.logger.warning(
                            f"Could not access Simple Rotation tab in iframe: {e}",
                        )

                time.sleep(SLEEP_DELAY)

            except TimeoutException:
                self.logger.warning(
                    f"Could not find navigation elements for {build_name}, saving current page content",
                )

            html_content = self.driver.page_source  # type: ignore

            # Extract overall DPS before processing HTML content
            overall_dps = self._extract_overall_dps(html_content)
            if overall_dps is not None:
                build_info["overall_dps"] = overall_dps
                self.logger.info(f"Found overall DPS: {overall_dps}")
            else:
                self.logger.warning("Could not extract overall DPS value")

            html_content = self._process_html_content(html_content)

            # Create appropriate subdirectory structure
            build_subdir = self.output_dir / benchmark_type / build_info["build_type"]
            build_subdir.mkdir(parents=True, exist_ok=True)

            # Generate filename from the build info, converting hyphens to underscores
            url_name_with_underscores = build_info["url_name"].replace("-", "_")
            filename = self._sanitize_build_name(url_name_with_underscores)
            file_path = build_subdir / filename

            # Save HTML content
            with file_path.open("w", encoding="utf-8") as f:
                f.write(html_content)

            # Add file path to build info
            build_info["html_file_path"] = str(file_path.relative_to(self.output_dir))

            dps_info = f" - DPS: {build_info.get('overall_dps', 'N/A')}" if build_info.get("overall_dps") else ""
            self.logger.info(
                f"Saved: {build_info['html_file_path']} ({build_info['profession']} - {build_info['elite_spec']}){dps_info}",
            )
            return True

        except Exception as e:
            self.logger.exception(
                f"Error downloading {build_info['name']} from {build_info['url']}: {e}",
            )
            return False

    def download_all_reports(self, builds_info: list[dict]) -> None:
        """Download all benchmark reports (DPS, Quick, Alac) with rate limiting and save metadata"""
        success_count = 0
        total_count = len(builds_info)
        successful_builds = []

        for i, build_info in enumerate(builds_info, 1):
            build_name = build_info["name"]
            benchmark_type = build_info["benchmark_type"]

            self.logger.info(
                f"Processing {i}/{total_count}: {build_name} ({benchmark_type.upper()}) - {build_info['profession']}",
            )

            if self.download_snowcrows_build_with_metadata(build_info):
                success_count += 1
                successful_builds.append(build_info)

            if i < total_count:  # Don't delay after the last request
                time.sleep(self.delay)

        # Save build metadata to JSON file (merge with existing data)
        metadata_file = self.output_dir / "build_metadata.json"

        # Load existing metadata if it exists
        existing_builds = {}
        if metadata_file.exists():
            try:
                with metadata_file.open(encoding="utf-8") as f:
                    existing_data = json.load(f)
                    for build in existing_data:
                        name = build.get("name", "")
                        benchmark_type = build.get("benchmark_type", "")
                        key = f"{name}{benchmark_type}"
                        existing_builds[key] = build

                self.logger.info(f"Loaded {len(existing_data)} existing build entries")
            except Exception as e:
                self.logger.warning(f"Could not load existing metadata: {e}")

        # Merge new builds with existing ones (overwrite existing, append new)
        for build in successful_builds:
            name = build.get("name", "")
            benchmark_type = build.get("benchmark_type", "")
            key = f"{name}{benchmark_type}"
            if key in existing_builds:
                self.logger.info(f"Updating existing build: {key}")
            else:
                self.logger.info(f"Adding new build: {key}")
            existing_builds[key] = build  # This updates existing or adds new

        # Convert back to list and save
        merged_builds = list(existing_builds.values())
        with metadata_file.open("w", encoding="utf-8") as f:
            json.dump(merged_builds, f, indent=2, ensure_ascii=False)

        self.logger.info(
            f"Saved merged build metadata to {metadata_file} (total: {len(merged_builds)} builds)",
        )
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


def update_build_date_in_header() -> None:
    version_file_path = Path("../src/Version.h")

    # Get current date in YYYY-MM-DD format
    current_date = datetime.now().strftime("%Y-%m-%d")

    try:
        # Read the current version file
        with version_file_path.open("r", encoding="utf-8") as f:
            content = f.read()

        # Replace the BUILD_DATE line
        updated_content = re.sub(r'#define BUILD_DATE "[\d-]+"', f'#define BUILD_DATE "{current_date}"', content)

        # Write the updated content back
        with version_file_path.open("w", encoding="utf-8") as f:
            f.write(updated_content)

        print(f"✅ Updated BUILD_DATE to {current_date} in {version_file_path}")

    except Exception as e:
        print(f"❌ Error updating build date in {version_file_path}: {e}")


def main() -> None:
    """Main function with CLI interface"""
    parser = argparse.ArgumentParser(
        description="Download benchmark report HTML files from SnowCrows (DPS, Quick, Alac) or manual list",
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
        output_dir=args.output,
        delay=args.delay,
        headless=not args.visible,
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
                f"\n📋 Found {len(builds_and_links)} builds with benchmark report links:",
            )
            for i, build_info in enumerate(builds_and_links, 1):
                filename = scraper._sanitize_build_name(build_info["url_name"])
                dps_info = f" (DPS: {build_info['overall_dps']})" if build_info.get("overall_dps") else ""
                weapons_info = f" - {build_info['weapons']}" if build_info.get("weapons") else ""
                print(
                    f"{i:2d}. {build_info['name']}{weapons_info} ({build_info['benchmark_type'].upper()}){dps_info} -> {filename}",
                )
                print(
                    f"     Profession: {build_info['profession']} ({build_info['elite_spec']})",
                )
                print(f"     Type: {build_info['build_type']}")
                if build_info.get("weapons"):
                    print(f"     Weapons: {build_info['weapons']}")
                if args.manual:
                    print(f"     DPS Report: {build_info['dps_report_url']}")
                    if build_info.get("sc_link_url"):
                        print(f"     SC Link: {build_info['sc_link_url']}")
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

    update_build_date_in_header()


if __name__ == "__main__":
    main()
