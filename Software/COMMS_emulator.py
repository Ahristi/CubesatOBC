import serial
import struct
import argparse


PORT = "COM8"
BAUD = 3000000
BEACON_TIME_STRING_BYTES = 32   
CUBESAT_IDENTIFIER_BYTES = 6    
UART_SOF = 0x64     
BEACON_MSG_ID = 0x65        


RX_HEADER_BYTES = 3
RX_CRC_BYTES = 2

BEACON_TIME_STRING_BYTES = 32
CUBESAT_IDENTIFIER_BYTES = 6


FIELD_DEFS = [
    ("rail_3v3_voltage", "H"),
    ("rail_3v3_current_ch1", "H"),
    ("rail_3v3_current_ch2", "H"),

    ("rail_5v_voltage", "H"),
    ("rail_5v_current_ch1", "H"),
    ("rail_5v_current_ch2", "H"),

    ("rail_6v_voltage", "H"),
    ("rail_6v_current_ch1", "H"),
    ("rail_6v_current_ch2", "H"),

    ("rail_12v_voltage", "H"),
    ("rail_12v_current_ch1", "H"),
    ("rail_12v_current_ch2", "H"),

    ("battery_voltage", "H"),
    ("sys_voltage", "H"),
    ("battery_current", "H"),
    ("battery_temp", "B"),
    ("mcu_temp", "H"),
    ("charger_die_temp", "B"),

    ("mppt1_voltage", "H"),
    ("mppt2_voltage", "H"),
    ("mppt1_current", "H"),
    ("mppt2_current", "H"),

    ("eFuse_states", "B"),
    ("eFuse_faults", "B"),

    ("roll", "H"),
    ("pitch", "H"),
    ("yaw", "H"),

    ("x_rw_speed", "H"),
    ("y_rw_speed", "H"),
    ("z_rw_speed", "H"),

    ("x_mag_current", "H"),
    ("y_mag_current", "H"),
    ("z_mag_current", "H"),

    ("EPS_Faults", "H"),
    ("OBC_Faults", "H"),
    ("ADCS_Faults", "H"),
    ("Payload_Faults", "H"),
    ("Comms_Faults", "H"),
]


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


def read_exact(ser: serial.Serial, n: int) -> bytes:
    data = ser.read(n)

    if len(data) != n:
        raise TimeoutError(f"Expected {n} bytes, got {len(data)}")

    return data


def read_packet(ser: serial.Serial):
    while True:
        b = ser.read(1)

        if not b:
            continue

        if b[0] == UART_SOF:
            break

    packet_id = read_exact(ser, 1)[0]
    length = read_exact(ser, 1)[0]

    payload = read_exact(ser, length)
    crc_bytes = read_exact(ser, 2)

    received_crc = crc_bytes[0] | (crc_bytes[1] << 8)

    header = bytes([UART_SOF, packet_id, length])
    calculated_crc = crc16_ccitt(header + payload)

    if received_crc != calculated_crc:
        raise ValueError(
            f"CRC mismatch: received 0x{received_crc:04X}, "
            f"calculated 0x{calculated_crc:04X}"
        )

    return packet_id, payload


def decode_beacon(payload: bytes) -> dict:
    expected_payload_len = (
        BEACON_TIME_STRING_BYTES
        + CUBESAT_IDENTIFIER_BYTES
        + struct.calcsize("<" + "".join(fmt for _, fmt in FIELD_DEFS))
    )

    if len(payload) != expected_payload_len:
        raise ValueError(
            f"Unexpected beacon payload length: got {len(payload)}, "
            f"expected {expected_payload_len}"
        )

    offset = 0

    utc_raw = payload[offset:offset + BEACON_TIME_STRING_BYTES]
    offset += BEACON_TIME_STRING_BYTES

    identifier_raw = payload[offset:offset + CUBESAT_IDENTIFIER_BYTES]
    offset += CUBESAT_IDENTIFIER_BYTES

    utc_time = utc_raw.split(b"\x00", 1)[0].decode("ascii", errors="replace")
    identifier = identifier_raw.split(b"\x00", 1)[0].decode("ascii", errors="replace")

    fmt = "<" + "".join(fmt for _, fmt in FIELD_DEFS)
    values = struct.unpack_from(fmt, payload, offset)

    decoded = {
        "utc_time": utc_time,
        "identifier": identifier,
    }

    for (name, _), value in zip(FIELD_DEFS, values):
        decoded[name] = value

    return decoded


def print_beacon(beacon: dict):
    print("\n========== BEACON ==========")
    print(f"UTC Time:   {beacon['utc_time']}")
    print(f"Identifier: {beacon['identifier']}")

    print("\n--- EPS Rails ---")
    print(f"3V3:  V={beacon['rail_3v3_voltage']}  I1={beacon['rail_3v3_current_ch1']}  I2={beacon['rail_3v3_current_ch2']}")
    print(f"5V:   V={beacon['rail_5v_voltage']}   I1={beacon['rail_5v_current_ch1']}   I2={beacon['rail_5v_current_ch2']}")
    print(f"6V:   V={beacon['rail_6v_voltage']}   I1={beacon['rail_6v_current_ch1']}   I2={beacon['rail_6v_current_ch2']}")
    print(f"12V:  V={beacon['rail_12v_voltage']}  I1={beacon['rail_12v_current_ch1']}  I2={beacon['rail_12v_current_ch2']}")

    print("\n--- Battery / System ---")
    print(f"Battery voltage:   {beacon['battery_voltage']}")
    print(f"System voltage:    {beacon['sys_voltage']}")
    print(f"Battery current:   {beacon['battery_current']}")
    print(f"Battery temp:      {beacon['battery_temp']}")
    print(f"MCU temp:          {beacon['mcu_temp']}")
    print(f"Charger die temp:  {beacon['charger_die_temp']}")

    print("\n--- MPPT ---")
    print(f"MPPT1: V={beacon['mppt1_voltage']}  I={beacon['mppt1_current']}")
    print(f"MPPT2: V={beacon['mppt2_voltage']}  I={beacon['mppt2_current']}")

    print("\n--- eFuses ---")
    print(f"eFuse states: 0x{beacon['eFuse_states']:02X}")
    print(f"eFuse faults: 0x{beacon['eFuse_faults']:02X}")

    print("\n--- ADCS ---")
    print(f"Roll:  {beacon['roll']}")
    print(f"Pitch: {beacon['pitch']}")
    print(f"Yaw:   {beacon['yaw']}")
    print(f"RW speeds:     X={beacon['x_rw_speed']}  Y={beacon['y_rw_speed']}  Z={beacon['z_rw_speed']}")
    print(f"Mag currents:  X={beacon['x_mag_current']}  Y={beacon['y_mag_current']}  Z={beacon['z_mag_current']}")

    print("\n--- Faults ---")
    print(f"EPS faults:     0x{beacon['EPS_Faults']:04X}")
    print(f"OBC faults:     0x{beacon['OBC_Faults']:04X}")
    print(f"ADCS faults:    0x{beacon['ADCS_Faults']:04X}")
    print(f"Payload faults: 0x{beacon['Payload_Faults']:04X}")
    print(f"Comms faults:   0x{beacon['Comms_Faults']:04X}")
    print("============================\n")


def main():
    port = "COM8"
    baud = 3000000

    expected_payload_len = (
        BEACON_TIME_STRING_BYTES
        + CUBESAT_IDENTIFIER_BYTES
        + struct.calcsize("<" + "".join(fmt for _, fmt in FIELD_DEFS))
    )

    print(f"Expected beacon payload length: {expected_payload_len} bytes")
    print(f"Opening {port} at {baud} baud")

    with serial.Serial(port, baud, timeout=1) as ser:
        while True:
            try:
                packet_id, payload = read_packet(ser)

                if packet_id != BEACON_MSG_ID:
                    print(f"Ignoring packet ID 0x{packet_id:02X}, length {len(payload)}")
                    continue

                beacon = decode_beacon(payload)
                print_beacon(beacon)

            except TimeoutError:
                continue

            except Exception as e:
                print(f"Packet error: {e}")


if __name__ == "__main__":
    main()