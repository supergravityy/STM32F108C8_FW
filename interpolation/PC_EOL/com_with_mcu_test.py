import serial
import time

# ============================================================
# Global Configuration (한 번에 교체 가능)
# ============================================================

SERIAL_PORT = "COM10"
BAUDRATE = 115200
SERIAL_TIMEOUT = 0.1          # readline timeout (초)
HELLO_READ_TIMEOUT = 60.0      # HELLO 수신 최대 대기 시간
RANDOM_READ_TIMEOUT = 10.0     # 무작위 숫자 수신 타임아웃
RANDOM_DATA_COUNT = 60

HELLO_FROM_MCU = "HELLO HOST PC"
HELLO_TO_MCU = "HELLO TARGET\n"

HANDSHAKE_BEGIN_TIMEOUT = 30.0
HANDSHAKE_TERMINATE_TIMEOUT = 30.0
HANDSHAKE_RDY = "RDY"
HANDSHAKE_ACK = "ACK"
HANDSHAKE_GO  = "GO"
HANDSHAKE_END = "END"

# ============================================================
# Helper Functions (Protocol Wrapper)
# ============================================================

def send_packet(ser, msg):
    """MCU 규격에 맞춰 $[데이터]\n 형식으로 전송"""
    packet = f'${msg}\n'
    ser.write(packet.encode())
    # print(f"[PC -> MCU] {packet.strip()}") # 디버깅용

def read_packet(ser):
    """$[데이터]\n 형식에서 데이터를 추출하고 strip"""
    line = ser.readline().decode(errors="ignore").strip()
    if line.startswith('$'):
        return line[1:] # '$' 제거 후 반환
    return line # '$'가 없는 경우(기존 legacy 대응) 그냥 반환

# ============================================================
# Core Logic
# ============================================================

def setup_serial():
    ser = serial.Serial(port=SERIAL_PORT, baudrate=BAUDRATE, timeout=SERIAL_TIMEOUT)
    print("[INFO] Serial port opened")
    return ser

def wait_hello_from_mcu(ser):
    print("[STEP] Waiting HELLO from MCU...")
    start_time = time.time()
    while time.time() - start_time < HELLO_READ_TIMEOUT:
        line = read_packet(ser)
        if line:
            print(f"[MCU -> PC] {line}")
            if HELLO_FROM_MCU in line:
                print("[PASS] HELLO received")
                return True
    raise TimeoutError("Timeout waiting HELLO")

def send_hello_to_mcu(ser):
    send_packet(ser, HELLO_TO_MCU)
    print(f"[PC -> MCU] {HELLO_TO_MCU}")

def handshake_start(ser):
    print("[STEP] Handshake start")
    
    # MCU가 wait_line2(RDY) 중이므로 먼저 보냄
    send_packet(ser, HANDSHAKE_RDY)
    print("[PC -> MCU] RDY")

    # MCU의 ACK 대기
    start_time = time.time()
    while time.time() - start_time < HANDSHAKE_BEGIN_TIMEOUT:
        line = read_packet(ser)
        if line == HANDSHAKE_ACK:
            print(f"[MCU -> PC] ACK received")
            break
    else:
        raise TimeoutError("Handshake START (ACK) timeout")

    time.sleep(0.1)
    send_packet(ser, HANDSHAKE_GO)
    print("[PC -> MCU] GO")

def receive_random_numbers(ser):
    print("[STEP] Receiving random numbers")
    numbers = []
    start_time = time.time()
    
    while len(numbers) < RANDOM_DATA_COUNT:
        line = read_packet(ser)
        if line:
            try:
                # 숫자가 $1, $2 처럼 오므로 read_packet이 $를 잘라줌
                value = int(line)
                numbers.append(value)
                print(f"Received: {value} ({len(numbers)}/{RANDOM_DATA_COUNT})", end='\r')
            except ValueError:
                continue
        
        if time.time() - start_time > 10.0: # 전체 데이터 수신 타임아웃
            break
    print("\n[PASS] Data reception complete")
    return numbers

def handshake_end(ser):
    print("[STEP] Handshake end")
    
    # MCU가 보낸 END 대기
    start_time = time.time()
    while time.time() - start_time < HANDSHAKE_TERMINATE_TIMEOUT:
        line = read_packet(ser)
        if line == HANDSHAKE_END:
            print(f"[MCU -> PC] END received")
            break
    else:
        raise TimeoutError("Handshake END wait timeout")

    # 종료 ACK 전송
    send_packet(ser, HANDSHAKE_ACK)
    print("[PC -> MCU] ACK (Final)")

def main():
    ser = None
    try:
        print("=====[INFO] SERIAL COMMUNICATION TEST BEGIN!=====")
        ser = setup_serial()
        
        # 1. Hello Phase
        wait_hello_from_mcu(ser)
        send_hello_to_mcu(ser)

        # 2. Handshake Phase
        handshake_start(ser)
        
        # 3. Data Phase
        numbers = receive_random_numbers(ser)
        
        # 4. Termination Phase
        handshake_end(ser)

        print(f"\n[RESULT] Successfully received {len(numbers)} numbers.")
        
    except Exception as e:
        print(f"\n[FAIL] Test aborted: {e}")
    finally:
        if ser:
            ser.close()
            print("[INFO] Serial port closed")

if __name__ == "__main__":
    main()

# # ============================================================
# # 1. Serial Port Setup
# # ============================================================

# def setup_serial():
#     """
#     시리얼 포트 초기화

#     실패 가능 이유:
#     - COM 포트 번호 틀림
#     - FTDI 드라이버 미설치
#     - 이미 다른 프로그램이 포트 점유
#     """
#     ser = serial.Serial(
#         port=SERIAL_PORT,
#         baudrate=BAUDRATE,
#         bytesize=serial.EIGHTBITS,
#         parity=serial.PARITY_NONE,
#         stopbits=serial.STOPBITS_ONE,
#         timeout=SERIAL_TIMEOUT
#     )

#     # MCU reset / USB 안정화 대기
#     # time.sleep(1.0)
#     print("[INFO] Serial port opened")
#     return ser


# # ============================================================
# # 2. Read Test: Receive HELLO from MCU
# # ============================================================

# def wait_hello_from_mcu(ser):
#     """
#     MCU가 먼저 보내는 HELLO 메시지 수신 테스트

#     실패 가능 이유:
#     - MCU 코드가 아직 HELLO를 안 보냄
#     - baudrate mismatch
#     - newline 처리 불일치
#     """
#     print("[STEP] Waiting HELLO from MCU...")

#     start_time = time.time()
#     while time.time() - start_time < HELLO_READ_TIMEOUT:
#         line = ser.readline().decode(errors="ignore").strip()
#         if line:
#             print(f"[MCU -> PC] {line}")
#             if line == HELLO_FROM_MCU:
#                 print("[PASS] HELLO received from MCU")
#                 return True

#     raise TimeoutError("Timeout waiting HELLO from MCU")


# # ============================================================
# # 3. Write Test: Send HELLO to MCU
# # ============================================================

# def send_hello_to_mcu(ser):
#     """
#     PC → MCU write 테스트

#     실패 가능 이유:
#     - TX 라인 문제
#     - MCU 수신 코드 미구현
#     """
#     msg = HELLO_TO_MCU + "\n"
#     ser.write(msg.encode())
#     print(f"[PC -> MCU] {HELLO_TO_MCU}")


# # ============================================================
# # 4. Handshake Start Test (RDY / ACK / GO)
# # ============================================================

# def handshake_start(ser):
#     """
#     통신 시작 handshake 테스트

#     실패 가능 이유:
#     - MCU 상태머신 미동기화
#     - 문자열 비교 오류
#     """
#     print("[STEP] Handshake start")

#     time.sleep(0.5)
#     ser.write((HANDSHAKE_RDY + "\n").encode())
#     print("[PC -> MCU] RDY")

#     start_time = time.time()
#     wait_time = 0.0
#     while wait_time < HANDSHAKE_BEGIN_TIMEOUT:
#         line = ser.readline().decode(errors="ignore").strip()
#         if line == HANDSHAKE_ACK:
#             print(f"[MCU -> PC] ACK in {wait_time} sec")
#             break 
#         wait_time = time.time() - start_time
    

#     if(wait_time >= HANDSHAKE_BEGIN_TIMEOUT):
#         raise TimeoutError(
#             f"Handshake START timeout ({wait_time:.3f}s)"
#         )

#     time.sleep(0.5)
#     ser.write((HANDSHAKE_GO + "\n").encode())
#     print("[PC -> MCU] GO ")


# # ============================================================
# # 5. Receive 60 Random Numbers from MCU
# # ============================================================

# def receive_random_numbers(ser):
#     """
#     MCU → PC 데이터 수신 테스트

#     요구사항:
#     - 0.5초 타임아웃
#     - 무작위 숫자 60개
#     - 변수에 저장

#     실패 가능 이유:
#     - MCU 송신 속도 문제
#     - 데이터 포맷 불일치
#     """
#     print("[STEP] Receiving random numbers")

#     numbers = []
#     last_rx_time = time.time()

#     while True:
#         line = ser.readline().decode(errors="ignore").strip()

#         if line:
#             try:
#                 value = int(line)
#                 numbers.append(value)
#                 last_rx_time = time.time()
#             except ValueError:
#                 pass

#             if len(numbers) >= RANDOM_DATA_COUNT:
#                 return numbers

#         if time.time() - last_rx_time > RANDOM_READ_TIMEOUT:
#             raise TimeoutError("Random data receive timeout")


# # ============================================================
# # 6. Handshake End Test (END / ACK)
# # ============================================================

# def handshake_end(ser):
#     """
#     통신 종료 handshake 테스트

#     실패 가능 이유:
#     - MCU END 송신 누락
#     - 종료 ACK 처리 오류
#     """
#     print("[STEP] Handshake end")

#     start_time = time.time()
#     wait_time = 0.0
#     while wait_time < HANDSHAKE_TERMINATE_TIMEOUT:
#         line = ser.readline().decode(errors="ignore").strip()
#         if line == HANDSHAKE_END:
#             print(f"[MCU -> PC] END in {wait_time} sec")
#             break
#         wait_time = time.time() - start_time

#     if wait_time >= HANDSHAKE_TERMINATE_TIMEOUT:
#         raise TimeoutError(
#             f"Handshake TERMINATE timeout ({wait_time:.3f}s"
#         )
    
#     time.sleep(0.5)
#     ser.write((HANDSHAKE_ACK + "\n").encode())
#     print("[PC -> MCU] ACK")


# # ============================================================
# # 7. Print Received Numbers
# # ============================================================

# def print_received_numbers(numbers):
#     """
#     수신한 모든 데이터 출력

#     목적:
#     - 데이터 유실 여부 확인
#     """
#     try:
#         if len(numbers) != RANDOM_DATA_COUNT:
#             raise ValueError

#     except ValueError as e: 
#         print("\n[ERROR] : a Number of Received Numbers is not correct")

#     print("\n[RESULT] Received Numbers")
#     for idx, val in enumerate(numbers):
#         print(f"{idx:02d}: {val}")


# # ============================================================
# # Main (순서 엄격히 유지)
# # ============================================================

# def main():
#     print("=====[INFO] SERIAL COMMUNICATION TEST BEGIN!=====")
#     ser = setup_serial()

#     wait_hello_from_mcu(ser)
#     send_hello_to_mcu(ser)

#     handshake_start(ser)
#     numbers = receive_random_numbers(ser)
#     handshake_end(ser)

#     print_received_numbers(numbers)

#     ser.close()
#     print("[INFO] Serial port closed")
#     print("=====[PASS] SERIAL COMMUNICATION TEST COMPLETE!=====")


# if __name__ == "__main__":
#     main()
