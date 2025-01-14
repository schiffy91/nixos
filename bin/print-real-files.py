#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
# python find_files.py /mnt --ignore-directories /mnt/nix /mnt/boot --print-contents
import os, argparse

def read_file_contents(path):
    try:
        with open(path, "r", encoding="utf-8") as f: return f.read()
    except Exception as e: return f"[Error reading file: {str(e)}]"

def find_files(directory, ignore_dirs, print_contents):
    ignore_dirs = [os.path.abspath(d) for d in ignore_dirs]
    for root, _, files in os.walk(directory):
        if any(ignore_dir in root for ignore_dir in ignore_dirs):
            continue
        for file in files:
            full_path = os.path.join(root, file)
            if os.path.isfile(full_path) and not os.path.islink(full_path):
                print(full_path)
                print("---------------------------")
                if print_contents:
                    print(read_file_contents(full_path).strip())
                print("---------------------------\n")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", help="Directory to search")
    parser.add_argument("--ignore-directories", nargs="*", default=[], help="Directories to ignore")
    parser.add_argument("--print-contents", action="store_true", help="Print contents of each file")
    args = parser.parse_args()
    if not os.path.isdir(args.directory):
        print(f"Error: {args.directory} is not a valid directory")
        return
    find_files(args.directory, args.ignore_directories, args.print_contents)

if __name__ == "__main__": main()
