import ctypes 
import json
import sys
from common import FontHeader, RECT8

def main():     
    if (len(sys.argv) < 3):
        raise ValueError("usage: convertFont.py [in.json] [out.xfnt]")  
    with open(sys.argv[1], "r") as f:
        fontmap = json.load(f)

    settings = fontmap["settings"]
    sprites = fontmap["sprites"]

    header = FontHeader()
    header.magic = int.from_bytes(b"XFNT", byteorder="little")
    header.firstchar = ord(sprites[0]["char"])
    header.spacewidth = settings["SPACE_WIDTH"]
    header.tabwidth = settings["TAB_WIDTH"]
    header.lineheight = settings["LINE_HEIGHT"]
    header.numchars = len(sprites) 

    with open(sys.argv[2], "wb") as f:
        f.write(header)
        for spr in sprites:
            rect = RECT8()
            rect.set(spr["rect"])
            f.write(rect)

if __name__ == "__main__":
	main()
