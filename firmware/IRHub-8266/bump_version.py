#!/usr/bin/env python3
"""
bump_version.py — Incrementa o patch de buildVersion em globals.cpp
Uso: python bump_version.py [--minor] [--major]
"""
import re
import sys
from pathlib import Path

VERSION_FILE = "version.txt"
GLOBALS_FILE = "globals.cpp"

def parse_version(v):
    return list(map(int, v.strip().split(".")))

def format_version(parts):
    return ".".join(map(str, parts))

def bump(parts, level):
    if level == "major":
        return [parts[0] + 1, 0, 0]
    elif level == "minor":
        return [parts[0], parts[1] + 1, 0]
    else:
        return [parts[0], parts[1], parts[2] + 1]

version_path = Path(VERSION_FILE)
globals_path = Path(GLOBALS_FILE)

with version_path.open("r", encoding="utf-8") as f:
    current = parse_version(f.read())

level = "patch"
if "--major" in sys.argv:
    level = "major"
elif "--minor" in sys.argv:
    level = "minor"

new_version = bump(current, level)
new_str = format_version(new_version)
old_str = format_version(current)

with version_path.open("w", encoding="utf-8", newline="\n") as f:
    f.write(new_str + "\n")

with globals_path.open("r", encoding="utf-8") as f:
    content = f.read()

new_content = re.sub(
    r'(const String buildVersion\s*=\s*")[^"]+(")',
    rf'\g<1>{new_str}\g<2>',
    content
)

with globals_path.open("w", encoding="utf-8", newline="\n") as f:
    f.write(new_content)

print(f"Versão atualizada: {old_str} → {new_str}")