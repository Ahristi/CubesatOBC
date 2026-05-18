# test_wod_downlink.py
# pip install pyserial
#
# Usage:
#   python test_wod_downlink.py COM7
#   python test_wod_downlink.py /dev/ttyUSB0 --baud 3000000

import argparse
import struct
import time
import serial


# ---------------- UART protocol ----------------
UART_SOF = 0x64          # Change this if UART_SOF is different in uart.h
COMMS_BAUDRATE = 3000000


# ---------------- COMMS message IDs ----------------
WOD_INFO_ID     = 0x66
WOD_REQUEST_ID  = 0x67
COMMS_ACK_ID    = 0x68
WOD_RECORD_ID   = 0x69
END_TRANSFER_ID = 0x70

WOD_INFO_BYTES = 9


def crc16_ccitt(data: bytes) -> int:
    """
    Matches your C function:
        init = 0xFFFF
        poly = 0x1021
        no reflection
        no final xor
    """
    crc = 0xFFFF

    for b in data:
        crc ^= b << 8

        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF

    return crc


def build_packet(msg_id: int, payload: bytes) -> bytes:
    if len(payload) > 255:
        raise ValueError("Payload too large for 1-byte UART length field")

    header = bytes([
        UART_SOF,
        msg_id & 0xFF,
        len(payload) & 0xFF,
    ])

    crc = crc16_ccitt(header + payload)

    # C code sends CRC little-endian:
    # data[crc_offset]     = msg->crc & 0xFF;
    # data[crc_offset + 1] = msg->crc >> 8;
    return header + payload + struct.pack("<H", crc)


def read_exact(ser: serial.Serial, n: int) -> bytes:
    data = ser.read(n)

    if len(data) != n:
        raise TimeoutError(f"Expected {n} bytes, received {len(data)}")

    return data


def read_packet(ser: serial.Serial):
    """
    Packet format:
        [SOF][ID][LENGTH][PAYLOAD...][CRC_L][CRC_H]
    """

    while True:
        b = ser.read(1)

        if not b:
            raise TimeoutError("Timeout waiting for UART_SOF")

        if b[0] == UART_SOF:
            break

    msg_id = read_exact(ser, 1)[0]
    length = read_exact(ser, 1)[0]

    if length == 0:
        raise ValueError("Received packet with zero payload length")

    payload = read_exact(ser, length)
    rx_crc = struct.unpack("<H", read_exact(ser, 2))[0]

    calc_crc = crc16_ccitt(bytes([UART_SOF, msg_id, length]) + payload)

    if rx_crc != calc_crc:
        raise ValueError(
            f"CRC mismatch for msg 0x{msg_id:02X}: "
            f"rx=0x{rx_crc:04X}, calc=0x{calc_crc:04X}"
        )

    return msg_id, payload


def send_wod_request(ser: serial.Serial):
    # Your UART_receive() rejects zero-length messages,
    # so send the request ID as a 1-byte payload too.
    payload = bytes([WOD_REQUEST_ID])
    pkt = build_packet(WOD_REQUEST_ID, payload)

    ser.write(pkt)
    ser.flush()

    print("TX WOD_REQUEST")


def send_ack(ser: serial.Serial):
    # Your UART_receive() rejects zero-length messages,
    # so send the ACK ID as a 1-byte payload too.
    payload = bytes([COMMS_ACK_ID])
    pkt = build_packet(COMMS_ACK_ID, payload)

    ser.write(pkt)
    ser.flush()

    print("TX ACK")


def parse_wod_info(payload: bytes):
    """
    WOD info payload:
        byte 0      = file ID
        bytes 1..4 = chunk size / record size, uint32 little-endian
        bytes 5..8 = number of chunks, uint32 little-endian
    """

    if len(payload) != WOD_INFO_BYTES:
        raise ValueError(
            f"Expected {WOD_INFO_BYTES} WOD info bytes, got {len(payload)}"
        )

    file_id = payload[0]
    chunk_size = struct.unpack_from("<I", payload, 1)[0]
    num_chunks = struct.unpack_from("<I", payload, 5)[0]

    return file_id, chunk_size, num_chunks

def main():
    PORT = "COM8"
    BAUD = 3000000
    TIMEOUT = 1
    OUT_FILE = "wod_downlink.bin"

    with serial.Serial(PORT, BAUD, timeout=TIMEOUT) as ser:
        time.sleep(0.2)

        ser.reset_input_buffer()
        ser.reset_output_buffer()

        send_wod_request(ser)

        expected_records = None
        record_size = None
        records_received = 0

        with open(OUT_FILE, "wb") as f:
            while True:
                msg_id, payload = read_packet(ser)

                if msg_id == WOD_INFO_ID:
                    file_id, record_size, expected_records = parse_wod_info(payload)

                    print("\nRX WOD_INFO")
                    print(f"  file_id          = 0x{file_id:02X}")
                    print(f"  record_size      = {record_size} bytes")
                    print(f"  expected_records = {expected_records}")

                    send_ack(ser)

                    if expected_records == 0:
                        print("No WOD records available.")

                elif msg_id == WOD_RECORD_ID:
                    records_received += 1
                    f.write(payload)

                    print(
                        f"RX WOD_RECORD {records_received}"
                        + (
                            f"/{expected_records}"
                            if expected_records is not None
                            else ""
                        )
                        + f" ({len(payload)} bytes)"
                    )

                    if record_size is not None and len(payload) != record_size:
                        print(
                            f"WARNING: expected record size {record_size}, "
                            f"got {len(payload)}"
                        )

                    send_ack(ser)

                elif msg_id == END_TRANSFER_ID:
                    print("\nRX END_TRANSFER")
                    break

                else:
                    print(f"RX unknown packet: id=0x{msg_id:02X}, len={len(payload)}")

        print()
        print(f"Saved {records_received} WOD records to {OUT_FILE}")

        if expected_records is not None and records_received != expected_records:
            print(
                f"WARNING: expected {expected_records} records, "
                f"but received {records_received}"
            )


if __name__ == "__main__":
    main()