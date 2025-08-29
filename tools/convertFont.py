import ctypes 
import json
import sys

class RECT8(ctypes.LittleEndianStructure):
    _pack_ = 1 
    _fields_ = [
        ("x", ctypes.c_uint8),
        ("y", ctypes.c_uint8),
        ("w", ctypes.c_uint8), 
        ("h", ctypes.c_uint8)
    ]

    def set(self, r):
        self.x, self.y, self.w, self.h = r
         

class FontHeader(ctypes.LittleEndianStructure):
    _pack_ = 1 
    _fields_ = [
        ("magic",      ctypes.c_uint32),
        ("firstchar",  ctypes.c_uint8),
        ("spacewidth", ctypes.c_uint8),
        ("tabwidth",   ctypes.c_uint8), 
        ("lineheight", ctypes.c_uint8),
        ("numchars",   ctypes.c_uint8),
        ("_padding",   ctypes.c_uint8 * 3)
    ]


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
    header.firstchar = settings["LINE_HEIGHT"]
    header.numchars = len(sprites)

    with open(sys.argv[2], "wb") as f:
        f.write(header)
        for spr in sprites:
            rect = RECT8()
            rect.set(spr["rect"])
            f.write(rect)

if __name__ == "__main__":
	main()
