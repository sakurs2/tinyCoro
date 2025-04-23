import sys
import subprocess
import time

RED = "\033[31m"
GREEN = "\033[32m"
RESET = "\033[0m"

# test cases: [(<threadnum>, <port>) ...]
paras = [(1, 8000), (0, 8001)]


def run_test(
    thread_num: int,
    server_port: int,
    benchprogram_path: str,
    benchtool_path: str,
) -> bool:
    try:
        program_process = subprocess.Popen(
            [benchprogram_path, str(thread_num), str(server_port)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        time.sleep(2)
        if program_process.poll() is not None:  # terminate
            stdout, stderr = program_process.communicate()
            print(f"{RED}[error] {benchprogram_path} start failed{RESET}")
            print(f"{RED}[return code] {program_process.returncode}{RESET}")
            print(f"{RED}[stdout] {stdout}{RESET}")
            print(f"{RED}[stderr] {stderr}{RESET}")
            return False

        bench_process = subprocess.Popen(
            [
                benchtool_path,
                "--address",
                f"127.0.0.1:{server_port}",
                "--number",
                "100",
                "--duration",
                "30",
                "--length",
                "1024",
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        time.sleep(2)
        if bench_process.stderr.readline() != "":
            print(f"{RED}[error] bench tools running error{RESET}")
            bench_process.kill()
            _, stderr = bench_process.communicate()
            print(f"{RED}[stderr] {stderr}{RESET}")
            if program_process.poll() is None:
                program_process.kill()
            return False

        time.sleep(40)

        # check status
        if bench_process.poll() is None:  # running
            stdout, stderr = program_process.communicate()
            print(
                f"{RED}[error] {benchtool_path} hasn't finish running, please check your server code{RESET}"
            )
            bench_process.kill()
            if program_process.poll() is None:
                program_process.kill()
            return False

        if program_process.poll() is not None:  # terminate
            stdout, stderr = program_process.communicate()
            print(f"{RED}[error] {benchprogram_path} crashed when test{RESET}")
            print(f"{RED}[return code] {program_process.returncode}{RESET}")
            print(f"{RED}[stdout] {stdout}{RESET}")
            print(f"{RED}[stderr] {stderr}{RESET}")
            return False

        # collect result
        stdout, _ = bench_process.communicate()
        print(f"{GREEN}[test result] {stdout}{RESET}")

        program_process.kill()

    except Exception as e:
        print(f"{RED}exception occur! detail: {e}{RESET}")
        return False

    return True


if __name__ == "__main__":
    for i, para in enumerate(paras):
        print(
            f"{GREEN}================== lab3 test case {i} begin =================={RESET}"
        )
        thread_num = para[0]
        if thread_num == 0:
            thread_num = "default core number"
        print(f"{GREEN}thread number: {thread_num}, server port: {para[1]}{RESET}")
        if not run_test(para[0], para[1], sys.argv[1], sys.argv[2]):
            print(
                f"{RED}xxxxxxxxxxxxxxxxxx lab3 test case {i} error xxxxxxxxxxxxxxxxxx{RESET}"
            )
            sys.exit(1)
        else:
            print(
                f"{GREEN}================== lab3 test case {i} finish ================={RESET}"
            )
            time.sleep(2)
    print(f"{GREEN}you pass lab3 test, congratulations!{RESET}")
