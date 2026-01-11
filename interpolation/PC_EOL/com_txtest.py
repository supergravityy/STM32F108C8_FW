import serial

ser = serial.Serial("COM10", 115200, timeout=1)

while True:
    line = ser.readline()
    if line:
        print(line.decode(errors="ignore").strip())