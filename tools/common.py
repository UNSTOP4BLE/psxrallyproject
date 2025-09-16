import ctypes

class TextureInfo(ctypes.LittleEndianStructure):
    _pack_ = 1 
    _fields_ = [
        ("u", ctypes.c_uint8),
        ("v", ctypes.c_uint8),
        ("w", ctypes.c_uint16),
        ("h", ctypes.c_uint16),
        ("page", ctypes.c_uint16),
        ("clut", ctypes.c_uint16),
        ("bpp", ctypes.c_uint16)
    ]

class TexHeader(ctypes.LittleEndianStructure):
    _pack_ = 1 
    _fields_ = [
        ("magic", ctypes.c_uint32),
        ("texinfo", TextureInfo),
        ("vrampos", ctypes.c_uint16*2), 
        ("clutpos", ctypes.c_uint16*2), 
        ("clutsize", ctypes.c_uint16), 
        ("texsize", ctypes.c_uint16) 
    ]

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


class GTEVector16(ctypes.LittleEndianStructure):
    _pack_ = 1 
    _fields_ = [
        ("x", ctypes.c_int16),     
        ("y", ctypes.c_int16),     
        ("z", ctypes.c_int16),     
        ("_padding", ctypes.c_int16)   
    ]

class Face(ctypes.LittleEndianStructure):
    _pack_ = 1 
    _fields_ = [
        ("indices", ctypes.c_int16*4),   
        ("color", ctypes.c_uint32),        
        ("u", ctypes.c_uint8 * 4),          
        ("v", ctypes.c_uint8 * 4),        
        ("texid", ctypes.c_int32)
    ]

    def __init__(self):
        self.indices[:] = [0, 0, 0, 0]
        self.color = 0x808080
        self.u[:] = [0, 0, 0, 0]
        self.v[:] = [0, 0, 0, 0]
        self.texid = -1

class ModelFileHeader(ctypes.LittleEndianStructure):
    _pack_ = 1 
    _fields_ = [
        ("magic", ctypes.c_uint32), 
        ("numvertices", ctypes.c_uint32),   
        ("numfaces", ctypes.c_uint32),
        ("numtex", ctypes.c_uint32)
    ]

class GTEVector16(ctypes.LittleEndianStructure):
    _pack_ = 1 
    _fields_ = [
        ("x", ctypes.c_int16),     
        ("y", ctypes.c_int16),     
        ("z", ctypes.c_int16),     
        ("_padding", ctypes.c_int16)   
    ]

class Face(ctypes.LittleEndianStructure):
    _pack_ = 1 
    _fields_ = [
        ("indices", ctypes.c_int16*4),   
        ("color", ctypes.c_uint32),        
        ("u", ctypes.c_uint8 * 4),          
        ("v", ctypes.c_uint8 * 4),        
        ("texid", ctypes.c_int32)
    ]

    def __init__(self):
        self.indices[:] = [0, 0, 0, 0]
        self.color = 0x808080
        self.u[:] = [0, 0, 0, 0]
        self.v[:] = [0, 0, 0, 0]
        self.texid = -1

class ModelFileHeader(ctypes.LittleEndianStructure):
    _pack_ = 1 
    _fields_ = [
        ("magic", ctypes.c_uint32), 
        ("numvertices", ctypes.c_uint32),   
        ("numfaces", ctypes.c_uint32),
        ("numtex", ctypes.c_uint32)
    ]
