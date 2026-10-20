#!/usr/bin/env python3
"""Reject exception syntax in owned C++ sources, ignoring comments and literals."""

import pathlib
import re
import subprocess
import sys


LITERALS = re.compile(
    r'//[^\n]*|/\*[\s\S]*?\*/|'
    r'R"([^ ()\\\t\r\n]{0,16})\([\s\S]*?\)\1"|'
    r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
)
FORBIDDEN = re.compile(
    r'\b(?:throw|try|catch|EXPECT_THROW|ASSERT_THROW|EXPECT_NO_THROW|ASSERT_NO_THROW)\b'
)


def violations(source):
    # C++ joins escaped newlines before tokenizing comments and keywords.
    source = re.sub(r'\\\r?\n', '', source)
    code = LITERALS.sub(lambda match: re.sub(r'[^\n]', ' ', match.group()), source)
    return [(code.count('\n', 0, match.start()) + 1, match.group())
            for match in FORBIDDEN.finditer(code)]


def main():
    assert not violations('"throw"; \'t\'; R"tag(try "catch")tag"; // throw\n/* catch */')
    assert [token for _, token in violations('try { th\\\nrow 1; } catch (...) {}')] == [
        'try', 'throw', 'catch'
    ]
    root = pathlib.Path(__file__).resolve().parent.parent
    files = subprocess.check_output(
        ['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard'], cwd=root
    ).decode().split('\0')
    failed = False
    for name in sorted(set(files)):
        path = pathlib.Path(name)
        if not name or path.parts[0] in ('external', 'build'):
            continue
        if path.suffix not in ('.h', '.hpp', '.hxx', '.cpp', '.cc', '.cxx', '.mm', '.inc'):
            continue
        if '.spv.' in name:
            continue
        for line, token in violations((root / path).read_text()):
            print(f'{name}:{line}: forbidden exception syntax: {token}')
            failed = True
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
