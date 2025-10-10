import argparse
import json
import logging
import re
from pathlib import Path
from urllib.parse import urlparse

import requests

from bench_list import BENCHES


class HTMLDownloader:
    """Simple HTML downloader with JSON extraction"""

    def __init__(self, output_dir: Path):
        self.output_dir = output_dir
        self.session = requests.Session()

        # Setup session
        self.session.headers.update({
            'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36'
        })

        # Setup logging
        logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')
        self.logger = logging.getLogger(__name__)

        # Create output directory
        self.output_dir.mkdir(exist_ok=True)

    def _sanitize_filename(self, url: str) -> str:
        """Convert URL to safe filename"""
        parsed = urlparse(url)
        path = parsed.path.strip('/') or 'index'

        # Get the last part of the path (filename)
        filename = path.split('/')[-1]

        # Replace problematic characters
        filename = ''.join(c if c.isalnum() or c in '-_.' else '_' for c in filename)

        # Ensure it has .html extension
        if not filename.endswith('.html'):
            filename += '.html'

        return filename

    def _extract_log_data(self, html_content: str) -> dict | None:
        """Extract JSON data from const _logData = line"""
        try:
            # Find the line that starts with "const _logData ="
            pattern = r'const\s+_logData\s*=\s*({.*?});?\s*$'
            match = re.search(pattern, html_content, re.MULTILINE | re.DOTALL)

            if match:
                json_str = match.group(1)
                return json.loads(json_str)
            else:
                self.logger.warning("Could not find 'const _logData =' in HTML content")
                return None

        except json.JSONDecodeError as e:
            self.logger.error(f"Error parsing JSON from _logData: {e}")
            return None
        except Exception as e:
            self.logger.error(f"Error extracting _logData: {e}")
            return None

    def _fetch_and_save(self, url: str, name: str | None = None, save_html: bool = False) -> bool:
        """Fetch HTML and save both HTML and extracted JSON"""
        try:
            self.logger.info(f"Fetching: {url}")

            response = self.session.get(url, timeout=30)
            response.raise_for_status()

            content_type = response.headers.get('content-type', '')
            if 'html' not in content_type:
                self.logger.warning(f"Content is not HTML: {content_type}")
                return False

            html_content = response.text

            # Generate filename
            if name:
                html_filename = f"{name}_v1.html"
                json_filename = f"{name}_v1.json"
            else:
                base_filename = self._sanitize_filename(url)
                base_stem = base_filename.replace('.html', '')
                html_filename = f"{base_stem}_v1.html"
                json_filename = f"{base_stem}_v1.json"

            # Save HTML file only if requested
            if save_html:
                html_path = self.output_dir / html_filename
                with open(html_path, 'w', encoding='utf-8') as f:
                    f.write(html_content)
                self.logger.info(f"Saved HTML: {url} -> {html_filename}")

            # Extract and save JSON data
            log_data = self._extract_log_data(html_content)
            if log_data:
                # Save full JSON data
                json_path = self.output_dir / json_filename
                with open(json_path, 'w', encoding='utf-8') as f:
                    json.dump(log_data, f, indent=2, ensure_ascii=False)
                self.logger.info(f"Saved JSON: {json_filename}")
            else:
                self.logger.warning(f"No JSON data extracted from {url}")

            return True

        except Exception as e:
            self.logger.error(f"Error fetching {url}: {e}")
            return False

    def download_urls(self, urls: dict, save_html: bool = False) -> None:
        """Download multiple URLs"""
        success_count = 0
        total_count = len(urls)

        for name, url in urls.items():
            self.logger.info(f"Processing: {name}")
            if self._fetch_and_save(url, name, save_html):
                success_count += 1

        self.logger.info(f"Download complete: {success_count}/{total_count} successful")


def main():
    """Main function with CLI arguments"""
    parser = argparse.ArgumentParser(description='Download HTML files and extract JSON data')
    parser.add_argument('--output', '-o', default='data/bench',
                       help='Output directory (default: data/bench)')
    parser.add_argument('--url', help='Single URL to download (optional)')
    parser.add_argument('--save-html', action='store_true',
                       help='Save HTML files (default: False, only saves JSON)')

    args = parser.parse_args()
    output = Path(args.output)

    print(f"📁 Output directory: {args.output}")
    print(f"💾 Save HTML files: {args.save_html}")

    downloader = HTMLDownloader(output_dir=output)

    if args.url:
        # Download single URL
        print(f"🌐 Downloading single URL: {args.url}")
        downloader._fetch_and_save(args.url, save_html=args.save_html)
    else:
        # Download all bench URLs
        print(f"🌐 Downloading {len(BENCHES)} benchmark URLs")
        downloader.download_urls(BENCHES, save_html=args.save_html)

    print("✅ Download completed!")


if __name__ == "__main__":
    main()
