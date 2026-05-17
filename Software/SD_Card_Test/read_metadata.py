import struct
from pathlib import Path

META_FILE = "SD_CARD_TEST/wod_metadata.bin"

# Matches:
# typedef struct __attribute__((packed))
# {
#     uint32_t magic;
#     uint64_t write_seq;
#     uint64_t downlink_seq;
#     uint32_t max_records;
#     uint32_t record_size;
# } CircularBuffer_Metadata_t;

META_FORMAT = "<IQQII"
META_SIZE = struct.calcsize(META_FORMAT)

def magic_to_ascii(magic: int) -> str:
    return magic.to_bytes(4, "big", signed=False).decode("ascii", errors="replace")

def main():
    path = Path(META_FILE)

    if not path.exists():
        print(f"Metadata file not found: {path}")
        return

    data = path.read_bytes()

    if len(data) < META_SIZE:
        print(f"Metadata file too small: {len(data)} bytes, expected {META_SIZE}")
        return

    magic, write_seq, downlink_seq, max_records, record_size = struct.unpack(
        META_FORMAT,
        data[:META_SIZE]
    )

    print(f"Metadata file: {path}")
    print(f"Struct size:   {META_SIZE} bytes")
    print(f"Magic:         0x{magic:08X} ({magic_to_ascii(magic)})")
    print(f"write_seq:     {write_seq}")
    print(f"downlink_seq:  {downlink_seq}")
    print(f"max_records:   {max_records}")
    print(f"record_size:   {record_size}")

    if downlink_seq > write_seq:
        print("WARNING: downlink_seq > write_seq")

    unsent = write_seq - downlink_seq
    print(f"unsent records: {unsent}")

if __name__ == "__main__":
    main()