#!/usr/bin/python

import platform
Import("env")

system = platform.system()
if system == "Darwin":
    # I'll leave it up to @thorrak to figure out how to
    # lay this out.
    env["UPLOAD_PORT"] = "/dev/cu.usbserial-*"
    # env["UPLOAD_PORT"] = "/dev/cu.wch*"
elif system == "Windows":
    # Windows users don't need no silly declarations
    # port = "COM*"
    # usbc = "COM*"
    pass
elif system == "Linux":
    # port = "/dev/cu.usbserial-*"
    # usbc = "/dev/cu.wch*"
    pass
else:
    pass
