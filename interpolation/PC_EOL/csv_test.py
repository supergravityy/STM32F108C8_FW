import csv
import os

TESTCASE_CSV_FILENAME = "testcase.csv"
RESULT_CSV_FILENAME = "result_test.csv"

REQUIRED_COLUMNS = ["TC_ID", "FUNC", "INPUT", "EXPECTED"]

def open_csv(filepath: str):
    print("[TEST] CSV open test")

    if not os.path.exists(filepath):
        raise FileNotFoundError(f"[ERROR] CSV file not found: {filepath}")
    
    fp = open(filepath, newline="", encoding="utf-8")
    print(f"[PASS] CSV opened: {filepath}")
    return fp

def read_testcases(csv_file):
    print("[TEST] CSV format & data test")

    reader = csv.DictReader(csv_file) # csv 파일 안에서 줄 하나를 뽑아서 딕셔너리 형태로 바꿈

    # ---- header check ----
    if reader.fieldnames is None:
        raise ValueError("[ERROR] CSV has no header")
    
    for field in REQUIRED_COLUMNS:
        if field not in reader.fieldnames:
            raise ValueError(f"[ERROR] Missing column: {field}")
        
    print("[PASS] CSV header validated")

    # ---- row parsing ----
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
            raise ValueError(f"[ERROR] Row {row_idx} value error: {e}") # 테스트케이스 오타감지용 -> 자료형 불일치
        except KeyError as e:
            raise KeyError(f"[ERROR] Row {row_idx} missing field: {e}") # 테스트케이스 오타감지용 -> 키에 맞는 값이 없음

        testcases.append(tc)
        row_idx += 1

    print(f"[PASS] Loaded {len(testcases)} testcases")
    return testcases

def write_result_csv(filepath, results):
    fieldnames = ["TC_ID", "FUNC", "INPUT", "EXPECTED", "OUTPUT", "RESULT"]

    with open(filepath, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(results)

    print(f"[PASS] Result CSV written: {filepath}")

def main():
    print("===== CSV SELF TEST START =====")

    try: 
        csv_file = open_csv(TESTCASE_CSV_FILENAME)
        testcases = read_testcases(csv_file)
    except Exception as e:
        print(e)
        print("===== CSV SELF TEST FAIL =====")
        return
    finally:
        try:
            csv_file.close()
        except Exception:
            pass

    # ---- print all rows ----
    print("\n[INFO] PRINT ALL DICTIONARY")
    for tc in testcases[:]:
        print(tc)

    fake_results = [
        {
            "TC_ID": "TC01",
            "FUNC": "u16u16",
            "INPUT": 1500,
            "EXPECTED": 4200,
            "OUTPUT": 4200,
            "RESULT": "PASS"
        },
        {
            "TC_ID": "TC31",
            "FUNC": "u16u16",
            "INPUT": 300,
            "EXPECTED": 677,
            "OUTPUT": 680,
            "RESULT": "FAIL"
        }
    ]

    write_result_csv(RESULT_CSV_FILENAME, fake_results)

    print("===== CSV SELF TEST PASS =====")


if __name__ == "__main__":
    main()