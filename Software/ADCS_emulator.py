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

SYNC = b"\x64"

#Commands
PKT_ATTITUDE_COMMAND = 0x10   # OBC -> ADCS, 6 floats
PKT_ORBIT_UPDATE     = 0x11   # OBC -> ADCS, 7 floats

#Telemetry
PKT_ADCS_REPORT      = 0x80   # ADCS -> OBC, 12 floats
PKT_ACK              = 0xF0
PKT_ERROR            = 0xFF

REPORT_RATE_HZ = 1


def crc16_ccitt(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if (crc & 0x8000) else (crc << 1) & 0xFFFF
    return crc

def build_packet(packet_type: int, payload: bytes = b"") -> bytes:
    if len(payload) > 255:
        raise ValueError("Payload too long for uint8_t length field")
    header = SYNC + bytes([packet_type, len(payload)])
    crc = crc16_ccitt(header + payload)
    return header + payload + struct.pack("<H", crc)

def send_packet(ser: serial.Serial, packet_type: int, payload: bytes = b""):
    ser.write(build_packet(packet_type, payload))


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

    detumble_scale = 0.5

    return struct.pack(
        "<13f",
        theta, phi, psi,
        wx, wy, wz,
        rw1, rw2, rw3,
        it1, it2, it3, detumble_scale
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "port",
        nargs="?",
        default="COM8",
        help="FTDI serial port, e.g. COM8 or /dev/ttyUSB0"
    )
    parser.add_argument("--baud", type=int, default=3000000)
    args = parser.parse_args()

    with serial.Serial(args.port, args.baud, timeout=0.02) as ser:
        print(f"ADCS emulator running on {args.port} @ {args.baud} baud")

        last_report = 0.0
        t0 = time.time()

        while True:
            now = time.time()
            if now - last_report >= 1.0 / REPORT_RATE_HZ:
                payload = make_attitude_report(now - t0)
                send_packet(ser, PKT_ADCS_REPORT, payload)
                print("Sent attitude report")
                last_report = now


if __name__ == "__main__":
    main()