import yaml
import sys
import subprocess

file_path = "CITests.yml"


def exec_command(command: str) -> bool:
    print(f"ready to run command: {command}")
    try:
        result = subprocess.run(
            command, shell=True, check=True, text=True, capture_output=True
        )
        if result.returncode == 0:
            print(f"commmand [{command}] exec success!")
            return True
        else:
            print(f"commmand [{command}] exec failed!")
            print(f"err info: {e.stderr}")
            return False

    except subprocess.CalledProcessError as e:
        print(f"commmand [{command}] exec occur exception!")
        print(f"err info: {e.stderr}")
        return False


def ci_tests(file_path: str):
    with open(file_path, "r", encoding="utf-8") as file:
        data = yaml.safe_load(file)

    tests_items = data["tests"]

    # pre-command
    for key in tests_items.keys():
        if key == "pre_command":
            pre_commands = tests_items[key]
            for item in pre_commands:
                if exec_command(item) is False:
                    sys.exit(1)
        else:
            test_body = tests_items[key]
            for sub_key in test_body.keys():
                value = test_body[sub_key]
                if value["enable"] is True:
                    if exec_command(value["command"]) is False:
                        sys.exit(1)


if __name__ == "__main__":
    ci_tests(file_path)
