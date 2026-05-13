import argparse
import math
import struct
import time
import serial

SYNC = b"\x64"

# Commands: OBC -> ADCS
PKT_ATTITUDE_COMMAND = 0x13   # 6 floats
PKT_ORBIT_UPDATE     = 0x11   # 7 floats

# Telemetry: ADCS -> OBC
PKT_ADCS_REPORT      = 0x80   # 13 floats
PKT_ACK              = 0xF0
PKT_ERROR            = 0xFF

REPORT_RATE_HZ = 1


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


def build_packet(packet_type: int, payload: bytes = b"") -> bytes:
    if len(payload) > 255:
        raise ValueError("Payload too long for uint8_t length field")

    header = SYNC + bytes([packet_type, len(payload)])
    crc = crc16_ccitt(header + payload)

    return header + payload + struct.pack("<H", crc)


def send_packet(ser: serial.Serial, packet_type: int, payload: bytes = b""):
    ser.write(build_packet(packet_type, payload))


def send_ack(ser: serial.Serial, received_packet_id: int):
    send_packet(ser, PKT_ACK, bytes([received_packet_id]))


def send_error(ser: serial.Serial, error_code: int):
    send_packet(ser, PKT_ERROR, bytes([error_code]))


def make_attitude_report(t: float) -> bytes:
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
        it1, it2, it3,
        detumble_scale
    )


def read_packet(ser: serial.Serial):
    """
    Packet format:

        [SOF][ID][LEN][PAYLOAD...][CRC_L][CRC_H]

    Returns:
        (packet_id, payload) if valid
        None if no complete packet available
    """

    # Wait for SOF
    sof = ser.read(1)

    if not sof:
        return None

    if sof != SYNC:
        return None

    header_rest = ser.read(2)

    if len(header_rest) != 2:
        return None

    packet_id = header_rest[0]
    length = header_rest[1]

    payload = ser.read(length)

    if len(payload) != length:
        return None

    crc_bytes = ser.read(2)

    if len(crc_bytes) != 2:
        return None

    received_crc = struct.unpack("<H", crc_bytes)[0]

    crc_data = SYNC + bytes([packet_id, length]) + payload
    calculated_crc = crc16_ccitt(crc_data)

    if received_crc != calculated_crc:
        print(
            f"CRC error on packet 0x{packet_id:02X}: "
            f"received 0x{received_crc:04X}, expected 0x{calculated_crc:04X}"
        )
        return ("CRC_ERROR", packet_id)

    return packet_id, payload


def handle_packet(ser: serial.Serial, packet_id: int, payload: bytes):
    if packet_id == PKT_ATTITUDE_COMMAND:
        expected_len = 6 * 4

        if len(payload) != expected_len:
            print(f"Invalid attitude command length: {len(payload)}")
            send_error(ser, packet_id)
            return

        attitude_cmd = struct.unpack("<6f", payload)

        theta_cmd = attitude_cmd[0]
        phi_cmd   = attitude_cmd[1]
        psi_cmd   = attitude_cmd[2]

        wx_cmd = attitude_cmd[3]
        wy_cmd = attitude_cmd[4]
        wz_cmd = attitude_cmd[5]

        print("Received attitude command:")
        print(f"  theta = {theta_cmd:.3f}")
        print(f"  phi   = {phi_cmd:.3f}")
        print(f"  psi   = {psi_cmd:.3f}")
        print(f"  wx    = {wx_cmd:.6f}")
        print(f"  wy    = {wy_cmd:.6f}")
        print(f"  wz    = {wz_cmd:.6f}")

        send_ack(ser, packet_id)

    elif packet_id == PKT_ORBIT_UPDATE:
        expected_len = 7 * 4

        if len(payload) != expected_len:
            print(f"Invalid orbit update length: {len(payload)}")
            send_error(ser, packet_id)
            return

        orbit = struct.unpack("<7f", payload)

        print("Received orbit update:")
        for i, value in enumerate(orbit):
            print(f"  orbit[{i}] = {value:.6f}")

        send_ack(ser, packet_id)

    else:
        print(f"Unknown packet ID: 0x{packet_id:02X}")
        send_error(ser, packet_id)


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
            # Receive commands from OBC
            packet = read_packet(ser)

            if packet is not None:
                if packet[0] == "CRC_ERROR":
                    bad_packet_id = packet[1]
                    send_error(ser, bad_packet_id)
                else:
                    packet_id, payload = packet
                    handle_packet(ser, packet_id, payload)

            # Periodically send fake ADCS telemetry
            now = time.time()

            if now - last_report >= 1.0 / REPORT_RATE_HZ:
                payload = make_attitude_report(now - t0)
                send_packet(ser, PKT_ADCS_REPORT, payload)
                print("Sent attitude report")
                last_report = now


if __name__ == "__main__":
    main()