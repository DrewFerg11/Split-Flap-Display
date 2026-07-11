# FAIL THE BUILD IF firmware.bin DOES NOT FIT THE APP PARTITION
#
# PlatformIO's "Flash: X% (used N from M)" check sums ELF sections, but the
# flashed .bin also contains MMU-alignment padding between mapped segments
# (~113KB on ESP32-C3). The bootloader validates the *full image length*
# against the app partition on every boot and refuses to boot an image that
# spills past it - an endless rst:0x3 reset loop with no error visible on
# USB-only boards. So the on-disk .bin size, not PlatformIO's figure, is the
# number that matters.
Import("env")

import csv
import os


def get_app_partition_size():
    partitions = env.BoardConfig().get("build.partitions", "default.csv")
    path = partitions
    if not os.path.isabs(path):
        candidate = os.path.join(env.subst("$PROJECT_DIR"), partitions)
        if os.path.exists(candidate):
            path = candidate
        else:
            framework_dir = env.PioPlatform().get_package_dir(
                "framework-arduinoespressif32"
            )
            path = os.path.join(framework_dir, "tools", "partitions", partitions)
    sizes = []
    with open(path) as f:
        for row in csv.reader(f):
            if not row or row[0].strip().startswith("#") or len(row) < 5:
                continue
            if row[1].strip() == "app":
                sizes.append(int(row[4].strip(), 0))
    if not sizes:
        raise RuntimeError("No app partitions found in " + path)
    return min(sizes), path


def check_bin_size(source, target, env):
    bin_path = env.subst("$BUILD_DIR/${PROGNAME}.bin")
    bin_size = os.path.getsize(bin_path)
    app_size, table = get_app_partition_size()
    pct = bin_size / app_size * 100
    print(
        "APPSIZE: firmware.bin is %d bytes, app partition is %d bytes (%.1f%%)"
        % (bin_size, app_size, pct)
    )
    if bin_size > app_size:
        print(
            "APPSIZE: ERROR - firmware.bin exceeds the app partition in %s by "
            "%d bytes. The bootloader will refuse to boot this image "
            "(silent rst:0x3 boot loop). Shrink the firmware or grow the app "
            "partitions." % (table, bin_size - app_size)
        )
        env.Exit(1)


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", check_bin_size)
