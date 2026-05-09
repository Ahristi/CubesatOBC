# adcs_emulator.py
# pip install pyserial
#
# Example:
# python adcs_emulator.py COM7 --baud 115200
# python adcs_emulator.py /dev/ttyUSB0 --baud 115200

import argparse
import math
import struct
import time
import serial

SYNC = b"\xAA\x55"

PKT_ATTITUDE_COMMAND = 0x10   # OBC -> ADCS, 6 floats
PKT_ORBIT_UPDATE     = 0x11   # OBC -> ADCS, 7 floats
PKT_ATTITUDE_REPORT  = 0x80   # ADCS -> OBC, 12 floats
PKT_ACK              = 0xF0
PKT_ERROR            = 0xFF

REPORT_RATE_HZ = 1.0


def crc16_ccitt(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if (crc & 0x8000) else (crc << 1) & 0xFFFF
    return crc


def build_packet(packet_type: int, payload: bytes = b"") -> bytes:
    header = SYNC + bytes([packet_type]) + struct.pack("<H", len(payload))
    crc = crc16_ccitt(header[2:] + payload)
    return header + payload + struct.pack("<H", crc)


def send_packet(ser: serial.Serial, packet_type: int, payload: bytes = b""):
    ser.write(build_packet(packet_type, payload))


def read_packet(ser: serial.Serial):
    # Find sync
    while True:
        b = ser.read(1)
        if not b:
            return None

        if b == SYNC[:1]:
            b2 = ser.read(1)
            if b2 == SYNC[1:2]:
                break

    header = ser.read(3)
    if len(header) != 3:
        return None

    packet_type = header[0]
    payload_len = struct.unpack("<H", header[1:3])[0]

    payload = ser.read(payload_len)
    rx_crc_bytes = ser.read(2)

    if len(payload) != payload_len or len(rx_crc_bytes) != 2:
        return None

    rx_crc = struct.unpack("<H", rx_crc_bytes)[0]
    calc_crc = crc16_ccitt(bytes([packet_type]) + struct.pack("<H", payload_len) + payload)

    if rx_crc != calc_crc:
        print(f"CRC fail: rx=0x{rx_crc:04X}, calc=0x{calc_crc:04X}")
        return None

    return packet_type, payload


def make_attitude_report(t: float) -> bytes:
    # Fake changing ADCS telemetry
    theta = 5.0 * math.sin(0.1 * t)
    phi   = 3.0 * math.sin(0.07 * t)
    psi   = 20.0 + 2.0 * math.sin(0.05 * t)

    wx = 0.01 * math.sin(0.2 * t)
    wy = 0.02 * math.sin(0.17 * t)
    wz = 0.015 * math.sin(0.13 * t)

    rw1 = 1200.0 + 10.0 * math.sin(0.1 * t)
    rw2 = 1180.0 + 10.0 * math.sin(0.12 * t)
    rw3 = 1210.0 + 10.0 * math.sin(0.08 * t)

    it1 = 0.10 * math.sin(0.15 * t)
    it2 = 0.10 * math.sin(0.11 * t)
    it3 = 0.10 * math.sin(0.09 * t)

    return struct.pack(
        "<12f",
        theta, phi, psi,
        wx, wy, wz,
        rw1, rw2, rw3,
        it1, it2, it3
    )


def handle_command(ser: serial.Serial, packet_type: int, payload: bytes):
    if packet_type == PKT_ATTITUDE_COMMAND:
        if len(payload) != 6 * 4:
            send_packet(ser, PKT_ERROR, b"bad attitude command length")
            return

        vals = struct.unpack("<6f", payload)
        theta0, phi0, psi0, wx, wy, wz = vals
        print(
            f"ATT CMD: theta0={theta0:.3f}, phi0={phi0:.3f}, psi0={psi0:.3f}, "
            f"wx={wx:.4f}, wy={wy:.4f}, wz={wz:.4f}"
        )
        send_packet(ser, PKT_ACK, bytes([PKT_ATTITUDE_COMMAND]))

    elif packet_type == PKT_ORBIT_UPDATE:
        if len(payload) != 7 * 4:
            send_packet(ser, PKT_ERROR, b"bad orbit update length")
            return

        vals = struct.unpack("<7f", payload)
        print("ORBIT UPDATE:", ", ".join(f"{v:.6g}" for v in vals))
        send_packet(ser, PKT_ACK, bytes([PKT_ORBIT_UPDATE]))

    else:
        print(f"Unknown packet type: 0x{packet_type:02X}, payload={payload.hex()}")
        send_packet(ser, PKT_ERROR, bytes([packet_type]))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("port", help="FTDI serial port, e.g. COM7 or /dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=115200)
    args = parser.parse_args()

    with serial.Serial(args.port, args.baud, timeout=0.02) as ser:
        print(f"ADCS emulator running on {args.port} @ {args.baud} baud")

        last_report = 0.0
        t0 = time.time()

        while True:
            pkt = read_packet(ser)
            if pkt is not None:
                packet_type, payload = pkt
                handle_command(ser, packet_type, payload)

            now = time.time()
            if now - last_report >= 1.0 / REPORT_RATE_HZ:
                payload = make_attitude_report(now - t0)
                send_packet(ser, PKT_ATTITUDE_REPORT, payload)
                print("Sent attitude report")
                last_report = now


if __name__ == "__main__":
    main()