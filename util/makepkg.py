#!/usr/bin/env python2

import zlib
import os
import os.path

PKGSIZE = 10240

def crc(filename):
    prev = 0
    for eachLine in open(filename, "rb"):
        prev = zlib.crc32(eachLine, prev)
    return "%08X" % (prev & 0xFFFFFFFF)

def main(filename):
    base = os.path.splitext(filename)[0]
    statbuf = os.stat(filename)
    num = statbuf.st_size / PKGSIZE
    if statbuf.st_size % PKGSIZE > 0:
        num += 1
    c = [""] * (num + 1)
    c[0] = crc(filename)
    with open(filename, "rb") as f:
        for i in range(1, num + 1):
            name = "%s_%03d.rar" % (base, i)
            buf = f.read(PKGSIZE)
            with open(name, "wb") as out:
                out.write(buf)
            c[i] = crc(name)
    with open("%s_000.rar" % base, "wb") as f:
        f.write("[CONCENTRATOR]\r\n")
        f.write("FILESIZE=%d\r\n" % statbuf.st_size)
        f.write("CRC=%s\r\n" % c[0])
        f.write("PACKAGE_COUNT=%d\r\n" % num)
        f.write("[PACKAGE]\r\n")
        for i in range(1, num + 1):
            name = "%s_%03d.rar" % (os.path.basename(base), i)
            f.write("%s=%s\r\n" % (name, c[i]))

if __name__ == '__main__':
    import sys
    if len(sys.argv) < 2 or not os.path.exists(sys.argv[1]):
        print 'Usage: makepkg.py filename'
    else:
        main(sys.argv[1])
