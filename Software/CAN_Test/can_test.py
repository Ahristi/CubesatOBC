import can
import time

CHANNEL = "PCAN_USBBUS1"
BITRATE = 125000

bus = can.interface.Bus(
    interface="pcan",
    channel=CHANNEL,
    bitrate=BITRATE
)

print("PCAN started at 125 kbps")
print("Sending frame 0x321 every 1 s...")

try:
    while True:
        msg = can.Message(
            arbitration_id=0x321,
            data=[0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88],
            is_extended_id=False
        )

        bus.send(msg)
        print(f"TX: ID=0x{msg.arbitration_id:X}, DATA={msg.data.hex(' ')}")

        rx = bus.recv(timeout=1.5)
        if rx is not None:
            print(f"RX: ID=0x{rx.arbitration_id:X}, DATA={rx.data.hex(' ')}")

        time.sleep(0.1)
        msg = can.Message(
            arbitration_id=0x323,
            data=[0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88],
            is_extended_id=False
        )

        bus.send(msg)
        print(f"TX: ID=0x{msg.arbitration_id:X}, DATA={msg.data.hex(' ')}")

        rx = bus.recv(timeout=1.5)
        if rx is not None:
            print(f"RX: ID=0x{rx.arbitration_id:X}, DATA={rx.data.hex(' ')}")

        time.sleep(0.5)

except KeyboardInterrupt:
    print("Stopped")

finally:
    bus.shutdown()