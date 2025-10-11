#!/usr/bin/env python3
"""
Web scraper to visit all DPS report sites listed on SnowCrows benchmarks page.

This script scrapes https://snowcrows.com/benchmarks?filter=dps to find all
dps.report links, then visits each one and downloads the HTML content.
"""

import argparse
import logging
import re
import time
from pathlib import Path
from urllib.parse import urljoin

import requests

# Try to import Selenium components
try:
    from selenium import webdriver
    from selenium.webdriver.common.by import By
    from selenium.webdriver.support.ui import WebDriverWait
    from selenium.webdriver.support import expected_conditions as EC
    from selenium.webdriver.chrome.options import Options
    from selenium.common.exceptions import TimeoutException
    SELENIUM_AVAILABLE = True
except ImportError:
    SELENIUM_AVAILABLE = False
    print("Warning: Selenium not installed. Install with: pip install selenium")
    print("WebDriver functionality will not be available.")


class SnowCrowsScraper:
    """Scraper for SnowCrows benchmark DPS report links"""

    def __init__(self, output_dir: Path, delay: float = 1.0, headless: bool = True):
        self.output_dir = output_dir
        self.delay = delay  # Delay between requests to be respectful
        self.headless = headless
        self.session = requests.Session()
        self.driver = None

        # Setup session headers
        self.session.headers.update({
            'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36'
        })

        # Setup logging
        logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')
        self.logger = logging.getLogger(__name__)

        # Create output directory
        self.output_dir.mkdir(parents=True, exist_ok=True)

    def _setup_webdriver(self):
        """Setup Chrome WebDriver with appropriate options"""
        if not SELENIUM_AVAILABLE:
            raise RuntimeError("Selenium is not installed. Please install selenium: pip install selenium")

        if self.driver is not None:
            return

        chrome_options = Options()
        if self.headless:
            chrome_options.add_argument('--headless')
        chrome_options.add_argument('--no-sandbox')
        chrome_options.add_argument('--disable-dev-shm-usage')
        chrome_options.add_argument('--disable-gpu')
        chrome_options.add_argument('--window-size=1920,1080')
        chrome_options.add_argument('--user-agent=Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36')

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
        # Remove or replace problematic characters
        filename = re.sub(r'[^\w\-_.]', '_', build_name.lower())
        # Remove multiple underscores
        filename = re.sub(r'_+', '_', filename)
        # Remove leading/trailing underscores
        filename = filename.strip('_')

        # Ensure it has .html extension
        if not filename.endswith('.html'):
            filename += '.html'

        return filename

    def get_snowcrows_builds_and_links(self) -> list[tuple[str, str]]:
        """Extract build names and their corresponding dps.report links from SnowCrows benchmarks page"""
        url = "https://snowcrows.com/benchmarks?filter=dps"
        builds_and_links = []

        try:
            self.logger.info(f"Fetching SnowCrows benchmarks page: {url}")
            response = self.session.get(url, timeout=30)
            response.raise_for_status()

            # Find build blocks - each contains build info and dps.report link
            # Updated pattern to match the actual HTML structure
            build_pattern = re.compile(
                r'<div[^>]*class="[^"]*transition-all[^"]*"[^>]*>.*?'
                r'<a[^>]*href="([^"]*dps\.report[^"]*)"[^>]*>.*?'
                r'<a[^>]*href="[^"]*"[^>]*class="[^"]*line-clamp-1[^"]*"[^>]*>'
                r'([^<]+)',
                re.DOTALL | re.IGNORECASE
            )

            # Alternative pattern that matches the structure you provided
            alternative_pattern = re.compile(
                r'<a[^>]*href="([^"]*dps\.report[^"]*)"[^>]*>.*?'
                r'<a[^>]*href="[^"]*"[^>]*class="[^"]*line-clamp-1[^"]*"[^>]*>'
                r'([^<]+)',
                re.DOTALL | re.IGNORECASE
            )

            # Try the first pattern
            matches = build_pattern.findall(response.text)
            
            if not matches:
                # Try the alternative pattern
                matches = alternative_pattern.findall(response.text)
            
            if not matches:
                # Try a more flexible pattern based on your HTML structure
                flexible_pattern = re.compile(
                    r'<a[^>]*target="_blank"[^>]*href="([^"]*dps\.report[^"]*)"[^>]*>.*?'
                    r'<a[^>]*href="[^"]*"[^>]*class="[^"]*line-clamp-1[^"]*"[^>]*>'
                    r'([^<\n]+)',
                    re.DOTALL | re.IGNORECASE
                )
                matches = flexible_pattern.findall(response.text)
            
            self.logger.info(f"Found {len(matches)} potential matches with regex patterns")
            
            # If regex patterns fail, try a different approach: extract all DPS report links and build names separately
            if not matches:
                self.logger.info("Regex patterns failed, trying separate extraction approach")
                
                # Extract all DPS report links
                dps_links_pattern = re.compile(r'href="([^"]*dps\.report[^"]*)"', re.IGNORECASE)
                dps_links = dps_links_pattern.findall(response.text)
                
                # Extract all build names from line-clamp-1 elements
                build_names_pattern = re.compile(
                    r'<a[^>]*class="[^"]*line-clamp-1[^"]*"[^>]*>([^<]+)</a>',
                    re.IGNORECASE
                )
                build_names = build_names_pattern.findall(response.text)
                
                self.logger.info(f"Found {len(dps_links)} DPS report links and {len(build_names)} build names")
                
                # Try to pair them up (assuming they appear in the same order)
                min_count = min(len(dps_links), len(build_names))
                for i in range(min_count):
                    matches.append((dps_links[i], build_names[i].strip()))
                    
                # If we still don't have matches, create dummy entries for the DPS links
                if not matches and dps_links:
                    for i, link in enumerate(dps_links):
                        matches.append((link, f"Build_{i+1}"))

            for dps_url, build_name in matches:
                # Clean up the build name
                clean_name = build_name.strip()

                # Make sure the URL is absolute
                if dps_url.startswith('http'):
                    full_url = dps_url
                else:
                    full_url = urljoin(url, dps_url)

                builds_and_links.append((clean_name, full_url))

            # Remove duplicates while preserving order
            seen = set()
            unique_builds = []
            for build_name, link in builds_and_links:
                if link not in seen:
                    seen.add(link)
                    unique_builds.append((build_name, link))

            self.logger.info(f"Found {len(unique_builds)} unique builds with DPS report links")
            return unique_builds

        except Exception as e:
            self.logger.error(f"Error fetching SnowCrows page: {e}")
            return []

    def _extract_build_name_from_dps_report(self, html_content: str) -> str:
        """Extract actual build name from DPS report HTML content"""
        try:
            # Try to extract profession from the JSON data
            profession_match = re.search(r'"profession":"([^"]+)"', html_content)
            profession = profession_match.group(1) if profession_match else "Unknown"
            
            # Try to extract elite specialization or build info from the page
            # Look for elite spec in the JSON data
            elite_spec_patterns = [
                r'"eliteSpec":"([^"]+)"',
                r'"name":"([^"]+)","eliteSpec":true',
            ]
            
            elite_spec = None
            for pattern in elite_spec_patterns:
                match = re.search(pattern, html_content)
                if match:
                    elite_spec = match.group(1)
                    break
            
            # Try to determine if it's power or condition based on stats
            is_condition = False
            
            # First check: look at power vs condition damage stats
            power_match = re.search(r'"power":(\d+)', html_content)
            condi_match = re.search(r'"condi":(\d+)', html_content)
            
            if power_match and condi_match:
                power_val = int(power_match.group(1))
                condi_val = int(condi_match.group(1))
                is_condition = condi_val > power_val
                self.logger.info(f"Stats comparison: Power={power_val}, Condition={condi_val}, IsCondition={is_condition}")
            
            # Second check: look for condition-specific skills or traits
            condition_indicators = [
                r'"name":"[^"]*(?:Burning|Bleeding|Poison|Torment|Confusion|Condi)',
                r'"name":"[^"]*(?:Viper|Sinister|Rabid)',
                r'condition.*damage',
                r'burning.*damage',
                r'bleeding.*damage'
            ]
            
            for indicator in condition_indicators:
                if re.search(indicator, html_content, re.IGNORECASE):
                    is_condition = True
                    self.logger.info(f"Found condition indicator: {indicator}")
                    break
            
            # Third check: look in the build title or page content for "condition"
            if re.search(r'\bcondition\b', html_content, re.IGNORECASE):
                is_condition = True
                self.logger.info("Found 'condition' keyword in HTML content")
            
            # Build the name
            prefix = "condition" if is_condition else "power"
            build_name = elite_spec if elite_spec else profession
            
            # Convert to lowercase and sanitize
            full_name = f"{prefix}_{build_name.lower()}"
            
            self.logger.info(f"Extracted build name: {full_name} (profession: {profession}, elite_spec: {elite_spec})")
            
            return full_name
            
        except Exception as e:
            self.logger.warning(f"Could not extract build name from DPS report: {e}")
            return "unknown_build"

    def download_dps_report(self, build_name: str, url: str) -> bool:
        """Download HTML content from a single DPS report URL after navigating to rotation view"""
        if not SELENIUM_AVAILABLE:
            return self._download_simple(build_name, url)

        try:
            self.logger.info(f"Processing {build_name}: {url}")

            # Setup WebDriver if not already done
            if self.driver is None:
                self._setup_webdriver()

            # Navigate to the DPS report page
            self.driver.get(url)  # type: ignore

            # Wait for the page to load
            WebDriverWait(self.driver, 10).until(  # type: ignore
                EC.presence_of_element_located((By.TAG_NAME, "body"))
            )

            try:
                # Click on "Player Summary" tab
                self.logger.info("Looking for Player Summary tab...")
                player_summary_link = WebDriverWait(self.driver, 10).until(  # type: ignore
                    EC.element_to_be_clickable((By.XPATH, "//a[@class='nav-link' and contains(text(), 'Player Summary')]"))
                )
                player_summary_link.click()
                self.logger.info("Clicked Player Summary tab")

                # Wait a moment for the content to load
                time.sleep(2)

                # Click on "Simple Rotation" tab
                self.logger.info("Looking for Simple Rotation tab...")
                rotation_link = WebDriverWait(self.driver, 10).until(  # type: ignore
                    EC.element_to_be_clickable((By.XPATH, "//a[@class='nav-link' and contains(., 'Simple') and contains(., 'Rotation')]"))
                )
                rotation_link.click()
                self.logger.info("Clicked Simple Rotation tab")

                # Wait for the rotation content to load
                time.sleep(3)

            except TimeoutException:
                self.logger.warning(f"Could not find navigation elements for {build_name}, saving current page content")

            # Get the final HTML content after navigation
            html_content = self.driver.page_source  # type: ignore

            # Extract the actual build name from the DPS report content
            actual_build_name = self._extract_build_name_from_dps_report(html_content)

            # Determine build type and create appropriate subdirectory
            build_type = "condition" if actual_build_name.startswith("condition") else "power"
            build_subdir = self.output_dir / "dps" / build_type
            build_subdir.mkdir(parents=True, exist_ok=True)

            # Generate filename from the extracted build name
            filename = self._sanitize_build_name(actual_build_name)
            file_path = build_subdir / filename

            # Save HTML content
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(html_content)

            self.logger.info(f"Saved: dps/{build_type}/{filename} (extracted from DPS report)")
            return True

        except Exception as e:
            self.logger.error(f"Error downloading {build_name} from {url}: {e}")
            return False

    def _download_simple(self, build_name: str, url: str) -> bool:
        """Fallback method using requests when Selenium is not available"""
        try:
            self.logger.info(f"Downloading (simple mode) {build_name}: {url}")

            response = self.session.get(url, timeout=30)
            response.raise_for_status()

            # Extract the actual build name from the DPS report content
            actual_build_name = self._extract_build_name_from_dps_report(response.text)

            # Determine build type and create appropriate subdirectory
            build_type = "condition" if actual_build_name.startswith("condition") else "power"
            build_subdir = self.output_dir / "dps" / build_type
            build_subdir.mkdir(parents=True, exist_ok=True)

            # Generate filename from the extracted build name
            filename = self._sanitize_build_name(actual_build_name)
            file_path = build_subdir / filename

            # Save HTML content
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(response.text)

            self.logger.info(f"Saved (simple mode): dps/{build_type}/{filename} (extracted from DPS report)")
            return True

        except Exception as e:
            self.logger.error(f"Error downloading {build_name} from {url}: {e}")
            return False

    def download_all_reports(self, builds_and_links: list[tuple[str, str]]) -> None:
        """Download all DPS reports with rate limiting"""
        success_count = 0
        total_count = len(builds_and_links)

        for i, (build_name, url) in enumerate(builds_and_links, 1):
            self.logger.info(f"Processing {i}/{total_count}: {build_name}")

            if self.download_dps_report(build_name, url):
                success_count += 1

            # Rate limiting - be respectful to the servers
            if i < total_count:  # Don't delay after the last request
                time.sleep(self.delay)

        self.logger.info(f"Download complete: {success_count}/{total_count} successful")

    def run(self) -> None:
        """Main execution method"""
        try:
            # Get all builds and DPS report links from SnowCrows
            builds_and_links = self.get_snowcrows_builds_and_links()

            if not builds_and_links:
                self.logger.warning("No builds with DPS report links found")
                return

            # Download all reports
            self.download_all_reports(builds_and_links)

        finally:
            # Always cleanup WebDriver
            self._cleanup_webdriver()


def main():
    """Main function with CLI interface"""
    parser = argparse.ArgumentParser(
        description='Download DPS report HTML files from SnowCrows benchmarks'
    )
    parser.add_argument(
        '--output', '-o',
        type=Path,
        default='data/html',
        help='Output directory for HTML files (default: data/html)'
    )
    parser.add_argument(
        '--delay', '-d',
        type=float,
        default=1.0,
        help='Delay between requests in seconds (default: 1.0)'
    )
    parser.add_argument(
        '--list-only', '-l',
        action='store_true',
        help='Only list the DPS report URLs without downloading'
    )
    parser.add_argument(
        '--visible',
        action='store_true',
        help='Run browser in visible mode (default: headless)'
    )

    args = parser.parse_args()

    print(f"📁 Output directory: {args.output}")
    print(f"⏱️  Delay between requests: {args.delay}s")
    print(f"🌐 Browser mode: {'Visible' if args.visible else 'Headless'}")

    scraper = SnowCrowsScraper(output_dir=args.output, delay=args.delay, headless=not args.visible)

    if args.list_only:
        # Just list the builds and URLs
        print("🔍 Finding builds with DPS report links...")
        builds_and_links = scraper.get_snowcrows_builds_and_links()

        if builds_and_links:
            print(f"\n📋 Found {len(builds_and_links)} builds with DPS report links:")
            for i, (build_name, url) in enumerate(builds_and_links, 1):
                # For display purposes, still show the original SnowCrows build name
                filename = scraper._sanitize_build_name(build_name)
                print(f"{i:2d}. {build_name} -> {filename}")
                print(f"     URL: {url}")
                print("     Note: Actual filename will be determined from DPS report content")
        else:
            print("❌ No builds with DPS report links found")
    else:
        # Run the full scraper
        print("🌐 Starting SnowCrows DPS report scraper...")
        scraper.run()
        print("✅ Scraping completed!")


if __name__ == "__main__":
    main()
