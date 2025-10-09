# Web Scraper Usage Examples

## Installation
```bash
pip install -r requirements.txt
```

## Basic Usage
```bash
# Scrape a website with default settings
python get_benches.py https://example.com

# Scrape with custom output directory
python get_benches.py https://example.com --output my_scraped_data
```
## Output Structure
```
scraped_content/
├── index_a1b2c3d4.html          # Main page
├── index_a1b2c3d4.json          # Metadata for main page
├── about_e5f6g7h8.html          # About page
├── about_e5f6g7h8.json          # Metadata for about page
└── crawl_summary.json           # Overall crawl summary
```
