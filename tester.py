import os
import subprocess
import time
from pathlib import Path
import re

def compile_sources(output_filename, input_array_cpp_files):
    build_dir = Path("build")
    build_dir.mkdir(exist_ok=True)
    result = subprocess.run(["g++", "-o", str(build_dir / output_filename)] + input_array_cpp_files, capture_output=True, text=True)
    if result.returncode != 0:
        print("Compilation error:")
        print(result.stderr)
        return False
    else:
        return True

def run_and_validate(output_filename, input_data, validate_output):
    executable_path = Path("build") / output_filename
    if not executable_path.exists():
        print(f"Executable file {output_filename} not found.")
        return
    
    for i, input_value in enumerate(input_data):
        input_lines = input_value.splitlines()
        process = subprocess.Popen([str(executable_path)], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        for line in input_lines:
            process.stdin.write(line + '\n')
            process.stdin.flush()
            time.sleep(0.5)
        process.stdin.close()
        stdout, stderr = process.communicate()
        if process.returncode != 0:
            print(f"Error during test {(i+1)} with input data: {input_value}")
            print(stderr)
            continue
        validate_output(i+1, input_value, stdout)

def main():
    output_filename = "main.exe"
    output_benchmark = "benchmark.exe"
    cpp_source = [str(file) for file in Path("./source").rglob("*.cpp")]
    cpp_benchmark = [str(file) for file in Path("./benchmark").rglob("*.cpp")]
    input_data = ["cd build\n./main.exe\nexit\n"]
    output_data = ["__PATH__$ __PATH__\\build$ Sleep 5.5s...\nDone\nExecution time: __NUM__ seconds\n__PATH__\\build$ "]
    
    def validate_output(test_number, input_value, output_value):
        cout = output_value.replace(os.path.dirname(os.path.abspath(__file__)), "__PATH__")
        cout = re.sub(r"\d+.\d+ seconds", "__NUM__ seconds", cout)
        if cout == output_data[test_number-1]:
            print(f"Test {test_number} passed")
        else:
            print(f"Test {test_number} failed:\nInput data: {input_value}\nResult: {cout}\nExpected: {output_data[test_number-1]}")
            exit(1)

    if compile_sources(output_filename, cpp_source):
        if compile_sources(output_benchmark, cpp_benchmark):
            run_and_validate(output_benchmark, input_data, validate_output)

if __name__ == "__main__":
    main()
