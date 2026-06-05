import serial
import struct
import time
import random


# ---------------- CONFIG ----------------

PORT = "COM6"              # Change this
BAUDRATE = 3_000_000
TIMEOUT_S = 0.5

UART_SOF = 0x64

EXPERIMENT_COMMAND_ID = 0x13
COMMS_ACK_ID = 0x68
CHUNK_ID = 0x69
END_TRANSFER_ID = 0x70
UPLINK_FILE_INFO_ID = 0x75
EXPERIMENT_FILE_ID = 0x07

NUM_RECORDS = 20

# uint16_t indx + 6 floats
RECORD_STRUCT = struct.Struct("<Hffffff")
RECORD_SIZE = RECORD_STRUCT.size

# Your COMMS_receivePacket() expects:
# payload[0:2] = packet index, big endian
# payload[2:]  = chunk data
CHUNK_SIZE = RECORD_SIZE


# ---------------- CRC ----------------

def crc16_ccitt(data: bytes, poly=0x1021, init=0xFFFF) -> int:
    crc = init

    for byte in data:
        crc ^= byte << 8

        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ poly) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF

    return crc


# ---------------- UART PACKET HELPERS ----------------

def build_uart_msg(msg_id: int, payload: bytes = b"") -> bytes:
    if len(payload) > 255:
        raise ValueError("Payload too large for uint8_t length field")

    header = bytes([
        UART_SOF,
        msg_id & 0xFF,
        len(payload) & 0xFF,
    ])

    frame_no_crc = header + payload
    crc = crc16_ccitt(frame_no_crc)

    # Assumes Arduino sends/expects CRC low byte first.
    return frame_no_crc + struct.pack("<H", crc)


def read_uart_msg(ser: serial.Serial):
    # Find SOF
    while True:
        b = ser.read(1)
        if not b:
            return None
        if b[0] == UART_SOF:
            break

    header_rest = ser.read(2)
    if len(header_rest) != 2:
        return None

    msg_id = header_rest[0]
    length = header_rest[1]

    payload = ser.read(length)
    if len(payload) != length:
        return None

    crc_bytes = ser.read(2)
    if len(crc_bytes) != 2:
        return None

    rx_crc = struct.unpack("<H", crc_bytes)[0]

    frame_no_crc = bytes([UART_SOF, msg_id, length]) + payload
    calc_crc = crc16_ccitt(frame_no_crc)

    if rx_crc != calc_crc:
        print(f"CRC mismatch: rx=0x{rx_crc:04X}, calc=0x{calc_crc:04X}")
        return None

    return {
        "sof": UART_SOF,
        "id": msg_id,
        "length": length,
        "payload": payload,
        "crc": rx_crc,
    }


def send_msg(ser: serial.Serial, msg_id: int, payload: bytes = b""):
    frame = build_uart_msg(msg_id, payload)
    ser.write(frame)
    ser.flush()


def wait_for_ack(ser: serial.Serial, label: str):
    msg = read_uart_msg(ser)

    if msg is None:
        raise TimeoutError(f"No valid ACK received after {label}")

    if msg["id"] != COMMS_ACK_ID:
        raise RuntimeError(
            f"Expected ACK after {label}, got id=0x{msg['id']:02X}, "
            f"length={msg['length']}, payload={msg['payload'].hex()}"
        )

    print(f"ACK received after {label}")


# ---------------- EXPERIMENT DATA ----------------

def make_experiment_record(index: int) -> bytes:
    pos_x = 1.0 * index
    pos_y = 2.0 * index
    pos_z = 3.0 * index

    roll = random.uniform(-180.0, 180.0)
    pitch = random.uniform(-90.0, 90.0)
    yaw = random.uniform(-180.0, 180.0)

    return RECORD_STRUCT.pack(
        index,
        pos_x,
        pos_y,
        pos_z,
        roll,
        pitch,
        yaw,
    )


def build_file_info_payload() -> bytes:
    # Your C code expects:
    # payload[0] = file ID
    # payload[1:5] = uint32_t chunk_size
    # payload[5:9] = uint32_t file_chunks
    return struct.pack(
        "<BII",
        EXPERIMENT_FILE_ID,
        CHUNK_SIZE,
        NUM_RECORDS,
    )


def build_chunk_payload(packet_idx: int, record: bytes) -> bytes:
    # Your C receivePacket() decodes packet index as big-endian:
    # packet_idx = ((uint16_t)payload[0] << 8) | payload[1]
    packet_index_bytes = struct.pack(">H", packet_idx)
    return packet_index_bytes + record


# ---------------- TEST SEQUENCE ----------------

def run_test():
    print(f"Record size: {RECORD_SIZE} bytes")
    print(f"Chunk size:  {CHUNK_SIZE} bytes")
    print(f"Records:     {NUM_RECORDS}")

    with serial.Serial(PORT, BAUDRATE, timeout=TIMEOUT_S) as ser:
        time.sleep(2.0)
        ser.reset_input_buffer()
        ser.reset_output_buffer()

        print("\n1. Requesting experiment uplink...")
        send_msg(ser, EXPERIMENT_COMMAND_ID, bytes([EXPERIMENT_COMMAND_ID]))

        # Give OBC one task cycle to enter UPLINK_RECEIVE_INFO
        time.sleep(0.1)

        print("2. Sending file info...")
        send_msg(ser, UPLINK_FILE_INFO_ID, build_file_info_payload())
        wait_for_ack(ser, "file info")

        print("3. Sending experiment records...")
        for i in range(NUM_RECORDS):
            record = make_experiment_record(i)
            chunk_payload = build_chunk_payload(i, record)

            send_msg(ser, CHUNK_ID, chunk_payload)
            wait_for_ack(ser, f"chunk {i}")

            print(f"Sent chunk {i}/{NUM_RECORDS - 1}")

        print("4. Sending END_TRANSFER...")
        send_msg(ser, END_TRANSFER_ID, bytes([END_TRANSFER_ID]))
        wait_for_ack(ser, "END_TRANSFER")

        print("\nUplink test complete.")


if __name__ == "__main__":
    run_test()