import sys
import ctypes
import struct
import os 
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

cwd = os.getcwd() + "/" 

inputfile = open(sys.argv[1], 'r')

inlines = inputfile.readlines()

textures = []
materials = []
verts = []
uvs = []
faces = []

for l in inlines:
    if (not l.startswith("mtllib ")):
        continue
    mtl = l.strip().split()[1] 
    mf = open(cwd + mtl, 'r')
    
    # parse mtl file
    curmtl = None
    for lm in mf:
        if (lm.startswith("newmtl ")):
            curmtl = lm.strip().split()[1] 
            continue
        if (not lm.startswith("map_Kd ")):
            continue

        tex = lm.strip().split()[1] 
        textures.append(tex)
        materials.append([curmtl, tex])
    mf.close()
print(textures)
#read verts
for l in inlines:
    if (not l.startswith("v ")):
        continue

    x, y, z = map(lambda v: int(float(v) * 32), l.strip().split()[1:4])  # convert to .12 fixed-point todo

    verts.append([x,y,z])

def reorder_z_shape(indices):
    if len(indices) == 4:
        return [indices[0], indices[1], indices[2], indices[3]]
    elif len(indices) == 3:
        return [indices[0], indices[1], indices[2], -1]  # Pad triangle
    else:
        raise ValueError(f"Invalid face: {indices}")
    
#read uv data
for l in inlines:
    if (not l.startswith("vt ")):
        continue
    
    uv = list(map(lambda v: float(v), l.strip().split()[1:3]))
    uvs.append(uv)

print(uvs)

material = None
#read faces
for l in inlines:
    if l.startswith("usemtl "):
        material = l.strip().split()[1]
        continue
    if (not l.startswith("f ")):
        continue

    #read texture resolution
    indices = [int(p.split("/")[0]) - 1 for p in l.strip().split()[1:]]  # 0-based
    uvindices = [int(p.split("/")[1]) - 1 for p in l.strip().split()[1:]]  # 0-based
    indices = reorder_z_shape(indices)
    uvindices = reorder_z_shape(uvindices)
    f = Face()
    f.indices[:] = indices
    if material is not None:
        f.texid = textures.index(dict(materials).get(material)) #textures

        #open texture for uv
        tex = Image.open(cwd + dict(materials).get(material))
        width, height = tex.size
        tex.close()

        #uv texture data, multiply by resolution to map them correctly
        f.u[:] = [int(uvs[i][0]*width) for i in uvindices] 
        f.v[:] = [int(uvs[i][1]*height) for i in uvindices]

        f.color = 0x808080 #set
    else:
        f.color = 0x808080 #set
        f.texid = -1
    faces.append(f)


inputfile.close()
#make model file
outputfile = open(sys.argv[1] + ".mdl", 'wb')
modelheader = ModelFileHeader()
modelheader.magic = struct.unpack('<I', b'MODL')[0]
modelheader.numvertices = len(verts)
modelheader.numfaces = len(faces)
modelheader.numtex = 0
outputfile.write(modelheader)
print("number of vertices ", len(verts))
print("number of faces ", len(faces))
#write verts
for v in verts:
    cv = GTEVector16()
    cv.x = v[0]
    cv.y = v[1]
    cv.z = v[2]
    cv.padding = 0
    outputfile.write(cv)

#write faces
for i in range(modelheader.numfaces):
    outputfile.write(faces[i])

outputfile.close()