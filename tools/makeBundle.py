import ctypes
import struct

class Header(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("magic", ctypes.c_uint32),
        ("numfiles", ctypes.c_uint32)
    ]

class File(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("filename", ctypes.c_uint32),
        ("length", ctypes.c_uint32),
        ("offset", ctypes.c_uint64),
    ]

def hash(s: str) -> int:
    hash_ = 0x811C9DC5
    prime = 0x01000193
    for c in s:
        hash_ = (hash_ ^ ord(c)) * prime
        hash_ &= 0xFFFFFFFF
    return hash_

filenames = ["assets/hi1.txt", "assets/hi2.txt"]
with open("assetbundle", "wb") as out:
    header = Header()
    header.magic = int.from_bytes(b"XBNL", byteorder="little")
    # Write file count
    numfiles = len(filenames)
    header.numfiles = numfiles
    out.write(bytes(header))

    # Track offset and entries
    offset = numfiles * ctypes.sizeof(File) + ctypes.sizeof(Header)
    entries = []

    # Write data and build entries
    for fn in filenames:
        with open(fn, "rb") as f:
            data = f.read()

        entry = File()
        entry.filename = hash(fn)
        entry.length = len(data)
        entry.offset = offset
        entries.append(entry)

        out.seek(offset)
        out.write(data)

        # Align
        while out.tell() % 4:
            print("padding")
            out.write(b'\x00')
            offset += 1


        offset += len(data)
            

    # Write file table
    out.seek(ctypes.sizeof(Header)) #skip header
    for entry in entries:
        out.write(bytes(entry))