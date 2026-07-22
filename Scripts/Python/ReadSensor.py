"""
Reads DataFrame_t packets from the STM32 over UART and decodes them.

Frame layout (little-endian, packed, matches CommsTask::DataFrame_t):
  uint8_t  header;        // 0xAA
  uint8_t  length;        // payload length (17)
  float    temperature;   // 4 bytes
  float    pressure;      // 4 bytes
  float    humidity;      // 4 bytes
  uint32_t timestamp_ms;  // 4 bytes
  uint8_t  checksum;      // XOR of the 17 payload bytes
Total: 19 bytes
"""

import serial
import struct
import sys

PORT = "/dev/serial0"
BAUD = 115200
HEADER = 0xAA
FRAME_LEN = 19          # header + length + 17 payload bytes + checksum... wait see below
PAYLOAD_LEN = 17        # length byte value
STRUCT_FMT = "<BBfffI B"  # header, length, temp, pres, humi, timestamp, checksum (little-endian)


def find_header(ser):
    """Byte-align: scan until we see 0xAA."""
    while True:
        b = ser.read(1)
        if not b:
            raise TimeoutError("No data received while syncing")
        if b[0] == HEADER:
            return


def read_frame(ser):
    find_header(ser)
    rest = ser.read(FRAME_LEN - 1)  # everything after the header byte
    if len(rest) != FRAME_LEN - 1:
        return None  # timeout / short read

    frame = bytes([HEADER]) + rest
    header, length, temp, pres, humi, ts, chk = struct.unpack(STRUCT_FMT, frame)

    # verify checksum: firmware XORs 17 bytes starting after header+length,
    # but the checksum field itself is zeroed at that point, so only the
    # 16 real payload bytes (temp+pres+humi+timestamp) actually matter.
    payload = frame[2:2 + 16]
    calc_chk = 0
    for b in payload:
        calc_chk ^= b

    ok = (calc_chk == chk) and (length == PAYLOAD_LEN)

    return {
        "temperature": temp,
        "pressure": pres,
        "humidity": humi,
        "timestamp_ms": ts,
        "checksum_ok": ok,
        "raw": frame.hex(),
    }


def main():
    print(f"Opening {PORT} @ {BAUD} baud...")
    ser = serial.Serial(PORT, BAUD, timeout=2)

    print("Listening for frames (Ctrl-C to stop)...\n")
    try:
        while True:
            try:
                data = read_frame(ser)
            except TimeoutError:
                print("Timed out waiting for header byte (0xAA) - check connection")
                continue

            if data is None:
                print("Short/incomplete frame, resyncing...")
                continue

            if not data["checksum_ok"]:
                print(f"[DROPPED] checksum failed, discarding frame "
                      f"(raw={data['raw']})")
                continue

            print(
                f"[OK] T={data['temperature']:.2f}C  "
                f"P={data['pressure']:.2f}hPa  "
                f"H={data['humidity']:.2f}%  "
                f"t={data['timestamp_ms']}ms"
            )
    except KeyboardInterrupt:
        print("\nStopping.")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
