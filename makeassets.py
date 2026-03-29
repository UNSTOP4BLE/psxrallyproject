import argparse
from pathlib import Path
from tools.convertImage import convert_image
import shutil

# Path to the assets folder
ASSET_PATH = Path(__file__).parent / "assets"

# Global list of all visible files, ignoring hidden files and folders
SRCFILES = [f for f in ASSET_PATH.rglob("*") if f.is_file() and all(not p.startswith(".") for p in f.parts)]

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("-o", "--output", help="Output folder", required=True)
    return parser.parse_args()

def main():
    args = parse_args()
    output_path = Path(args.output).resolve()

    print(f"Output folder: {output_path}")

    for f in SRCFILES:
        relfile = f.relative_to(ASSET_PATH)

        if f.suffix.lower() == ".png":
            # build output path with same structure
            out_file = output_path / relfile.with_suffix(".xtex")
            out_file.parent.mkdir(parents=True, exist_ok=True)

            convert_image(f, f.with_suffix(".vram"), out_file)

            print("Converted image", f)
        #skip
        elif f.suffix.lower() in (".vram", ".obj", ".mtl"):
            continue
        else:
            #copy file if dont have to convert
            out_file = output_path / relfile
            out_file.parent.mkdir(parents=True, exist_ok=True)

            shutil.copy2(f, out_file)


if __name__ == "__main__":
    main()