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
        return [indices[0], indices[1], indices[2], indices[3]]
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
    
    def convert_model(input_path, output_path, texture_dir):
    materials = []
    vertices = []
    uvs = []
    faces = []
    textures = []

    # open obj
    with open(input_path, 'r') as fin:
        content = fin.read()

    # find mtl
    match = re.search(r"mtllib\s+(.+\.mtl)", content)
    mtl_file = match.group(1) if match else None
    print("material file:", mtl_file)

    # parse MTL
    if mtl_file is not None:
        mtl_path = os.path.join(os.path.dirname(input_path), mtl_file)
        with open(mtl_path, 'r') as fmtl:
            curmat = None
            for line in fmtl:
                data = line.split()
                if not data:
                    continue

                if data[0] == "newmtl":
                    if curmat is not None:
                        materials.append(curmat)
                    curmat = Material()
                    curmat.name = data[1]

                elif curmat and data[0] == "Kd":
                    curmat.setcol(data[1], data[2], data[3])

                elif curmat and data[0] == "map_Kd":
                    texpath = data[1]
                    textures.append(texpath)
                    curmat.texture = texpath
                    curmat.texid = textures.index(texpath)

            if curmat:
                materials.append(curmat)

    # parse OBJ
    curmat = None
    with open(input_path, 'r') as fin:
        for line in fin:
            data = line.split()
            if not data:
                continue

            if data[0] == "v":
                v = GTEVector16()
                v.x = int(float(data[1]) * 32)
                v.y = int(float(data[2]) * 32)
                v.z = int(float(data[3]) * 32)
                vertices.append(v)

            elif data[0] == "vt":
                u = float(data[1])
                v = 1.0 - float(data[2])
                uvs.append([u, v])

            elif data[0] == "usemtl":
                curmat = next((m for m in materials if m.name == data[1]), None)

            elif data[0] == "f":
                f = Face()
                matches = re.findall(r'(\d+)(?:/(\d*)/?(\d*))?\s*', line)

                vert_indices = [int(v) - 1 for v, vt, vn in matches if v]
                f.indices[:] = reorder_z_shape(vert_indices)

                if curmat is not None:
                    f.color = curmat.color

                    if curmat.texid >= 0:
                        texname = os.path.splitext(curmat.texture)[0] + ".xtex"
                        texpath = os.path.join(texture_dir, texname)

                        with open(texpath, 'rb') as tex:
                            data = tex.read(ctypes.sizeof(TexHeader))
                            texheader = TexHeader.from_buffer_copy(data)

                        width = texheader.texinfo.w - 1
                        height = texheader.texinfo.h - 1

                        uv_indices = [int(vt) - 1 for v, vt, vn in matches if vt]
                        uv_indices = reorder_z_shape(uv_indices)

                        f.u[:] = [int(uvs[i][0] * width) for i in uv_indices]
                        f.v[:] = [int(uvs[i][1] * height) for i in uv_indices]
                        f.texid = curmat.texid

                faces.append(f)

    # write output
    with open(output_path, 'wb') as fout:
        header = ModelFileHeader()
        header.magic = int.from_bytes(b"XMDL", byteorder="little")
        header.numvertices = len(vertices)
        header.numfaces = len(faces)
        header.numtex = len(textures)

        fout.write(header)

        for v in vertices:
            fout.write(v)

        for f in faces:
            fout.write(f)

        if header.numtex > 0:
            for mat in materials:
                if mat.texid < 0:
                    continue

                texname = os.path.splitext(os.path.basename(mat.texture))[0] + ".xtex"
                texpath = os.path.join(texture_dir, texname)

                with open(texpath, 'rb') as ftex:
                    fout.write(ftex.read())

def main():
    if len(sys.argv) != 4:
        print("Usage: python script.py <input.obj> <output.xmdl> <texture_dir>")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]
    texture_dir = sys.argv[3]

    if not os.path.isfile(input_path):
        error(f"Input file not found: {input_path}")

    if not os.path.isdir(texture_dir):
        error(f"Texture directory not found: {texture_dir}")

    try:
        convert_model(input_path, output_path, texture_dir)
        print(f"Conversion successful: {output_path}")
    except Exception as e:
        error(f"Conversion failed: {e}")


if __name__ == "__main__":
    main()