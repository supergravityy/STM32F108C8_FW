import serial
import csv
import os
import time
from colorama import Fore, init

init(autoreset=True)

BAUDRATE    = 115200
PORTNUM     = "COM10"

TESTCASE_CSV_FILENAME = "testcase.csv"
RESULT_CSV_FILENAME = "result_test.csv"
REQUIRED_COLUMNS = ["TC_ID", "FUNC", "INPUT", "EXPECTED"]

COMMUNICATION_PREFIX = "$"
COMMUNICATION_SUFFIX = "\n"

HANDSHAKE_RCV_TIMEOUT = 10.0
PC_TO_MCU_RDY = "RDY"
PC_TO_MCU_GO  = "GO"
PC_TO_MCU_ACK = "ACK"
MCU_TO_PC_ACK = "ACK"
MCU_TO_PC_END = "END"

RCV_WHOLE_OUTPUT_TIMEOUT = 30.0

# ===============================
# Protocol Helpers (MCU 규격 일치화)
# ===============================
def send_data(ser, msg: str):
    """$[데이터]\n 형식으로 전송"""
    frame = f'{COMMUNICATION_PREFIX}{msg}{COMMUNICATION_SUFFIX}'
    ser.write(frame.encode())

def read_data(ser):
    """$[데이터]\n 형식에서 데이터를 추출"""
    line = ser.readline().decode(errors="ignore").strip()
    if line.startswith(COMMUNICATION_PREFIX):
        return line[len(COMMUNICATION_PREFIX):]
    return line

# ===============================
# 1~3. Setup & CSV Logic
# ===============================
def setup_serial(port: str, baudrate: int, timeout: float = 1.0):
    try:
        ser = serial.Serial(port=port, baudrate=baudrate, timeout=timeout)
        print(f"[INFO] Serial opened: {port}")
        return ser
    except Exception as e:
        print(f"[ERROR] Could not open port: {e}")
        exit(1)

def open_csv(filepath: str):

    if not os.path.exists(filepath):
        raise FileNotFoundError(f"[ERROR] CSV not found: {filepath}")
    
    return open(filepath, newline="", encoding="utf-8")

def read_testcases(csv_file):
    reader = csv.DictReader(csv_file)

    # ---- header check ----
    if reader.fieldnames is None:
        raise ValueError("[ERROR] CSV has no header")

    for field in REQUIRED_COLUMNS:
        if field not in reader.fieldnames:
            raise ValueError(f"[ERROR] Missing column: {field}")

    testcases = []
    row_idx = 2
    for row in reader:
        try:
            tc = {
                REQUIRED_COLUMNS[0] : row[REQUIRED_COLUMNS[0]],        # 문자열 그대로 둬도 됨
                REQUIRED_COLUMNS[1]: row[REQUIRED_COLUMNS[1]].strip(),
                REQUIRED_COLUMNS[2]: int(row[REQUIRED_COLUMNS[2]]),
                REQUIRED_COLUMNS[3]: int(row[REQUIRED_COLUMNS[3]])
            }
        except ValueError as e:
            raise ValueError(f"[ERROR] Row {row_idx} value error: {e}") # 오타감지용 -> 자료형 불일치
        except KeyError as e:
            raise KeyError(f"[ERROR] Row {row_idx} missing field: {e}") # 오타감지용 -> 키에 맞는 값이 없음

        testcases.append(tc)
        row_idx += 1

    print(f"[INFO] Loaded {len(testcases)} testcases")
    return testcases

# ===============================
# 4. Handshake Start
# ===============================
def handshake_start(ser):
    print("[INFO] Sending RDY")

    send_data(ser, PC_TO_MCU_RDY)

    # 2. MCU의 ACK 대기
    print("[INFO] Waiting for ACK from MCU")
    start_time = time.time()
    while time.time() - start_time < HANDSHAKE_RCV_TIMEOUT:
        if read_data(ser) == MCU_TO_PC_ACK:
            print("[INFO] ACK received from MCU")
            break
    else:
        raise TimeoutError("Handshake ACK timeout")

    print("[INFO] Sending GO")
    send_data(ser, PC_TO_MCU_GO)
    print("[INFO] Sent GO to MCU")

# ===============================
# 5. Run Test & Print Result
# ===============================
def run_test_and_print(ser, testcases):
    results = []
    tc_index = 0
    total_tcs = len(testcases)
    start_time = time.time()

    print("[STEP] Running Tests (Group Mode: 5 TCs per line)...")

    while tc_index < total_tcs or time.time() - start_time < RCV_WHOLE_OUTPUT_TIMEOUT:
        line = read_data(ser)
        if not line: continue

        if line == MCU_TO_PC_END:
            print("[INFO] MCU signaled END")
            break

        # MCU에서 보낸 데이터 파싱 (예: "4200,4200,4200,4200,4200")
        try : 
            mcu_values = [int(val) for val in line.split(",")]
        except ValueError:
            print(Fore.YELLOW + f"[WARNING] Invalid data from MCU: {line}")
            continue

        # 한 번에 5개의 결과가 들어왔으므로, CSV의 다음 5개 단위로 TC와 비교
        for mcu_outp in mcu_values:
            if tc_index >= total_tcs:
                break
            
            tc = testcases[tc_index]
            expected = tc[REQUIRED_COLUMNS[3]]
            verdict = "PASS" if mcu_outp == expected else "FAIL"
            color = Fore.GREEN if verdict == "PASS" else Fore.RED

            print(color + f"[{verdict}] "
                  f"TC={tc[REQUIRED_COLUMNS[0]]} "
                  f"FUNC={tc[REQUIRED_COLUMNS[1]]} IN={tc[REQUIRED_COLUMNS[2]]} "
                  f"OUT={mcu_outp} EXP={expected}")
            
            results.append({ **tc, "OUTPUT": mcu_outp, "RESULT": verdict })

            tc_index += 1

    return results


# ===============================
# 6. Handshake End
# ===============================
def handshake_end(ser):
    send_data(ser, PC_TO_MCU_ACK)
    print("[INFO] Sent Final ACK (Handshake End)")


# ===============================
# 7. Write Result CSV
# ===============================
def write_result_csv(filepath: str, results):
    if not results: return

    fieldnames = REQUIRED_COLUMNS
    fieldnames.append("OUTPUT")
    fieldnames.append("RESULT")

    with open(filepath, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(results)

    print(f"[INFO] Result CSV written: {filepath}")


# ===============================
# 8. Close
# ===============================
def close_csv_files(ser, csv_file):
    csv_file.close()
    ser.close()
    print("[INFO] Serial & CSV closed")


# ===============================
# Main Flow
# ===============================
def main():
    ser = setup_serial(PORTNUM, BAUDRATE)
    
    try:
        csv_file = open_csv(TESTCASE_CSV_FILENAME)

        # 1. 파일 읽기
        testcases = read_testcases(csv_file)

        # 2. 핸드쉐이크 (PC:RDY -> MCU:ACK -> PC:GO)
        input("[ACTION] Press Enter to start handshake...")
        handshake_start(ser)

        # 3. 테스트 실행 및 데이터 수신
        results = run_test_and_print(ser, testcases)

        # 4. 종료 절차
        handshake_end(ser)

        # 5. 결과 저장
        write_result_csv(RESULT_CSV_FILENAME, results)

    except Exception as e:
        print(f"\n[FATAL ERROR] {e}")
    finally:
        close_csv_files(ser, csv_file)
        if 'ser' in locals() and 'csv_file' in locals():
            close_csv_files(ser, csv_file)

if __name__ == "__main__":
    main()
