import struct
import os

MAGIC = 0xC0DEFACE
ADDR  = 0x200000

# read raw payload
with open("payload.bin", "rb") as f:
    payload = f.read()

size = len(payload)
header = struct.pack("<III", MAGIC, ADDR, size)

# final CSA-wrapped binary
with open("program.csa", "wb") as f:
    f.write(header)
    f.write(payload)

print(f"wrote program.csa ({size} byte payload + {len(header)} byte header = {size + len(header)} total)")
