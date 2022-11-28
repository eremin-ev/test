#!/usr/bin/env python3

import argparse
import itertools
from pathlib import Path

def list_names(paths):
    files = []
    dirs = []

    for path in map(Path, paths):
        if path.is_file():
            files.append(path)
        elif path.is_dir():
            dirs.append(path.rglob('*.cc'))

    print(f'files {files}, dirs {dirs}')

    names = itertools.chain(files, itertools.chain.from_iterable(dirs))

    for name in names:
        print(f'name {name}')


def main():
    parser = argparse.ArgumentParser(description='Test pathlib.')
    parser.add_argument('path', nargs='*',
                        help='Path to file or directory')
    args = parser.parse_args()

    list_names(args.path)

if __name__ == '__main__':
    main()
