"""Draw the PickleballKeeper store icon at the two required sizes."""

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs" / "store"


def render(size: int) -> None:
    scale = 4
    canvas = Image.new("RGB", (size * scale, size * scale), "#091625")
    draw = ImageDraw.Draw(canvas)

    def box(coords: tuple[float, float, float, float]) -> tuple[int, int, int, int]:
        return tuple(round(v * size * scale / 144) for v in coords)

    draw.rounded_rectangle(box((7, 7, 137, 137)), radius=24 * size * scale // 144,
                           outline="#48677B", width=max(1, round(2 * size * scale / 144)))
    draw.rounded_rectangle(box((16, 105, 69, 132)), radius=7 * size * scale // 144,
                           fill="#235A9F")
    draw.rounded_rectangle(box((75, 105, 128, 132)), radius=7 * size * scale // 144,
                           fill="#D95A4B")
    try:
        font = ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial Bold.ttf",
                                  round(22 * size * scale / 144))
    except OSError:
        font = ImageFont.load_default(round(22 * size * scale / 144))
    draw.text((round(42 * size * scale / 144), round(118 * size * scale / 144)),
              "9", fill="#FFFFFF", font=font, anchor="mm")
    draw.text((round(102 * size * scale / 144), round(118 * size * scale / 144)),
              "8", fill="#FFFFFF", font=font, anchor="mm")
    draw.ellipse(box((32, 19, 112, 99)), fill="#DBEB67", outline="#F5F7C3",
                 width=max(1, round(3 * size * scale / 144)))

    holes = ((55, 38, 5), (77, 36, 5), (96, 52, 5), (87, 76, 5),
             (64, 82, 5), (45, 63, 5), (72, 58, 5))
    for x, y, r in holes:
        draw.ellipse(box((x - r, y - r, x + r, y + r)), fill="#26371D")

    filename = "pickleballkeeper-small.png" if size == 80 else "pickleballkeeper-large.png"
    OUT.mkdir(parents=True, exist_ok=True)
    canvas.resize((size, size), Image.Resampling.LANCZOS).save(OUT / filename)


if __name__ == "__main__":
    render(80)
    render(144)
