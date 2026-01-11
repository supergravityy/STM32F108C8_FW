import serial
import time

ser = serial.Serial("COM10", 115200, timeout=1)
cnt = 0
base_msg = "HELLO_FROM_PC "

while True:
    time.sleep(1)
    cnt += 1
    if cnt > 9: cnt = 0
    
    # f-string을 사용해 매번 새로 생성하는 것이 가장 깔끔합니다.
    full_msg = f'${base_msg}{cnt}\n'
    print(f"send: {full_msg.strip()}")
    ser.write(full_msg.encode())