#!/usr/bin/env python3
"""
bump_version.py — Incrementa o patch de buildVersion em globals.cpp
Uso: python bump_version.py [--minor] [--major]
"""
import re
import sys

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

# Lê versão atual
with open(VERSION_FILE, "r") as f:
    current = parse_version(f.read())

# Determina nível de bump
level = "patch"
if "--major" in sys.argv:
    level = "major"
elif "--minor" in sys.argv:
    level = "minor"

new_version = bump(current, level)
new_str = format_version(new_version)
old_str = format_version(current)

# Atualiza version.txt
with open(VERSION_FILE, "w") as f:
    f.write(new_str + "\n")

# Atualiza globals.cpp
with open(GLOBALS_FILE, "r") as f:
    content = f.read()

new_content = re.sub(
    r'(const String buildVersion\s*=\s*")[^"]+(")',
    rf'\g<1>{new_str}\g<2>',
    content
)

with open(GLOBALS_FILE, "w") as f:
    f.write(new_content)

print(f"Versão atualizada: {old_str} → {new_str}")