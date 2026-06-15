import time
import psutil
import serial
import serial.tools.list_ports


BAUD_RATE = 115200


def find_arduino():
    """
    Find a likely Arduino serial port.
    """

    ports = serial.tools.list_ports.comports()

    for port in ports:
        desc = (port.description or "").lower()
        device = port.device

        if (
            "arduino" in desc
            or "ch340" in desc
            or "cp210" in desc
            or "usb serial" in desc
            or "usb-serial" in desc
            or "cdc" in desc
            or device.startswith("/dev/ttyACM")
            or device.startswith("/dev/ttyUSB")
        ):
            return device

    return None


def connect():
    while True:
        port = find_arduino()

        if port:
            try:
                ser = serial.Serial(port, BAUD_RATE, timeout=1)

                # Allow Arduino to reboot after opening serial
                time.sleep(2)

                print(f"Connected to {port}")
                return ser

            except Exception as e:
                print(f"Failed to open {port}: {e}")

        print("Waiting for Arduino...")
        time.sleep(2)


def get_stats():
    cpu = int(psutil.cpu_percent(interval=0.5))

    mem = psutil.virtual_memory()
    ram_used = round(mem.used / (1024 ** 3), 1)
    ram_total = round(mem.total / (1024 ** 3))

    uptime_seconds = int(time.time() - psutil.boot_time())

    uptime_days = uptime_seconds // 86400
    uptime_hours = (uptime_seconds % 86400) // 3600

    return cpu, ram_used, ram_total, uptime_days, uptime_hours


def build_packet():
    cpu, ram_used, ram_total, days, hours = get_stats()

    return (
        f"CPU:{cpu}"
        f"RAM_USED:{ram_used}"
        f"RAM_TOTAL:{ram_total}"
        f"UPTIME_D:{days}"
        f"UPTIME_H:{hours}\n"
    )


def main():
    ser = connect()

    while True:
        try:
            packet = build_packet()

            ser.write(packet.encode("utf-8"))

            print(packet.strip())

            time.sleep(1)

        except (
            serial.SerialException,
            OSError,
        ) as e:
            print(f"Serial disconnected: {e}")

            try:
                ser.close()
            except Exception:
                pass

            ser = connect()

        except KeyboardInterrupt:
            print("\nExiting...")
            ser.close()
            break


if __name__ == "__main__":
    main()
