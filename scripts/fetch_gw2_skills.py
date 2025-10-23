import asyncio
import json
import logging
from pathlib import Path
from typing import Any, Dict, List

import aiohttp
from aiohttp import ClientSession, ClientTimeout


class GW2SkillFetcher:
    """Fetches skill data from the Guild Wars 2 API."""

    BASE_URL = "https://api.guildwars2.com/v2"
    SKILLS_ENDPOINT = f"{BASE_URL}/skills"
    CHUNK_SIZE = 200  # API supports up to 200 IDs per request

    def __init__(self, output_dir: Path = Path("data/skills")) -> None:
        """Initialize the skill fetcher.

        Args:
            output_dir: Directory to store the output JSON file
        """
        self.output_dir = output_dir
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.logger = self._setup_logger()

        # Configure timeout and connection limits
        self.timeout = ClientTimeout(total=30, connect=10)
        self.connector = aiohttp.TCPConnector(
            limit=10,  # Maximum number of connections
            limit_per_host=5,  # Maximum connections per host
            ttl_dns_cache=300,  # DNS cache TTL
        )

    def _setup_logger(self) -> logging.Logger:
        """Set up logging configuration."""
        logging.basicConfig(
            level=logging.INFO,
            format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
            handlers=[
                logging.StreamHandler(),
                logging.FileHandler(self.output_dir / "fetch_skills.log"),
            ],
        )
        return logging.getLogger(__name__)

    async def get_all_skill_ids(self, session: ClientSession) -> List[int]:
        """Fetch all available skill IDs from the API."""
        self.logger.info("Fetching skill IDs from GW2 API...")

        async with session.get(self.SKILLS_ENDPOINT) as response:
            response.raise_for_status()
            skill_ids = await response.json()

        self.logger.info(f"Found {len(skill_ids)} total skills")
        return skill_ids

    async def fetch_skill_chunk(
        self, session: ClientSession, skill_ids: List[int]
    ) -> List[Dict[str, Any]]:
        """Fetch a chunk of skill data."""
        ids_param = ",".join(map(str, skill_ids))
        url = f"{self.SKILLS_ENDPOINT}?ids={ids_param}&lang=en"

        self.logger.debug(f"Fetching {len(skill_ids)} skills: {skill_ids[:5]}...")

        async with session.get(url) as response:
            response.raise_for_status()
            skills_data = await response.json()

        return skills_data

    def filter_skill_data(self, skill: Dict[str, Any]) -> Dict[str, Any]:
        """Filter skill data to only include required fields."""
        # Extract recharge value from facts
        recharge_value = None
        if "facts" in skill and isinstance(skill["facts"], list):
            for fact in skill["facts"]:
                if (
                    fact.get("type") == "Recharge"
                    and fact.get("text") == "Recharge"
                    and "value" in fact
                ):
                    recharge_value = fact["value"]
                    break

        # Determine if skill is an auto attack
        is_auto_attack = self._is_auto_attack(skill)

        # Build filtered skill data with only the essential fields
        filtered_skill = {
            "icon": skill.get("icon"),
            "id": skill.get("id"),
            "name": skill.get("name"),
            "is_auto_attack": is_auto_attack,
        }

        # Add recharge only if it exists
        if recharge_value is not None:
            filtered_skill["recharge"] = recharge_value
        else:
            filtered_skill["recharge"] = 0

        return filtered_skill

    async def fetch_all_skills(self) -> Dict[str, Any]:
        """Fetch all skill information from the GW2 API."""
        all_skills = {}

        async with ClientSession(
            timeout=self.timeout, connector=self.connector
        ) as session:
            try:
                # Get all skill IDs
                skill_ids = await self.get_all_skill_ids(session)

                # Create chunks for batch requests
                skill_chunks = [
                    skill_ids[i : i + self.CHUNK_SIZE]
                    for i in range(0, len(skill_ids), self.CHUNK_SIZE)
                ]

                self.logger.info(f"Processing {len(skill_chunks)} chunks of skills...")

                # Fetch skills in chunks with controlled concurrency
                semaphore = asyncio.Semaphore(3)  # Limit concurrent requests

                async def fetch_with_semaphore(
                    chunk: List[int],
                ) -> List[Dict[str, Any]]:
                    async with semaphore:
                        return await self.fetch_skill_chunk(session, chunk)

                # Execute all chunk requests concurrently
                tasks = [fetch_with_semaphore(chunk) for chunk in skill_chunks]
                chunk_results = await asyncio.gather(*tasks, return_exceptions=True)

                # Process results and handle any exceptions
                for i, result in enumerate(chunk_results):
                    if isinstance(result, Exception):
                        self.logger.error(f"Failed to fetch chunk {i}: {result}")
                        continue

                    # Organize skills by ID - result should be a list of skills
                    if isinstance(result, list):
                        for skill in result:
                            skill_id = skill.get("id")
                            if skill_id:
                                # Filter skill data to only include required fields
                                filtered_skill = self.filter_skill_data(skill)
                                all_skills[str(skill_id)] = filtered_skill
                    else:
                        self.logger.warning(
                            f"Unexpected result type for chunk {i}: {type(result)}"
                        )

                self.logger.info(f"Successfully fetched {len(all_skills)} skills")

            except Exception as e:
                self.logger.error(f"Error fetching skills: {e}")
                raise

        return all_skills

    def _is_auto_attack(self, skill: Dict[str, Any]) -> bool:
        """Determine if a skill is an auto attack based on various criteria."""
        slot = skill.get("slot")
        if slot == "Weapon_1":
            return True

        return False

    def save_skills_to_file(self, skills_data: Dict[str, Any]) -> None:
        """Save skill data to a JSON file."""
        output_file = self.output_dir / "gw2_skills_en.json"

        try:
            with open(output_file, "w", encoding="utf-8") as f:
                json.dump(skills_data, f, indent=2, ensure_ascii=False, sort_keys=True)

            self.logger.info(f"Skills data saved to: {output_file}")
            self.logger.info(
                f"File size: {output_file.stat().st_size / 1024 / 1024:.2f} MB"
            )

        except Exception as e:
            self.logger.error(f"Error saving skills data: {e}")
            raise

    def create_metadata(self, skills_data: Dict[str, Any]) -> Dict[str, Any]:
        """Create metadata about the fetched skills."""
        from datetime import datetime, timezone

        # Analyze skill types and professions
        skill_types = set()
        professions = set()
        categories = set()

        for skill in skills_data.values():
            if "type" in skill:
                skill_types.add(skill["type"])
            if "professions" in skill:
                professions.update(skill["professions"])
            if "categories" in skill:
                categories.update(skill["categories"])

        metadata = {
            "fetch_timestamp": datetime.now(timezone.utc).isoformat(),
            "total_skills": len(skills_data),
            "api_version": "v2",
            "language": "en",
        }

        return metadata

    def save_metadata(self, skills_data: Dict[str, Any]) -> None:
        """Save metadata about the fetched skills."""
        metadata = self.create_metadata(skills_data)
        metadata_file = self.output_dir / "gw2_skills_metadata.json"

        try:
            with open(metadata_file, "w", encoding="utf-8") as f:
                json.dump(metadata, f, indent=2, ensure_ascii=False)

            self.logger.info(f"Metadata saved to: {metadata_file}")

        except Exception as e:
            self.logger.error(f"Error saving metadata: {e}")
            raise

    async def run(self) -> None:
        """Main execution method."""
        try:
            self.logger.info("Starting GW2 skill data fetch...")

            # Fetch all skills
            skills_data = await self.fetch_all_skills()

            if not skills_data:
                self.logger.warning("No skills data retrieved")
                return

            # Save data and metadata
            self.save_skills_to_file(skills_data)
            self.save_metadata(skills_data)

            self.logger.info("Successfully completed skill data fetch!")

        except Exception as e:
            self.logger.error(f"Failed to fetch skill data: {e}")
            raise

        finally:
            # Close the connector
            if hasattr(self, "connector"):
                await self.connector.close()


async def main() -> None:
    # Get the script directory and set up data path
    script_dir = Path(__file__).parent
    data_dir = script_dir.parent / "data" / "skills"

    # Create and run the fetcher
    fetcher = GW2SkillFetcher(output_dir=data_dir)
    await fetcher.run()


if __name__ == "__main__":
    asyncio.run(main())
