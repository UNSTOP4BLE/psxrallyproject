import argparse
from pathlib import Path
from tools.convertImage import convert_image

# Path to the assets folder
ASSET_PATH = Path("./assets")

# Global list of all visible files, ignoring hidden files and folders
SRCFILES = [f for f in ASSET_PATH.rglob("*") if f.is_file() and all(not p.startswith(".") for p in f.parts)]

BUILT_IMAGES = []

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("-o", "--output", help="Output folder", required=True)
    return parser.parse_args()

def conv_images(imgs, output_folder: Path):
    """
    Convert all images in imgs list using corresponding .vram files.
    """
    output_folder.mkdir(parents=True, exist_ok=True)

    for img in imgs:
        if img.suffix.lower() != ".png":
            continue  # skip non-PNGs

        basename = img.stem
        vram_file = img.parent / f"{basename}.vram"
        if not vram_file.exists():
            print(f"Warning: VRAM file {vram_file} not found. Skipping {img}")
            continue

        out_file = output_folder / f"{basename}.xtex"

        # Open VRAM file and output file, then call convert_image
        with open(vram_file, "r") as vram, open(out_file, "wb") as out:
            convert_image(img, vram, out)

        BUILT_IMAGES.append(out_file)

    return BUILT_IMAGES

def main():
    args = parse_args()
    output_path = Path(args.output)

    print(f"Output folder: {output_path}")

    built_images = conv_images(SRCFILES, output_path)

    print("Built images:")
    for img in built_images:
        print(img)

if __name__ == "__main__":
    main()