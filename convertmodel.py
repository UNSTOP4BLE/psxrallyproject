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

vertices = []
uvs = []
faces = []
cwd = os.getcwd() + "/" 

#read the obj file
fin = open(sys.argv[1], 'r')
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
        curmat = data[1]
    
    #faces 
    if (re.search("^f ", line)):
#read faces
#for l in inlines:
#    if l.startswith("usemtl "):
#        material = l.strip().split()[1]
#        continue
#    if (not l.startswith("f ")):
#        continue

#    indices = [int(p.split("/")[0]) - 1 for p in l.strip().split()[1:]]  # 0-based
#    indices = reorder_z_shape(indices)

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