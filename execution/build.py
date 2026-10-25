"""Clean Pebble build and copy the package to dist/."""

from pathlib import Path
import shutil
import subprocess


ROOT = Path(__file__).resolve().parents[1]
WATCHAPP = ROOT / "watchapp"
DIST = ROOT / "dist"


def run(command: list[str]) -> None:
    subprocess.run(command, cwd=WATCHAPP, check=True)


def main() -> None:
    run(["pebble", "clean"])
    run(["pebble", "build"])
    source = WATCHAPP / "build" / "watchapp.pbw"
    if not source.is_file():
        raise FileNotFoundError(source)
    DIST.mkdir(parents=True, exist_ok=True)
    target = DIST / "pickleballkeeper.pbw"
    shutil.copy2(source, target)
    print(target)


if __name__ == "__main__":
    main()
