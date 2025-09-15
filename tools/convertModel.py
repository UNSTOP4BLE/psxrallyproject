import sys
import ctypes
import struct
import os 
import re
from PIL import Image
from common import GTEVector16, Face, TexHeader, ModelFileHeader

def reorder_z_shape(indices):
    print(indices)
    if (len(indices) == 4):
        return [indices[0], indices[1], indices[3], indices[2]]
    elif (len(indices) == 3):
        return [indices[0], indices[1], indices[2], -1]  # Pad triangle
    else:
        raise ValueError(f"Invalid face: {indices}")

class Material:    
    def __init__(self):
        self.name = None
        self.color = 0x808080 #default neutral gray, diffuse color
        self.texture = None
        self.texid = -1

    def setcol(self, r, g, b):
        r_int = int(max(0, min(1, float(r))) * 255)
        g_int = int(max(0, min(1, float(g))) * 255)
        b_int = int(max(0, min(1, float(b))) * 255)
        self.color = (b_int << 16) | (g_int << 8) | r_int

def error(txt):
    print(txt, file=sys.stderr)
    sys.exit(1)
    
if (__name__ == '__main__'):
    materials = []
    vertices = []
    uvs = []
    faces = []
    textures = []

    if (len(sys.argv) < 4):
        error("wrong args, usage: convertModel [in] [out] [out texture dir]")
    #open the obj file
    fin = open(sys.argv[1], 'r')

    #read the mtl file
    match = re.search(r"mtllib\s+(.+\.mtl)", fin.read())
    fin.seek(0) 
    mtl_file = match.group(1) if match else None
    print("material file:", mtl_file)

    if (mtl_file is not None):
        fmtl = open(os.path.join(os.path.dirname(sys.argv[1]), mtl_file), 'r')

        curmat = None
        for line in fmtl.readlines():
            data = line.split()
            if (len(data) == 0):
                continue
            #start new material
            if (data[0] == "newmtl"):
                if curmat is not None:  #save previous material if exists
                    print("adding material", curmat.name)
                    materials.append(curmat)
                curmat = Material()
                curmat.name = data[1]
                
            #diffuse color
            elif (curmat and data[0] == "Kd"):
                curmat.setcol(data[1], data[2], data[3])
                
            #texture
            elif (curmat and data[0] == "map_Kd"):
                texpath = data[1]
                textures.append(texpath)
                print("found texture", texpath)
                curmat.texture = texpath
                curmat.texid = textures.index(texpath)
        
        #append the last material if it exists
        if (curmat):
            print("adding material", curmat.name)
            materials.append(curmat)
            
        fmtl.close()

    #read the obj file
    curmat = None
    for line in fin.readlines():
        data = line.split()
        if (len(data) == 0):
            continue
        #vertices
        if (data[0] == "v"):
            v = GTEVector16()
            #todo should be fixed point, 4096 instead of 32
            v.x = int(float(data[1])*32)
            v.y = -int(float(data[2])*32)
            v.z = int(float(data[3])*32)
            vertices.append(v)

        #uv texture data
        if (data[0] == "vt"):
            u = float(data[1])
            v = 1.0 - float(data[2])
            uvs.append([u, v])

        #read faces 
        #change material if found
        if (data[0] == "usemtl"):
            curmat = next((m for m in materials if m.name == data[1]), None)
            print("using material", data[1])

        #faces 
        if (data[0] == "f"):
            f = Face()
            matches = re.findall(r'(\d+)(?:/(\d*)/?(\d*))?\s*', line)
            #indices
            vert_indices = [int(v) - 1 for v, vt, vn in matches if v != '']
            f.indices[:] = reorder_z_shape(vert_indices)

            if (curmat is not None): 
                f.color = curmat.color
                if (not curmat.texid < 0): #face is textured
                    tex = open(os.path.join(sys.argv[3], os.path.splitext(curmat.texture)[0] + ".xtex"), 'rb')
                    data = tex.read(ctypes.sizeof(TexHeader))
                    texheader = TexHeader.from_buffer_copy(data)
                    width, height = texheader.texinfo.w, texheader.texinfo.h
                    width -= 1
                    height -= 1
                    tex.close()
                    
                    #uv indices
                    uv_indices = [int(vt) - 1 for v, vt, vn in matches if vt != '']
                    uv_indices = reorder_z_shape(uv_indices)

                    f.u[:] = [int(uvs[i][0]*width) for i in uv_indices]
                    f.v[:] = [int(uvs[i][1]*height) for i in uv_indices]
                    f.texid = curmat.texid
            
            faces.append(f)

    fin.close()

    #make model file
    fout = open(sys.argv[2], 'wb')

    header = ModelFileHeader()
    #magic number
    header.magic = int.from_bytes(b"XMDL", byteorder="little")
    header.numvertices = len(vertices)
    header.numfaces    = len(faces)
    header.numtex      = len(textures)
    fout.write(header)

    print("number of vertices ", len(vertices))
    print("number of faces ", len(faces))
    #write verts
    for i in range(header.numvertices):
        fout.write(vertices[i])

    #write faces
    for i in range(header.numfaces):
        fout.write(faces[i])

    #write textures
    if (header.numtex > 0): #check if any textures exist
        for mat in materials:
            if (mat.texid < 0):
                continue
            #read converted texture
            texpath = mat.texture
            texname = os.path.splitext(os.path.basename(texpath))[0] + ".xtex"  #just the file name with .xtex
            texpath = os.path.join(sys.argv[3], texname)
        
            print("writing texture", texpath)
            ftex = open(texpath, 'rb')
            texdata = ftex.read()
            fout.write(texdata)
            ftex.close()
    fout.close()