# test_wod_downlink.py
# pip install pyserial

import struct
import sys
import time
from dataclasses import dataclass

import serial


# ---------------- User settings ----------------

PORT = "COM8"
BAUD = 3_000_000
TIMEOUT = 5.0
OUT_FILE = "wod_downlink.bin"
VERBOSE = True


# ---------------- UART protocol ----------------

UART_SOF = 0x64
UART_MAX_PAYLOAD = 255


# ---------------- COMMS message IDs ----------------

WOD_INFO_ID     = 0x66
WOD_REQUEST_ID  = 0x67
COMMS_ACK_ID    = 0x68
WOD_RECORD_ID   = 0x69
END_TRANSFER_ID = 0x70

WOD_INFO_BYTES = 9


@dataclass
class Packet:
    msg_id: int
    payload: bytes
    crc: int


@dataclass
class WodInfo:
    file_id: int
    record_size: int
    expected_records: int


def crc16_ccitt(data: bytes) -> int:
    crc = 0xFFFF

    for b in data:
        crc ^= b << 8

        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF

    return crc


def build_packet(msg_id: int, payload: bytes = b"") -> bytes:
    if len(payload) > UART_MAX_PAYLOAD:
        raise ValueError(f"Payload too large: {len(payload)} bytes")

    header = bytes([
        UART_SOF,
        msg_id & 0xFF,
        len(payload) & 0xFF,
    ])

    crc = crc16_ccitt(header + payload)

    return header + payload + struct.pack("<H", crc)


def read_exact(ser: serial.Serial, n: int) -> bytes:
    data = ser.read(n)

    if len(data) != n:
        raise TimeoutError(f"Expected {n} bytes, got {len(data)}")

    return data


def read_packet(ser: serial.Serial) -> Packet:
    while True:
        b = ser.read(1)

        if not b:
            raise TimeoutError("Timeout waiting for SOF")

        if b[0] == UART_SOF:
            break

        if VERBOSE:
            print(f"Skipping byte while searching for SOF: 0x{b[0]:02X}")

    msg_id = read_exact(ser, 1)[0]
    length = read_exact(ser, 1)[0]

    payload = read_exact(ser, length) if length > 0 else b""
    rx_crc = struct.unpack("<H", read_exact(ser, 2))[0]

    calc_crc = crc16_ccitt(bytes([UART_SOF, msg_id, length]) + payload)

    if rx_crc != calc_crc:
        raise ValueError(
            f"CRC mismatch: id=0x{msg_id:02X}, len={length}, "
            f"rx=0x{rx_crc:04X}, calc=0x{calc_crc:04X}"
        )

    return Packet(msg_id=msg_id, payload=payload, crc=rx_crc)


def read_packet_resync(ser: serial.Serial) -> Packet:
    crc_errors = 0
    max_crc_errors = 20

    while True:
        try:
            return read_packet(ser)

        except ValueError as e:
            crc_errors += 1
            print(f"WARNING: {e}")

            if crc_errors >= max_crc_errors:
                raise RuntimeError("Too many CRC errors while resynchronising")

            continue


def send_wod_request(ser: serial.Serial) -> None:
    payload = bytes([WOD_REQUEST_ID])
    packet = build_packet(WOD_REQUEST_ID, payload)

    ser.write(packet)
    ser.flush()

    print("TX WOD_REQUEST")


def send_ack(ser: serial.Serial) -> None:
    payload = bytes([COMMS_ACK_ID])
    packet = build_packet(COMMS_ACK_ID, payload)

    ser.write(packet)
    ser.flush()

    if VERBOSE:
        print("TX ACK")


def parse_wod_info(payload: bytes) -> WodInfo:
    if len(payload) != WOD_INFO_BYTES:
        raise ValueError(
            f"Invalid WOD_INFO length: expected {WOD_INFO_BYTES}, got {len(payload)}"
        )

    file_id = payload[0]
    record_size = struct.unpack_from("<I", payload, 1)[0]
    expected_records = struct.unpack_from("<I", payload, 5)[0]

    return WodInfo(
        file_id=file_id,
        record_size=record_size,
        expected_records=expected_records,
    )


def main() -> int:
    records_received = 0
    bytes_received = 0
    wod_info = None

    print(f"Opening {PORT} @ {BAUD} baud")

    with serial.Serial(PORT, BAUD, timeout=TIMEOUT) as ser:
        time.sleep(0.2)

        ser.reset_input_buffer()
        ser.reset_output_buffer()

        send_wod_request(ser)

        with open(OUT_FILE, "wb") as f:
            while True:
                packet = read_packet_resync(ser)

                if VERBOSE:
                    print(
                        f"RX packet: id=0x{packet.msg_id:02X}, "
                        f"len={len(packet.payload)}, "
                        f"crc=0x{packet.crc:04X}"
                    )

                if packet.msg_id == WOD_INFO_ID:
                    wod_info = parse_wod_info(packet.payload)

                    print()
                    print("RX WOD_INFO")
                    print(f"  file_id          = 0x{wod_info.file_id:02X}")
                    print(f"  record_size      = {wod_info.record_size} bytes")
                    print(f"  expected_records = {wod_info.expected_records}")

                    send_ack(ser)

                elif packet.msg_id == WOD_RECORD_ID:
                    records_received += 1
                    bytes_received += len(packet.payload)

                    f.write(packet.payload)

                    if VERBOSE:
                        if wod_info is not None:
                            print(
                                f"RX WOD_RECORD {records_received}/"
                                f"{wod_info.expected_records}: "
                                f"{len(packet.payload)} bytes"
                            )
                        else:
                            print(
                                f"RX WOD_RECORD {records_received}: "
                                f"{len(packet.payload)} bytes"
                            )

                    if wod_info is not None:
                        if len(packet.payload) != wod_info.record_size:
                            print(
                                f"WARNING: record {records_received} has "
                                f"{len(packet.payload)} bytes, expected "
                                f"{wod_info.record_size}"
                            )

                    send_ack(ser)

                elif packet.msg_id == END_TRANSFER_ID:
                    print()
                    print("RX END_TRANSFER")
                    break

                else:
                    print(
                        f"WARNING: unknown packet id=0x{packet.msg_id:02X}, "
                        f"len={len(packet.payload)}"
                    )

    print()
    print(f"Saved output to {OUT_FILE}")
    print(f"Records received: {records_received}")
    print(f"Bytes received:   {bytes_received}")

    if wod_info is not None:
        if records_received != wod_info.expected_records:
            print(
                f"WARNING: expected {wod_info.expected_records} records, "
                f"but received {records_received}"
            )
            return 2

        expected_bytes = wod_info.expected_records * wod_info.record_size

        if bytes_received != expected_bytes:
            print(
                f"WARNING: expected {expected_bytes} bytes, "
                f"but received {bytes_received}"
            )
            return 3

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())

    except serial.SerialException as e:
        print(f"Serial error: {e}", file=sys.stderr)
        raise SystemExit(10)

    except TimeoutError as e:
        print(f"Timeout: {e}", file=sys.stderr)
        raise SystemExit(11)

    except KeyboardInterrupt:
        print("\nInterrupted.")
        raise SystemExit(130)

    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        raise SystemExit(1)