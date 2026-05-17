import struct
from pathlib import Path

WOD_FILE = "SD_Card_Test/wod.bin"

# LOGGING_Record_t, little-endian, packed:
#
# uint64_t sequence;
# uint16_t year;
# uint8_t  month, day, hours, minutes, seconds;
# uint16_t x 18;
# int16_t  x 9;
# uint16_t obc_faults;

RECORD_FORMAT = "<QHBBBBB" + "H" * 19 + "h" * 9 + "H"
RECORD_SIZE = struct.calcsize(RECORD_FORMAT)

FIELDS = [
    "sequence",

    "year",
    "month",
    "day",
    "hours",
    "minutes",
    "seconds",

    "rail_3v3_voltage",
    "rail_3v3_current_ch1",
    "rail_3v3_current_ch2",

    "rail_5v_voltage",
    "rail_5v_current_ch1",
    "rail_5v_current_ch2",

    "rail_6v_voltage",
    "rail_6v_current_ch1",

    "rail_12v_voltage",
    "rail_12v_current_ch1",
    "rail_12v_current_ch2",

    "mppt1_voltage",
    "mppt1_current",
    "mppt2_voltage",
    "mppt2_current",

    "battery_voltage",
    "battery_current",
    "battery_temp",
    "mcu_temp",

    "roll",
    "pitch",
    "yaw",

    "x_rw_speed",
    "y_rw_speed",
    "z_rw_speed",

    "x_mag_current",
    "y_mag_current",
    "z_mag_current",

    "obc_faults",
]


def parse_record(raw: bytes) -> dict:
    values = struct.unpack(RECORD_FORMAT, raw)
    return dict(zip(FIELDS, values))


def main():
    path = Path(WOD_FILE)

    if not path.exists():
        print(f"File not found: {path}")
        return

    data = path.read_bytes()

    print(f"File:        {path}")
    print(f"File size:   {len(data)} bytes")
    print(f"Record size: {RECORD_SIZE} bytes")

    if len(data) % RECORD_SIZE != 0:
        print("WARNING: file size is not an integer number of records")

    num_records = len(data) // RECORD_SIZE
    print(f"Records:     {num_records}")
    print()

    for i in range(num_records):
        start = i * RECORD_SIZE
        end = start + RECORD_SIZE

        record = parse_record(data[start:end])

        print(f"Record {i}")
        print(f"  sequence: {record['sequence']}")
        print(
            f"  time:     {record['year']:04d}-"
            f"{record['month']:02d}-"
            f"{record['day']:02d} "
            f"{record['hours']:02d}:"
            f"{record['minutes']:02d}:"
            f"{record['seconds']:02d}"
        )

        print(
            f"  3V3:      V={record['rail_3v3_voltage']}, "
            f"I1={record['rail_3v3_current_ch1']}, "
            f"I2={record['rail_3v3_current_ch2']}"
        )

        print(
            f"  5V:       V={record['rail_5v_voltage']}, "
            f"I1={record['rail_5v_current_ch1']}, "
            f"I2={record['rail_5v_current_ch2']}"
        )

        print(
            f"  6V:       V={record['rail_6v_voltage']}, "
            f"I1={record['rail_6v_current_ch1']}"
        )

        print(
            f"  12V:      V={record['rail_12v_voltage']}, "
            f"I1={record['rail_12v_current_ch1']}, "
            f"I2={record['rail_12v_current_ch2']}"
        )

        print(
            f"  MPPT:     "
            f"MPPT1 V={record['mppt1_voltage']}, "
            f"MPPT1 I={record['mppt1_current']}, "
            f"MPPT2 V={record['mppt2_voltage']}, "
            f"MPPT2 I={record['mppt2_current']}"
        )

        print(
            f"  Battery:  V={record['battery_voltage']}, "
            f"I={record['battery_current']}, "
            f"T={record['battery_temp']}, "
            f"MCU T={record['mcu_temp']}"
        )

        print(
            f"  ADCS:     roll={record['roll']}, "
            f"pitch={record['pitch']}, "
            f"yaw={record['yaw']}"
        )

        print(
            f"  RW:       x={record['x_rw_speed']}, "
            f"y={record['y_rw_speed']}, "
            f"z={record['z_rw_speed']}"
        )

        print(
            f"  Mag:      x={record['x_mag_current']}, "
            f"y={record['y_mag_current']}, "
            f"z={record['z_mag_current']}"
        )

        print(f"  Faults:   0x{record['obc_faults']:04X}")
        print()


if __name__ == "__main__":
    main()