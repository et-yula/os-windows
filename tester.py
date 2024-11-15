import os
import subprocess
import time
from pathlib import Path
import re
import struct

def compile_sources(output_filename, input_array_cpp_files):
    build_dir = Path("build")
    build_dir.mkdir(exist_ok=True)
    compile_arr = ["g++", "-o", str(build_dir / output_filename)] + input_array_cpp_files
    print(" ".join(compile_arr))
    result = subprocess.run(compile_arr, capture_output=True, text=True)
    if result.returncode != 0:
        print("Compilation error:")
        print(result.stderr)
        exit(1)
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
            exit(1)
        validate_output(i+1, input_value, stdout)

def read_binary_file(filename):
    if not os.path.exists(filename):
        print(f"Error: File {filename} does not exist.")
        exit(1)
    int_array = []
    with open(filename, 'rb') as file:
        while True:
            bytes_read = file.read(4)
            if not bytes_read:
                break
            value = struct.unpack('i', bytes_read)[0]
            int_array.append(value)
    return int_array

def is_sorted(int_array):
    for i in range(1, len(int_array)):
        if int_array[i] < int_array[i - 1]:
            return False
    return True

def create_random_binary_file(filename, length):
    with open(filename, 'wb') as f:
        random_bytes = os.urandom(length)
        f.write(random_bytes)

def main():
    output_filename = "main.exe"
    output_benchmark = "benchmark.exe"
    output_benchmark2 = "benchmark2.exe"
    cpp_source = [str(file) for file in Path("./source").rglob("*.cpp")]
    cpp_benchmark = [str(file) for file in Path("./benchmark").rglob("*1.cpp")]
    cpp_benchmark2 = [str(file) for file in Path("./benchmark").rglob("*2.cpp")]
    input_data = ["cd build\n./benchmark.exe input_file.txt output_file.txt\nexit\n", "cd build\n./benchmark2.exe 256000 2\nexit\n"]
    output_data = ["__PATH__$ __PATH__\\build$ \nExecution time: __NUM__ seconds\n__PATH__\\build$ ", "_"]
    
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
            if compile_sources(output_benchmark2, cpp_benchmark2):
                create_random_binary_file("build/input_file.txt",256*1024); # 256 MB
                run_and_validate(output_filename, input_data, validate_output)
                if not is_sorted(read_binary_file("build/output_file.txt")):
                    print("The array from benchmark №1 is not sorted!")
                    exit(0)

if __name__ == "__main__":
    main()
