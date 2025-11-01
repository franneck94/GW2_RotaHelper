import asyncio
import argparse
import json
import logging
from enum import Enum
from pathlib import Path
from typing import Any, Dict, List

import aiohttp
from aiohttp import ClientSession, ClientTimeout


class SkillSlot(Enum):
    NONE = 0
    WEAPON_1 = 1
    WEAPON_2 = 2
    WEAPON_3 = 3
    WEAPON_4 = 4
    WEAPON_5 = 5
    HEAL = 6
    UTILITY_1 = 7
    UTILITY_2 = 8
    UTILITY_3 = 9
    ELITE = 10
    PROFESSION_1 = 11
    PROFESSION_2 = 12
    PROFESSION_3 = 13
    PROFESSION_4 = 14
    PROFESSION_5 = 15
    PROFESSION_6 = 16
    PROFESSION_7 = 17


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
        # Extract recharge and cast time values from facts
        recharge_value = None
        cast_time_value = None

        if "facts" in skill and isinstance(skill["facts"], list):
            for fact in skill["facts"]:
                if (
                    fact.get("type") == "Recharge"
                    and fact.get("text") == "Recharge"
                    and "value" in fact
                ):
                    recharge_value = fact["value"]
                elif (
                    fact.get("type") == "Time"
                    and fact.get("text") == "Cast Time"
                    and "value" in fact
                ):
                    cast_time_value = fact["value"]

        # Determine skill types based on slot
        slot = skill.get("slot", "")
        is_auto_attack = self._is_auto_attack(skill)
        is_weapon_skill = self._is_weapon_skill(slot)
        is_utility_skill = self._is_utility_skill(slot)
        is_elite_skill = self._is_elite_skill(slot)
        is_heal_skill = self._is_heal_skill(slot)
        is_profession_skill = self._is_profession_skill(slot)
        skill_type_int = self._get_skill_type_int(slot)

        # Special rule for Necromancer: Downed_1 to Downed_5 are treated as weapon skills
        if self._is_necromancer_downed_skill(skill):
            is_weapon_skill = True
            # Update skill type for Necromancer downed skills
            if slot == "Downed_1":
                is_auto_attack = True
                skill_type_int = SkillSlot.WEAPON_1.value
            elif slot == "Downed_2":
                skill_type_int = SkillSlot.WEAPON_2.value
            elif slot == "Downed_3":
                skill_type_int = SkillSlot.WEAPON_3.value
            elif slot == "Downed_4":
                skill_type_int = SkillSlot.WEAPON_4.value
            elif slot == "Downed_5":
                skill_type_int = SkillSlot.WEAPON_5.value

        # Build filtered skill data with only the essential fields
        filtered_skill = {
            "icon": skill.get("icon"),
            "id": skill.get("id"),
            "name": skill.get("name"),
            "skill_type": str(skill_type_int),
            "is_auto_attack": is_auto_attack,
            "is_weapon_skill": is_weapon_skill,
            "is_utility_skill": is_utility_skill,
            "is_elite_skill": is_elite_skill,
            "is_heal_skill": is_heal_skill,
            "is_profession_skill": is_profession_skill,
        }

        # Add recharge and cast time, defaulting to 0 if not found
        filtered_skill["recharge"] = recharge_value if recharge_value is not None else 0
        filtered_skill["cast_time"] = (
            cast_time_value if cast_time_value is not None else 0.0
        )

        return filtered_skill

    async def fetch_all_skills(self) -> tuple[Dict[str, Any], Dict[str, Any]]:
        """Fetch all skill information from the GW2 API."""
        all_skills_raw = {}
        all_skills_filtered = {}

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
                                # Store raw skill data
                                all_skills_raw[str(skill_id)] = skill
                                # Filter skill data to only include required fields
                                filtered_skill = self.filter_skill_data(skill)
                                all_skills_filtered[str(skill_id)] = filtered_skill
                    else:
                        self.logger.warning(
                            f"Unexpected result type for chunk {i}: {type(result)}"
                        )

                self.logger.info(
                    f"Successfully fetched {len(all_skills_filtered)} skills"
                )

            except Exception as e:
                self.logger.error(f"Error fetching skills: {e}")
                raise

        return all_skills_raw, all_skills_filtered

    def _is_auto_attack(self, skill: Dict[str, Any]) -> bool:
        """Determine if a skill is an auto attack based on various criteria."""
        slot = skill.get("slot")
        if slot == "Weapon_1":
            return True

        return False

    def _is_weapon_skill(self, slot: str) -> bool:
        """Determine if a skill is a weapon skill based on slot."""
        weapon_slots = {
            "Weapon_1",
            "Weapon_2",
            "Weapon_3",
            "Weapon_4",
            "Weapon_5",
            "Weapon",
        }
        return slot in weapon_slots

    def _is_utility_skill(self, slot: str) -> bool:
        """Determine if a skill is a utility skill based on slot."""
        return slot == "Utility"

    def _is_elite_skill(self, slot: str) -> bool:
        """Determine if a skill is an elite skill based on slot."""
        return slot == "Elite"

    def _is_heal_skill(self, slot: str) -> bool:
        """Determine if a skill is a heal skill based on slot."""
        return slot == "Heal"

    def _is_profession_skill(self, slot: str) -> bool:
        """Determine if a skill is a profession skill based on slot."""
        profession_slots = {
            "Profession_1",
            "Profession_2",
            "Profession_3",
            "Profession_4",
            "Profession_5",
        }
        return slot in profession_slots

    def _get_skill_type_int(self, slot: str) -> int:
        """Get the SkillSlot enum integer value based on slot."""
        slot_to_skill_type = {
            "Weapon_1": SkillSlot.WEAPON_1,
            "Weapon_2": SkillSlot.WEAPON_2,
            "Weapon_3": SkillSlot.WEAPON_3,
            "Weapon_4": SkillSlot.WEAPON_4,
            "Weapon_5": SkillSlot.WEAPON_5,
            "Weapon": SkillSlot.WEAPON_1,  # Generic weapon slot, default to weapon 1
            "Heal": SkillSlot.HEAL,
            "Utility": SkillSlot.UTILITY_1,  # Default utility to utility 1, could be refined later
            "Elite": SkillSlot.ELITE,
            "Profession_1": SkillSlot.PROFESSION_1,
            "Profession_2": SkillSlot.PROFESSION_2,
            "Profession_3": SkillSlot.PROFESSION_3,
            "Profession_4": SkillSlot.PROFESSION_4,
            "Profession_5": SkillSlot.PROFESSION_5,
            # Downed skills (mapped to weapon slots)
            "Downed_1": SkillSlot.WEAPON_1,
            "Downed_2": SkillSlot.WEAPON_2,
            "Downed_3": SkillSlot.WEAPON_3,
            "Downed_4": SkillSlot.WEAPON_4,
        }

        return slot_to_skill_type.get(slot, SkillSlot.NONE).value

    def _is_necromancer_downed_skill(self, skill: Dict[str, Dict[str, Any]]) -> bool:
        """Check if this is a Necromancer downed skill that should be treated as weapon skill."""
        slot: str = skill.get("slot", "")  # type: ignore
        professions = skill.get("professions", [])

        # Check if it's a Necromancer and has a Downed slot
        if "Necromancer" in professions and slot.startswith("Downed_"):
            return True
        return False

    def save_raw_skills_to_file(self, skills_data: Dict[str, Any]) -> None:
        """Save all raw skill data to a JSON file without any filtering."""
        output_file = self.output_dir / "gw2_skills_raw.json"

        try:
            with open(output_file, "w", encoding="utf-8") as f:
                json.dump(skills_data, f, indent=2, ensure_ascii=False, sort_keys=True)

            self.logger.info(f"Raw skills data saved to: {output_file}")
            self.logger.info(f"Saved {len(skills_data)} raw skills")
            self.logger.info(
                f"File size: {output_file.stat().st_size / 1024 / 1024:.2f} MB"
            )

        except Exception as e:
            self.logger.error(f"Error saving raw skills data: {e}")
            raise

    def save_skills_to_file(self, skills_data: Dict[str, Dict[str, Any]]) -> None:
        """Save skill data to a JSON file, filtering out uncategorized skills."""
        # Filter out skills that have all boolean flags set to false
        categorized_skills = {}

        for skill_id, skill_data in skills_data.items():
            # Keep skills that have at least one boolean flag set to true
            if (
                skill_data.get("is_auto_attack", False)
                or skill_data.get("is_elite_skill", False)
                or skill_data.get("is_heal_skill", False)
                or skill_data.get("is_utility_skill", False)
                or skill_data.get("is_weapon_skill", False)
                or skill_data.get("is_profession_skill", False)
            ):
                categorized_skills[skill_id] = skill_data

        categorized_skills["9999"] = {
            "icon": "",
            "id": 9999,
            "name": "Weapon Swap",
            "skill_type": str(SkillSlot.NONE.value),
            "is_auto_attack": False,
            "is_weapon_skill": False,
            "is_utility_skill": False,
            "is_elite_skill": False,
            "is_heal_skill": False,
            "is_profession_skill": False,
            "recharge": 0,
            "cast_time": 0.0,
        }

        categorized_skills["-9999"] = {
            "icon": "",
            "id": -9999,
            "name": "Unknown Skill",
            "skill_type": str(SkillSlot.NONE.value),
            "is_auto_attack": False,
            "is_weapon_skill": False,
            "is_utility_skill": False,
            "is_elite_skill": False,
            "is_heal_skill": False,
            "is_profession_skill": False,
            "recharge": 0,
            "cast_time": 0.0,
        }

        output_file = self.output_dir / "gw2_skills_en.json"

        try:
            with open(output_file, "w", encoding="utf-8") as f:
                json.dump(
                    categorized_skills, f, indent=2, ensure_ascii=False, sort_keys=True
                )

            self.logger.info(f"Skills data saved to: {output_file}")
            self.logger.info(
                f"Saved {len(categorized_skills)} categorized skills out of {len(skills_data)} total skills"
            )
            self.logger.info(
                f"File size: {output_file.stat().st_size / 1024 / 1024:.2f} MB"
            )

        except Exception as e:
            self.logger.error(f"Error saving skills data: {e}")
            raise

    def save_uncategorized_skills(self, skills_data: Dict[str, Dict[str, Any]]) -> None:
        """Save skills that have all boolean flags set to false."""
        uncategorized_skills = {}

        for skill_id, skill_data in skills_data.items():
            # Check if all boolean flags are false
            if (
                not skill_data.get("is_auto_attack", False)
                and not skill_data.get("is_elite_skill", False)
                and not skill_data.get("is_heal_skill", False)
                and not skill_data.get("is_utility_skill", False)
                and not skill_data.get("is_weapon_skill", False)
                and not skill_data.get("is_profession_skill", False)
            ):
                uncategorized_skills[skill_id] = skill_data

        output_file = self.output_dir / "gw2_uncategorized_skills.json"

        try:
            with open(output_file, "w", encoding="utf-8") as f:
                json.dump(
                    uncategorized_skills,
                    f,
                    indent=2,
                    ensure_ascii=False,
                    sort_keys=True,
                )

            self.logger.info(f"Uncategorized skills data saved to: {output_file}")
            self.logger.info(
                f"Found {len(uncategorized_skills)} uncategorized skills out of {len(skills_data)} total skills"
            )
            self.logger.info(
                f"File size: {output_file.stat().st_size / 1024 / 1024:.2f} MB"
            )

        except Exception as e:
            self.logger.error(f"Error saving uncategorized skills data: {e}")
            raise

    def create_metadata(self, skills_data: Dict[str, Dict[str, Any]]) -> Dict[str, Any]:
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

    def save_metadata(self, skills_data: Dict[str, Dict[str, Any]]) -> None:
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

    async def run(self, save_raw: bool = False) -> None:
        """Main execution method."""
        try:
            self.logger.info("Starting GW2 skill data fetch...")

            # Fetch all skills
            raw_skills_data, filtered_skills_data = await self.fetch_all_skills()

            if not filtered_skills_data:
                self.logger.warning("No skills data retrieved")
                return

            # Save data and metadata
            if save_raw:
                self.save_raw_skills_to_file(raw_skills_data)  # Save raw data first
            self.save_skills_to_file(filtered_skills_data)
            self.save_uncategorized_skills(filtered_skills_data)
            self.save_metadata(filtered_skills_data)

            self.logger.info("Successfully completed skill data fetch!")

        except Exception as e:
            self.logger.error(f"Failed to fetch skill data: {e}")
            raise

        finally:
            # Close the connector
            if hasattr(self, "connector"):
                await self.connector.close()


async def main() -> None:
    # Parse command line arguments
    parser = argparse.ArgumentParser(
        description="Fetch skill data from the Guild Wars 2 API"
    )
    parser.add_argument(
        "--save-raw",
        action="store_true",
        default=False,
        help="Save raw unfiltered skill data to gw2_skills_raw.json (default: False)",
    )
    parser.add_argument(
        "--output",
        "-o",
        type=str,
        default="data/skills",
        help="Output directory for skill files (default: data/skills)",
    )

    args = parser.parse_args()

    # Get the script directory and set up data path
    script_dir = Path(__file__).parent
    if args.output.startswith("/") or (len(args.output) > 1 and args.output[1] == ":"):
        # Absolute path
        data_dir = Path(args.output)
    else:
        # Relative path
        data_dir = script_dir.parent / args.output

    # Create and run the fetcher
    fetcher = GW2SkillFetcher(output_dir=data_dir)
    await fetcher.run(save_raw=args.save_raw)


if __name__ == "__main__":
    asyncio.run(main())
