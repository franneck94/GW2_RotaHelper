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

        # Setup session headers
        self.session.headers.update(
            {
                "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"
            }
        )

        # Setup logging
        logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")
        self.logger = logging.getLogger(__name__)

        # Create output directory
        self.output_dir.mkdir(parents=True, exist_ok=True)

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

    def get_snowcrows_builds_and_links(self) -> list[tuple[str, str, str]]:
        """Extract build names, their corresponding SnowCrows build page links, and benchmark types"""
        benchmark_urls = [
            ("https://snowcrows.com/benchmarks/?filter=dps", "dps"),
            ("https://snowcrows.com/benchmarks/?filter=quickness", "quick"),
            ("https://snowcrows.com/benchmarks/?filter=alacrity", "alac"),
        ]
        builds_and_links = []

        for url, benchmark_type in benchmark_urls:
            try:
                self.logger.info(f"Fetching SnowCrows {benchmark_type.upper()} benchmarks page: {url}")
                response = self.session.get(url, timeout=30)
                response.raise_for_status()

                # Extract build URLs from SnowCrows build links
                # Pattern to match: <a href="/builds/raids/[profession]/[build-name]"
                build_url_pattern = re.compile(
                    r'<a\s+href="(/builds/raids/[^"]+)"[^>]*class="[^"]*flex\s+items-center[^"]*"',
                    re.IGNORECASE,
                )

                build_urls = build_url_pattern.findall(response.text)

                self.logger.info(f"Found {len(build_urls)} {benchmark_type.upper()} build URLs")

                for build_url in build_urls:
                    # Extract build name from URL path
                    # Example: /builds/raids/mesmer/condition-mirage-ih-oasis
                    url_parts = build_url.strip("/").split("/")
                    if len(url_parts) >= 4:
                        build_name = url_parts[-1]  # Last part is the build name

                        # Convert URL format to readable format
                        # Example: condition-mirage-ih-oasis -> Condition Mirage IH Oasis
                        readable_name = self._url_to_readable_name(build_name)

                        # Create full SnowCrows URL
                        full_url = f"https://snowcrows.com{build_url}"

                        builds_and_links.append((readable_name, full_url, benchmark_type))

            except Exception as e:
                self.logger.error(f"Error fetching SnowCrows {benchmark_type} page: {e}")

        # Remove duplicates while preserving order (same build might appear in multiple categories)
        seen = set()
        unique_builds = []
        for build_name, link, benchmark_type in builds_and_links:
            key = (link, benchmark_type)  # Allow same build in different benchmark types
            if key not in seen:
                seen.add(key)
                unique_builds.append((build_name, link, benchmark_type))

        self.logger.info(f"Found {len(unique_builds)} total unique builds across all benchmark types")
        return unique_builds

    def _url_to_readable_name(self, url_name: str) -> str:
        """Convert URL format build name to readable format"""
        # Replace hyphens with spaces and capitalize
        readable = url_name.replace("-", " ").title()

        # Handle special cases
        readable = readable.replace("Ih", "IH")  # Infinite Horizon
        readable = readable.replace("Gs", "GS")  # Greatsword
        readable = readable.replace("Lb", "LB")  # Longbow
        readable = readable.replace("Sb", "SB")  # Shortbow

        return readable

    def _extract_build_name_from_url(self, url_name: str) -> tuple[str, bool]:
        """Extract and format build name from SnowCrows URL path"""
        try:
            self.logger.info(f"Processing SnowCrows URL name: {url_name}")

            # Determine if it's power or condition build
            is_condition = url_name.startswith("condition")

            # Convert URL format to filename format
            # Replace hyphens with underscores and keep lowercase
            build_name = url_name.replace("-", "_")

            self.logger.info(
                f"Extracted build name from URL: {build_name} (type: {'condition' if is_condition else 'power'})"
            )
            return build_name, is_condition

        except Exception as e:
            self.logger.warning(
                f"Could not extract build name from SnowCrows URL '{url_name}': {e}"
            )
            return "unknown_build", False

    def _get_cache_remainder(self, cache_url: str, cache_prefix: str) -> str:
        """Helper method to get remainder after cache prefix"""
        return cache_url[len(cache_prefix) :]

    def convert_cache_url(self, cache_url: str) -> str:
        """Convert cache URLs to proper Guild Wars 2 render URLs"""
        cache1 = "/cache/https_render.guildwars2.com_file_"
        cache2 = "/cache/https_wiki.guildwars2.com_images_"

        # Check if the URL starts with either cache prefix
        if not cache_url.startswith(cache1) and not cache_url.startswith(cache2):
            return cache_url

        # Get the remainder after the cache prefix
        remainder = ""
        if cache_url.startswith(cache1):
            remainder = self._get_cache_remainder(cache_url, cache1)
        elif cache_url.startswith(cache2):
            remainder = self._get_cache_remainder(cache_url, cache2)
        else:
            return cache_url

        # Find the last underscore to separate hash from skill_id
        last_underscore = remainder.rfind("_")
        if last_underscore == -1:
            return cache_url

        # Extract hash and skill_id with extension
        hash_part = remainder[:last_underscore]
        skill_id_with_ext = remainder[last_underscore + 1 :]

        # Remove extension to get skill_id
        dot_pos = skill_id_with_ext.rfind(".")
        skill_id = skill_id_with_ext[:dot_pos] if dot_pos != -1 else skill_id_with_ext

        # Construct the proper render URL
        return f"https://render.guildwars2.com/file/{hash_part}/{skill_id}.png"

    def _process_html_content(self, html_content: str) -> str:
        """Process HTML content to replace cache URLs with proper image URLs"""
        # Pattern to find cache URLs in src attributes
        cache_url_pattern = re.compile(
            r'src="([^"]*(?:/cache/https_render\.guildwars2\.com_file_|/cache/https_wiki\.guildwars2\.com_images_)[^"]*)"',
            re.IGNORECASE,
        )

        def replace_cache_url(match):
            cache_url = match.group(1)
            converted_url = self.convert_cache_url(cache_url)
            return f'src="{converted_url}"'

        # Replace all cache URLs in the HTML content
        processed_content = cache_url_pattern.sub(replace_cache_url, html_content)

        self.logger.info(
            "Processed HTML content: replaced cache URLs with proper image URLs"
        )
        return processed_content

    def download_snowcrows_build(self, build_name: str, url: str, benchmark_type: str = "dps") -> bool:
        """Download HTML content from DPS report found on SnowCrows build page"""
        try:
            self.logger.info(f"Processing {build_name}: {url}")

            if self.driver is None:
                self._setup_webdriver()

            # Step 1: Navigate to the SnowCrows build page
            self.driver.get(url)  # type: ignore

            # Wait for the SnowCrows page to load
            WebDriverWait(self.driver, 10).until(  # type: ignore
                EC.presence_of_element_located((By.TAG_NAME, "body"))
            )

            # Give it a moment to fully load
            time.sleep(SLEEP_DELAY)

            # Step 2: Find the DPS report link on the SnowCrows page
            try:
                self.logger.info("Looking for DPS report link...")
                dps_report_link = WebDriverWait(self.driver, 10).until(  # type: ignore
                    EC.element_to_be_clickable(
                        (By.XPATH, "//a[contains(@href, 'dps.report')]")
                    )
                )

                # Get the DPS report URL
                dps_report_url = dps_report_link.get_attribute("href")
                self.logger.info(f"Found DPS report URL: {dps_report_url}")

                # Step 3: Navigate to the DPS report page
                self.driver.get(dps_report_url)  # type: ignore

                # Wait for the DPS report page to load
                WebDriverWait(self.driver, 10).until(  # type: ignore
                    EC.presence_of_element_located((By.TAG_NAME, "body"))
                )

                # Step 4: Navigate to Player Summary -> Simple Rotation
                try:
                    # Click on "Player Summary" tab
                    self.logger.info("Looking for Player Summary tab...")
                    player_summary_link = WebDriverWait(self.driver, 10).until(  # type: ignore
                        EC.element_to_be_clickable(
                            (
                                By.XPATH,
                                "//a[@class='nav-link' and contains(text(), 'Player Summary')]",
                            )
                        )
                    )
                    player_summary_link.click()
                    self.logger.info("Clicked Player Summary tab")

                    # Wait a moment for the content to load
                    time.sleep(SLEEP_DELAY)

                    # Click on "Simple Rotation" tab
                    self.logger.info("Looking for Simple Rotation tab...")
                    rotation_link = WebDriverWait(self.driver, 10).until(  # type: ignore
                        EC.element_to_be_clickable(
                            (
                                By.XPATH,
                                "//a[@class='nav-link' and contains(., 'Simple') and contains(., 'Rotation')]",
                            )
                        )
                    )
                    rotation_link.click()
                    self.logger.info("Clicked Simple Rotation tab")

                    # Wait for the rotation content to load
                    time.sleep(SLEEP_DELAY)

                except TimeoutException:
                    self.logger.warning(
                        f"Could not find navigation elements for {build_name}, saving current page content"
                    )

                # Step 5: Get the final HTML content
                html_content = self.driver.page_source  # type: ignore

                # Process HTML content to replace cache URLs with proper image URLs
                html_content = self._process_html_content(html_content)

            except TimeoutException:
                self.logger.warning(
                    f"Could not find DPS report link on {url}, skipping..."
                )
                return False

            # Extract the actual build name from the URL path
            url_parts = url.split("/")
            if len(url_parts) >= 1:
                url_build_name = url_parts[-1]  # Last part of URL
                actual_build_name, is_condition = self._extract_build_name_from_url(
                    url_build_name
                )
            else:
                actual_build_name, is_condition = "unknown_build", False

            # Determine build type and create appropriate subdirectory
            build_type = "condition" if is_condition else "power"
            build_subdir = self.output_dir / benchmark_type / build_type
            build_subdir.mkdir(parents=True, exist_ok=True)

            # Generate filename from the extracted build name
            filename = self._sanitize_build_name(actual_build_name)
            file_path = build_subdir / filename

            # Save HTML content
            with open(file_path, "w", encoding="utf-8") as f:
                f.write(html_content)

            self.logger.info(
                f"Saved: {benchmark_type}/{build_type}/{filename} (extracted from SnowCrows URL)"
            )
            return True

        except Exception as e:
            self.logger.error(f"Error downloading {build_name} from {url}: {e}")
            return False

    def download_all_reports(self, builds_and_links: list[tuple[str, str, str]]) -> None:
        """Download all benchmark reports (DPS, Quick, Alac) with rate limiting"""
        success_count = 0
        total_count = len(builds_and_links)

        for i, (build_name, url, benchmark_type) in enumerate(builds_and_links, 1):
            self.logger.info(f"Processing {i}/{total_count}: {build_name} ({benchmark_type.upper()})")

            if self.download_snowcrows_build(build_name, url, benchmark_type):
                success_count += 1

            if i < total_count:  # Don't delay after the last request
                time.sleep(self.delay)

        self.logger.info(f"Download complete: {success_count}/{total_count} successful")

    def run(self) -> None:
        """Main execution method"""
        try:
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
        description="Download benchmark report HTML files from SnowCrows (DPS, Quick, Alac)"
    )
    parser.add_argument(
        "--output",
        "-o",
        type=Path,
        default="data/html",
        help="Output directory for HTML files (default: data/html)",
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

    args = parser.parse_args()

    print(f"📁 Output directory: {args.output}")
    print(f"⏱️  Delay between requests: {args.delay}s")
    print(f"🌐 Browser mode: {'Visible' if args.visible else 'Headless'}")

    scraper = SnowCrowsScraper(
        output_dir=args.output, delay=args.delay, headless=not args.visible
    )

    if args.list_only:
        # Just list the builds and URLs
        print("🔍 Finding builds with benchmark report links (DPS, Quick, Alac)...")
        builds_and_links = scraper.get_snowcrows_builds_and_links()

        if builds_and_links:
            print(f"\n📋 Found {len(builds_and_links)} builds with benchmark report links:")
            for i, (build_name, url, benchmark_type) in enumerate(builds_and_links, 1):
                # For display purposes, still show the original SnowCrows build name
                filename = scraper._sanitize_build_name(build_name)
                print(f"{i:2d}. {build_name} ({benchmark_type.upper()}) -> {filename}")
                print(f"     URL: {url}")
                print(
                    "     Note: Actual filename will be determined from SnowCrows title"
                )
        else:
            print("❌ No builds with benchmark report links found")
    else:
        # Run the full scraper
        print("🌐 Starting SnowCrows benchmark scraper (DPS, Quick, Alac)...")
        scraper.run()
        print("✅ Scraping completed!")


if __name__ == "__main__":
    main()
