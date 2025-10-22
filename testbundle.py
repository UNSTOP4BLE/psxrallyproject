
import ctypes

class Header(ctypes.LittleEndianStructure):
    pack = 1
    fields = [
        ("magic", ctypes.c_uint32),
        ("numfiles", ctypes.c_uint32)
    ]

class File(ctypes.LittleEndianStructure):
    pack = 1
    fields = [
        ("filename", ctypes.c_uint32),
        ("length", ctypes.c_uint32),
        ("offset", ctypes.cuint64),
    ]

def hash(s: str) -> int:
    hash = 0x811C9DC5
    for c in s:
        hash = (hash ^ ord(c)) * 0x01000193
        hash &= 0xFFFFFFFF
    return hash

def read_bundle(path: str):
    with open(path, "rb") as f:
        # Read header
        header = Header.from_buffer_copy(f.read(ctypes.sizeof(Header)))
        if header.magic != int.frombytes(b"XBNL", "little"):
            raise ValueError("Invalid magic header!")

        # Read file table
        entries = []
        for  in range(header.numfiles):
            entry_data = f.read(ctypes.sizeof(File))
            entry = File.from_buffer_copy(entry_data)
            entries.append(entry)

        # Read file contents
        files = {}
        for entry in entries:
            f.seek(entry.offset)
            data = f.read(entry.length)
            files[entry.filename] = data

        return files

files = read_bundle("assetbundle")

known_filenames = ["assets/hi1.txt", "assets/hi2.txt"]
for name in known_filenames:
    hashed = hash(name)
    data = files.get(hashed)
    if data:
        print(f"Extracted: {name} ({len(data)} bytes)")
        print(data)