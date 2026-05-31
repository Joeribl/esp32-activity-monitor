import time
import psutil
import serial
import serial.tools.list_ports

def main():
    port = find_esp32_port()

    if port:
        print(f"ESP32 found on port: {port.device}")
    else:
        print("ESP32 not found. Please check the connection.")
        exit(1)

    ser = serial.Serial(port.device, 115200, timeout=1)
    time.sleep(2) # Wait for the serial connection to initialize

    try:
        while True:
            cpu_usage = psutil.cpu_percent(interval=1)
            total_memory, used_memory = get_memory_info()
            charge, plugged = get_battery_info()
            data_string = f"CPU:{cpu_usage},RAM_USED:{used_memory},RAM_TOTAL:{total_memory},BATTERY:{charge},PLUGGED:{plugged}"
            ser.write(data_string.encode('utf-8'))
            print(f"Sent data: {data_string}")
            time.sleep(1)

    except KeyboardInterrupt:
        print("Exiting...")
        ser.close()
        exit(0)


def get_memory_info():
    memory = psutil.virtual_memory()
    total_memory = memory.total / 1024 ** 3 # Convert from bytes to GB
    used_memory = (memory.used / 1024 ** 3).__round__(1) # Convert from bytes to GB
    return total_memory, used_memory

def get_battery_info():
    battery = psutil.sensors_battery()
    charge = battery.percent
    plugged = battery.power_plugged
    return charge, plugged

def find_esp32_port():
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if 'usbserial' in port.device:
            return port
    return None


if __name__ == "__main__":
    main()