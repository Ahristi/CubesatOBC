import can
import time
import struct
import math

CHANNEL = "PCAN_USBBUS1"
BITRATE = 125000   # Change to 500000 if using ICD value instead of your current bench setup

# CAN IDs
EPS_3V3_TELEMETRY_ID = 0x320
EPS_5V_TELEMETRY_ID = 0x321
EPS_6V_TELEMETRY_ID = 0x322
EPS_12V_TELEMETRY_ID = 0x323
EPS_BMS_TELEMETRY_ID = 0x324
EPS_SYS_TELEMETRY_ID = 0x325


def u16_from_scaled(value, scale):
    raw = int(round(value / scale))
    raw = max(0, min(raw, 0xFFFF))
    return raw


def s16_from_scaled(value, scale):
    raw = int(round(value / scale))
    raw = max(-32768, min(raw, 32767))
    return raw


def s8_from_scaled(value, scale=1.0):
    raw = int(round(value / scale))
    raw = max(-128, min(raw, 127))
    return raw


def u8_from_scaled(value, scale=1.0):
    raw = int(round(value / scale))
    raw = max(0, min(raw, 255))
    return raw


def pack_regulator_telem(rail_voltage, current_ch1, current_ch2,
                         voltage_scale, current_scale):
    """
    Regulator telemetry format:
      bytes 0-1: rail voltage, u16
      bytes 2-3: channel 1 current, u16
      bytes 4-5: channel 2 current, u16

    Little-endian packing is assumed.
    """
    v_raw = u16_from_scaled(rail_voltage, voltage_scale)
    i1_raw = u16_from_scaled(current_ch1, current_scale)
    i2_raw = u16_from_scaled(current_ch2, current_scale)

    return struct.pack("<HHH", v_raw, i1_raw, i2_raw)


def pack_bms_telem(battery_voltage, battery_current, sys_voltage,
                   battery_temp, die_temp):
    """
    BMS telemetry format:
      bytes 0-1: battery voltage, s16, 0.001 V/bit
      bytes 2-3: battery current, s16, 0.001 A/bit
      bytes 4-5: SYS voltage, s16, 0.001 per bit
      byte 6: battery temperature, s8, 1 degC/bit
      byte 7: die temperature, s8, 1 degC/bit

    Note: ICD says SYS voltage scale unit is A/bit, but this looks like a typo.
    This script treats it as 0.001 V/bit.
    """
    batt_v_raw = s16_from_scaled(battery_voltage, 0.001)
    batt_i_raw = s16_from_scaled(battery_current, 0.001)
    sys_v_raw = s16_from_scaled(sys_voltage, 0.001)
    batt_t_raw = s8_from_scaled(battery_temp)
    die_t_raw = s8_from_scaled(die_temp)

    return struct.pack("<hhhbb", batt_v_raw, batt_i_raw, sys_v_raw,
                       batt_t_raw, die_t_raw)


def pack_sys_telem(efuse_state_mask, efuse_fault_mask, mcu_temp):
    """
    System telemetry format:
      byte 0: eFuse states mask
      byte 1: eFuse faults mask
      byte 2: MCU temperature, u8, 1 degC/bit
    """
    return bytes([
        efuse_state_mask & 0xFF,
        efuse_fault_mask & 0xFF,
        u8_from_scaled(mcu_temp)
    ])


def send_frame(bus, arbitration_id, data):
    msg = can.Message(
        arbitration_id=arbitration_id,
        data=data,
        is_extended_id=False
    )
    bus.send(msg)

    print(
        f"TX ID=0x{arbitration_id:03X} "
        f"DLC={len(data)} "
        f"DATA={data.hex(' ')}"
    )


def main():
    bus = can.interface.Bus(
        interface="pcan",
        channel=CHANNEL,
        bitrate=BITRATE
    )

    print(f"PCAN EPS telemetry emulator started on {CHANNEL} at {BITRATE} bps")

    t0 = time.time()

    try:
        while True:
            t = time.time() - t0

            # Small sinusoidal variation to make telemetry look alive
            ripple = math.sin(t * 0.5)

            # Regulator telemetry
            data_3v3 = pack_regulator_telem(
                rail_voltage=3.31 + 0.01 * ripple,
                current_ch1=0.20,
                current_ch2=0.10,
                voltage_scale=0.001555,
                current_scale=0.0008954
            )

            data_5v = pack_regulator_telem(
                rail_voltage=5.02 + 0.01 * ripple,
                current_ch1=0.35,
                current_ch2=0.25,
                voltage_scale=0.00161132812,
                current_scale=0.0012397
            )

            data_6v = pack_regulator_telem(
                rail_voltage=6.01 + 0.01 * ripple,
                current_ch1=0.60,
                current_ch2=0.00,
                voltage_scale=0.00161132812,
                current_scale=0.003663
            )

            data_12v = pack_regulator_telem(
                rail_voltage=12.02 + 0.02 * ripple,
                current_ch1=0.15,
                current_ch2=0.45,
                voltage_scale=0.006498,
                current_scale=0.00080586
            )

            # BMS telemetry
            data_bms = pack_bms_telem(
                battery_voltage=16.20 + 0.02 * ripple,
                battery_current=-0.25,     # negative = discharging, if your convention uses that
                sys_voltage=16.10,
                battery_temp=24,
                die_temp=31
            )

            # System telemetry
            efuse_state_mask = 0b00111111   # all listed eFuses on
            efuse_fault_mask = 0b00000000   # no faults
            data_sys = pack_sys_telem(
                efuse_state_mask=efuse_state_mask,
                efuse_fault_mask=efuse_fault_mask,
                mcu_temp=32
            )

            send_frame(bus, EPS_3V3_TELEMETRY_ID, data_3v3)
            time.sleep(0.02)

            send_frame(bus, EPS_5V_TELEMETRY_ID, data_5v)
            time.sleep(0.02)

            send_frame(bus, EPS_6V_TELEMETRY_ID, data_6v)
            time.sleep(0.02)

            send_frame(bus, EPS_12V_TELEMETRY_ID, data_12v)
            time.sleep(0.02)

            send_frame(bus, EPS_BMS_TELEMETRY_ID, data_bms)
            time.sleep(0.02)

            send_frame(bus, EPS_SYS_TELEMETRY_ID, data_sys)

            print("-" * 60)

            # Overall telemetry period
            time.sleep(1.0)

    except KeyboardInterrupt:
        print("Stopped")

    finally:
        bus.shutdown()


if __name__ == "__main__":
    main()