#!/usr/bin/env python3
"""Offline regression of the exact cleanup bodies with reduced mock types.

Does not compile the complete firmware or use actual ESP-IDF. Requires Python 3,
GCC and Clang. No network, credentials or third-party Python packages.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def fragment(path, start, end=None):
    source = (ROOT / path).read_text(encoding="utf-8")
    if source.count(start) != 1:
        raise ValueError(f"unexpected marker: {path}: {start}")
    result = source[source.index(start):]
    if end is not None:
        if end not in result:
            raise ValueError(f"missing end marker: {path}: {end}")
        result = result[:result.index(end)]
    return result.rstrip() + "\n"


def main():
    pieces = {
        "radio.inc": fragment("src/room_apsta_owner.h", "typedef bool (*room_apsta_guard)(void *);", "#endif"),
        "session.inc": fragment("main/room_apsta_session.c", "static bool cleanup_live(void *v) {"),
        "task.inc": fragment("main/room_apsta_task.c", "bool room_apsta_task_close_until("),
        "link.inc": fragment("main/room_wifi_apsta_link.c", "static bool channel_clock(", "static bool load("),
        "attach.inc": fragment("main/room_wifi_apsta_link.c", "bool room_wifi_apsta_link_attach_io_until("),
        "outer.inc": fragment("main/room_wifi_idf.c", "static bool cleanup_live(void) {", "static void configure("),
        "channel.inc": fragment("main/room_phone_channel.c", "static bool owner(", "static bool active(") + fragment("main/room_phone_channel.c", "static bool timed_live("),
    }
    with tempfile.TemporaryDirectory(prefix="room-cleanup-") as temp:
        work = Path(temp)
        for name, body in pieces.items():
            (work / name).write_text(body, encoding="utf-8")
        for cc in ("gcc", "clang"):
            binary = work / cc
            subprocess.run([cc, "--version"], check=True, timeout=10)
            subprocess.run([cc, "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
                            "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                            "-fno-omit-frame-pointer", "-no-pie", "-I", str(work),
                            "-I", str(ROOT / "src"), str(ROOT / "tests/test_room_cleanup_path.c"),
                            "-o", str(binary)], check=True, timeout=30)
            subprocess.run([str(binary)], check=True, timeout=15)


if __name__ == "__main__":
    main()
