"""Validate PickleballKeeper's RePebble release package, then optionally publish it."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import shutil
import struct
import subprocess
import zipfile


ROOT = Path(__file__).resolve().parents[1]
PBW = ROOT / "dist" / "pickleballkeeper.pbw"
LISTING = ROOT / "docs" / "store" / "listing.md"
MEDIA = ROOT / "docs" / "screenshots"
STORE = ROOT / "docs" / "store"
STAGING = ROOT / ".tmp" / "store-submission"
UUID = "6ba1de5c-79e4-4937-a6b3-40e74b9a2d68"
SHOTS = (
    "live-score-emery.png",
    "game-won-emery.png",
    "game-lost-emery.png",
    "formats-emery.png",
    "saved-match-emery.png",
)


def section(markdown: str, title: str) -> str:
    marker = f"## {title}\n"
    if marker not in markdown:
        raise ValueError(f"Missing listing section: {title}")
    body = markdown.split(marker, 1)[1].split("\n## ", 1)[0].strip()
    if not body:
        raise ValueError(f"Empty listing section: {title}")
    return body


def png_size(path: Path) -> tuple[int, int]:
    with path.open("rb") as handle:
        header = handle.read(24)
    if header[:8] != b"\x89PNG\r\n\x1a\n" or header[12:16] != b"IHDR":
        raise ValueError(f"Not a PNG: {path}")
    return struct.unpack(">II", header[16:24])


def prepare() -> tuple[str, str, list[Path]]:
    if not PBW.is_file():
        raise FileNotFoundError(PBW)
    with zipfile.ZipFile(PBW) as archive:
        info = json.loads(archive.read("appinfo.json"))
    if info.get("displayName") != "PickleballKeeper":
        raise ValueError("PBW displayName is not PickleballKeeper")
    if info.get("uuid") != UUID:
        raise ValueError("PBW UUID changed")
    if info.get("versionLabel") != "0.1.0":
        raise ValueError("PBW version is not 0.1.0")
    if info.get("targetPlatforms") != ["emery"]:
        raise ValueError("PBW does not target Emery only")
    if info.get("watchapp", {}).get("watchface") is not False:
        raise ValueError("PBW is not a watchapp")

    listing = LISTING.read_text(encoding="utf-8")
    name = section(listing, "Name")
    category = section(listing, "Category")
    description = section(listing, "Description")
    release_notes = section(listing, "Release notes")
    if name != "PickleballKeeper" or category != "Health & Fitness":
        raise ValueError("Listing identity or category changed")

    icons = (
        (STORE / "pickleballkeeper-small.png", (80, 80)),
        (STORE / "pickleballkeeper-large.png", (144, 144)),
    )
    for path, expected in icons:
        if png_size(path) != expected:
            raise ValueError(f"Invalid icon size: {path}")

    STAGING.mkdir(parents=True, exist_ok=True)
    staged = []
    for index, filename in enumerate(SHOTS, 1):
        source = MEDIA / filename
        if png_size(source) != (200, 228):
            raise ValueError(f"Invalid Emery screenshot size: {source}")
        target = STAGING / f"emery_{index:02d}_{filename}"
        shutil.copy2(source, target)
        staged.append(target)
    return description, release_notes, staged


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--dry-run", action="store_true")
    action.add_argument("--publish", action="store_true")
    args = parser.parse_args()

    description, release_notes, screenshots = prepare()
    print(f"Validated PBW: {PBW}")
    print("Validated listing: PickleballKeeper 0.1.0 / Health & Fitness")
    print(f"Validated Emery screenshots: {len(screenshots)}")
    if args.dry_run:
        return

    command = [
        "pebble", "publish", "--non-interactive", "--is-published",
        "--no-gif-all-platforms", "--name", "PickleballKeeper",
        "--description", description, "--category", "health",
        "--release-notes", release_notes,
        "--icon-small", str(STORE / "pickleballkeeper-small.png"),
        "--icon-large", str(STORE / "pickleballkeeper-large.png"),
        "--screenshots", *(str(path) for path in screenshots),
    ]
    subprocess.run(command, cwd=ROOT / "watchapp", check=True)


if __name__ == "__main__":
    main()
