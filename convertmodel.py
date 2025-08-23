import sys
import ctypes
import struct

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
        ("u", ctypes.c_uint8 * 3),          
        ("v", ctypes.c_uint8 * 3),        
        ("texid", ctypes.c_int16)     
    ]

class ModelFileHeader(ctypes.LittleEndianStructure):
    _pack_ = 1 
    _fields_ = [
        ("magic", ctypes.c_uint32),        #should be MODL
        ("numvertices", ctypes.c_uint32),   
        ("numfaces", ctypes.c_uint32),
        ("numtex", ctypes.c_uint32)
    ]

inputfile = open(sys.argv[1], 'r')

inlines = inputfile.readlines()

verts = []
faces = []

#read verts
for l in inlines:
    if (not l.startswith("v ")):
        continue

    x, y, z = map(lambda v: int(float(v) * 32), l.strip().split()[1:4])  # convert to .12 fixed-point

    verts.append([x,y,z])
        
print(verts)

def reorder_z_shape(indices):
    if len(indices) == 4:
        return [indices[0], indices[1], indices[2], indices[3]]
    elif len(indices) == 3:
        return [indices[0], indices[1], indices[2], -1]  # Pad triangle
    else:
        raise ValueError(f"Invalid face: {indices}")
    
#read faces
for l in inlines:
    if (not l.startswith("f ")):
        continue

    # get just the vertex indices, ignore texture/normal
    indices = [int(p.split("/")[0]) - 1 for p in l.strip().split()[1:]]  # 0-based
    indices = reorder_z_shape(indices)
    faces.append(indices)

print(faces)

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
for f in faces:
    if (len(f) != 4): 
        raise ValueError("Face ", f, " is neither a triangle or a quad")
    cf = Face()
    print(f)
    cf.indices[:] = f
    cf.color = 0x00FF00
    cf.u[:] = [0, 0, 0]
    cf.v[:] = [0, 0, 0]
    cf.texid = -1
    outputfile.write(cf)

outputfile.close()