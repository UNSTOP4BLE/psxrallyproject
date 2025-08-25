import sys
import ctypes
import struct
import os 
import re
from PIL import Image

class GTEVector16(ctypes.LittleEndianStructure):
    _pack_ = 1 
    _fields_ = [
        ("x", ctypes.c_int16),     
        ("y", ctypes.c_int16),     
        ("z", ctypes.c_int16),     
        ("padding", ctypes.c_int16)   
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

class ModelFileHeader(ctypes.LittleEndianStructure):
    _pack_ = 1 
    _fields_ = [
        ("magic", ctypes.c_uint32),        #should be MODL
        ("numvertices", ctypes.c_uint32),   
        ("numfaces", ctypes.c_uint32),
        ("numtex", ctypes.c_uint32)
    ]

def reorder_z_shape(indices):
    if len(indices) == 4:
        return [indices[0], indices[1], indices[3], indices[2]]
    elif len(indices) == 3:
        return [indices[0], indices[1], indices[2], -1]  # Pad triangle
    else:
        raise ValueError(f"Invalid face: {indices}")

class Material:    
    def __init__(self):
        self.name = None
        self.col = 0x808080 #default neutral gray, diffuse color
        self.texture = None
        self.texid = -1

    def setcol(self, r, g, b):
        r_int = int(max(0, min(1, float(r))) * 255)
        g_int = int(max(0, min(1, float(g))) * 255)
        b_int = int(max(0, min(1, float(b))) * 255)
        self.col = (r_int << 16) | (g_int << 8) | b_int

    def print_info(self):
        print(f"Material Name: {self.name}")
        print(f"Color (RGB): ", self.col)
        print(f"Texture: {self.texture}")
        print(f"Texture ID: {self.texid}")

materials = []
vertices = []
uvs = []
faces = []
textures = []
cwd = os.getcwd() + "/" 

#open the obj file
fin = open(sys.argv[1], 'r')

#read the mtl file, this fucking sucks and needs a rewrite
match = re.search(r"mtllib\s+(.+\.mtl)", fin.read())
fin.seek(0) 
mtl_file = match.group(1) if match else None
print("material file:", mtl_file)

if mtl_file is not None:
    fmtl = open(cwd + mtl_file, 'r')
    matlen = fmtl.read().count("newmtl")
    fmtl.seek(0)
    lines = fmtl.readlines()  #read all lines once cause im a idiot
    line_index = 0
    for i in range(matlen):  #loop x times
        mtl = Material()
        found_name = False
        #read material data until the next newmtl or end of file
        while line_index < len(lines):
            line = lines[line_index]
            data = line.split()
            line_index += 1
            if not data:  #skip empty lines
                continue
            if re.search("^newmtl ", line):
                if found_name:  #if we already have a name, start a new material
                    line_index -= 1  #rewind to process this newmtl in the next iteration
                    break
                mtl.name = data[1]
                found_name = True
            elif found_name and re.search("^Kd ", line):
                mtl.setcol(data[1], data[2], data[3])
            elif found_name and re.search("^map_Kd ", line):
                textures.append(data[1])
                mtl.texture = data[1]
                mtl.texid = textures.index(data[1])
        if found_name:  #only append if we found a material name
            print("adding material", mtl.name)
            materials.append(mtl)

fin.seek(0) #reset pointer cause of fin.read()
fmtl.close()

#read the obj file
curmat = None
for line in fin.readlines():
    data = line.split()
    #vertices
    if (re.search("^v ", line)):
        v = GTEVector16()
        #should be fixed point, 4096 instead of 32
        v.x = int(float(data[1])*32)
        v.y = int(float(data[2])*32)
        v.z = int(float(data[3])*32)
        vertices.append(v)

    #uv texture data
    if (re.search("^vt ", line)):
        u = 1.0 - float(data[1])
        v = float(data[2])
        uvs.append([u, v])

    #read faces 
    #change material if found
    if (re.search("^usemtl ", line)):
        curmat = next((m for m in materials if m.name == data[1]), None)
        print("using material", data[1])
    #faces 
    if (re.search("^f ", line)):
        #indicie, uv indice, normal indice
        #f 4/1/2 3/2/2 7/3/2 8/4/2
        f = Face()
        matches = re.findall(r'(\d+)/(\d+)/(\d+)', line)
        #indices
        vert_indices = [int(v)-1 for v, vt, vn in matches]
        vert_indices = reorder_z_shape(vert_indices)
        f.indices[:] = vert_indices 
        f.color = 0x808080
        f.texid = -1
        #uv indices
        uv_indices = [int(vt)-1 for v, vt, vn in matches]  
        uv_indices = reorder_z_shape(uv_indices)

        if curmat is not None:
            f.color = curmat.col
            if not curmat.texid < 0: #face is textured
                tex = Image.open(cwd + curmat.texture)
                width, height = tex.size
                width -= 1
                height -= 1
                tex.close()
                f.u[:] = [int(uvs[i][0]*width) for i in uv_indices]
                f.v[:] = [int(uvs[i][1]*height) for i in uv_indices]
                f.texid = curmat.texid
                
        
        faces.append(f)

fin.close()

#make model file
fout = open(sys.argv[1] + ".mdl", 'wb')

header = ModelFileHeader()
#magic number
header.magic = struct.unpack('<I', b'MODL')[0]

header.numvertices = len(vertices)
header.numfaces    = len(faces)
#header.numtex      = len(textures)
fout.write(header)

print("number of vertices ", len(vertices))
print("number of faces ", len(faces))
#write verts
for i in range(header.numvertices):
    fout.write(vertices[i])

#write faces
for i in range(header.numfaces):
    fout.write(faces[i])

fout.close()