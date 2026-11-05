#!/usr/bin/env python3
"""Self-test of afp_pcap.py: write a synthetic capture with known requests,
run the parser on it and check what it reports.

usage: python3 -I test_afp_pcap.py   (exit 0 when every check passes)
"""

import csv
import os
import pathlib
import re
import struct
import subprocess
import sys
import tempfile

HERE = pathlib.Path(__file__).resolve().parent
CLIENT, SERVER = bytes([10, 0, 0, 2]), bytes([10, 0, 0, 1])
CPORT, SPORT = 50000, 548


def path3(names):
    """A UTF-8 AFP path (type 3) from bytes with NUL separators."""
    return struct.pack(">BIH", 3, 0x08000103, len(names)) + names


def getfiledirparms(did, names):
    return struct.pack(">BBHIHH", 34, 0, 1, did, 0x0001, 0x0100) + \
        path3(names)


def enumext2(did):
    return struct.pack(">BBHIHHHII", 68, 0, 1, did, 0x0001, 0x0100, 1000, 1,
                       65536) + path3(b"")


def dsi(flags, cmd, reqid, payload, code=0):
    return struct.pack(">BBHiII", flags, cmd, reqid, code, len(payload), 0) \
        + payload


def frame(src, dst, sport, dport, seq, payload, flags=0x18):
    tcp = struct.pack(">HHIIBBHHH", sport, dport, seq, 0, 5 << 4, flags,
                      65535, 0, 0)
    ip = struct.pack(">BBHHHBBH4s4s", 0x45, 0, 20 + len(tcp) + len(payload),
                     0, 0, 64, 6, 0, src, dst)
    eth = b"\x00" * 12 + b"\x08\x00"
    return eth + ip + tcp + payload


def write_capture(path):
    """Requests at known times; the reply comes 1 ms after each request."""
    recs = []
    cseq, sseq, t = 1000, 9000, 100.0
    # the capture starts inside a request: half of a frame first
    stale = dsi(0, 2, 7, getfiledirparms(2, b"stale"))
    recs.append((t, frame(CLIENT, SERVER, CPORT, SPORT, cseq,
                          stale[len(stale) // 2:])))
    cseq += len(stale) - len(stale) // 2
    reqs = [
        getfiledirparms(2, b"navmark-down"),
        enumext2(17),
        getfiledirparms(17, b"Icon\r"),
        getfiledirparms(2, b"navmark-up"),
        getfiledirparms(57, b"a\0\0\0a"),
        enumext2(57),
    ]
    for i, payload in enumerate(reqs):
        t += 0.010
        req = dsi(0, 2, 100 + i, payload)
        recs.append((t, frame(CLIENT, SERVER, CPORT, SPORT, cseq, req)))
        cseq += len(req)
        rep = dsi(1, 2, 100 + i, b"\0" * 20, code=-5018 if i == 2 else 0)
        recs.append((t + 0.001, frame(SERVER, CLIENT, SPORT, CPORT, sseq,
                                      rep)))
        sseq += len(rep)
    write_pcap(path, recs)


def write_pipelined(path):
    """Two requests in one segment: the first reply goes out 1 ms later and
    the second 2 ms after that."""
    reqs = dsi(0, 2, 1, enumext2(17)) + dsi(0, 2, 2, enumext2(18))
    recs = [(200.0, frame(CLIENT, SERVER, CPORT, SPORT, 1000, reqs))]
    sseq = 9000
    for i, t in enumerate((200.001, 200.003)):
        rep = dsi(1, 2, 1 + i, b"\0" * 20)
        recs.append((t, frame(SERVER, CLIENT, SPORT, CPORT, sseq, rep)))
        sseq += len(rep)
    write_pcap(path, recs)


def write_gapped(path):
    """Five requests in their own segments, the second one lost."""
    recs = []
    cseq, sseq = 1000, 9000
    for i in range(5):
        t = 300.0 + i * 0.010
        req = dsi(0, 2, 1 + i, enumext2(1 + i))
        if i != 1:
            recs.append((t, frame(CLIENT, SERVER, CPORT, SPORT, cseq, req)))
        cseq += len(req)
        rep = dsi(1, 2, 1 + i, b"\0" * 20)
        recs.append((t + 0.001, frame(SERVER, CLIENT, SPORT, CPORT, sseq,
                                      rep)))
        sseq += len(rep)
    write_pcap(path, recs)


def same_token(path):
    """Whether a path "x/../../x" shows one token for both names."""
    parts = path.split("/")
    return len(parts) == 4 and parts[0] == parts[3] \
        and re.fullmatch(r"n[0-9a-f]{10}", parts[0]) is not None


def write_pcap(path, recs):
    with open(path, "wb") as f:
        f.write(struct.pack("<IHHiIII", 0xa1b2c3d4, 2, 4, 0, 0, 262144, 1))
        for ts, data in recs:
            sec = int(ts)
            usec = int(round((ts - sec) * 1e6))
            f.write(struct.pack("<IIII", sec, usec, len(data), len(data)))
            f.write(data)


def main():
    with tempfile.TemporaryDirectory() as tmp:
        cap = os.path.join(tmp, "t.pcap")
        out_csv = os.path.join(tmp, "t.csv")
        write_capture(cap)
        res = subprocess.run([sys.executable, "-I", str(HERE / "afp_pcap.py"),
                              "--keep-names", "--csv", out_csv, cap],
                             capture_output=True, text=True)
        text = res.stdout
        with open(out_csv, newline="") as f:
            rows = list(csv.DictReader(f))
        # the same capture with the names replaced by tokens
        tok_csv = os.path.join(tmp, "tok.csv")
        subprocess.run([sys.executable, "-I", str(HERE / "afp_pcap.py"),
                        "--csv", tok_csv, cap], capture_output=True, text=True)
        with open(tok_csv, newline="") as f:
            tok_rows = list(csv.DictReader(f))
        # the same capture cut off five bytes into its last record's frame,
        # too short for the Ethernet header
        cut = os.path.join(tmp, "cut.pcap")
        with open(cap, "rb") as f:
            whole = f.read()
        pos = last = 24
        while pos + 16 <= len(whole):
            last = pos
            pos += 16 + struct.unpack("<I", whole[pos + 8:pos + 12])[0]
        with open(cut, "wb") as f:
            f.write(whole[:last + 16 + 5])
        cut_res = subprocess.run([sys.executable, "-I",
                                  str(HERE / "afp_pcap.py"), cut],
                                 capture_output=True, text=True)
        piped, piped_csv = (os.path.join(tmp, "piped.pcap"),
                            os.path.join(tmp, "piped.csv"))
        write_pipelined(piped)
        subprocess.run([sys.executable, "-I", str(HERE / "afp_pcap.py"),
                        "--csv", piped_csv, piped],
                       capture_output=True, text=True)
        with open(piped_csv, newline="") as f:
            piped_rows = list(csv.DictReader(f))
        gap, gap_csv = (os.path.join(tmp, "gap.pcap"),
                        os.path.join(tmp, "gap.csv"))
        write_gapped(gap)
        gap_res = subprocess.run([sys.executable, "-I",
                                  str(HERE / "afp_pcap.py"), "--csv", gap_csv,
                                  gap], capture_output=True, text=True)
        with open(gap_csv, newline="") as f:
            gap_rows = list(csv.DictReader(f))
    work = [r for r in rows if "navmark" not in r["path"]]
    checks = [
        ("parser exits 0", res.returncode == 0),
        ("two phases from the markers",
         "== phase down:" in text and "== phase up:" in text),
        ("down holds two requests", "phase down: 2 requests" in text),
        ("up holds two requests", "phase up: 2 requests" in text),
        ("server time is the 1 ms gap",
         all(r["server_us"] == "1000" for r in rows)),
        ("the client gap is the 9 ms between them",
         all(r["client_gap_us"] == "9000" for r in work)),
        ("the error reply keeps its code",
         [r["result"] for r in work if r["path"] == "Icon\\r"] == ["-5018"]),
        ("two parent hops decoded",
         [(r["names"], r["hops"], r["path"]) for r in work
          if r["did"] == "57" and r["cmd"] == "FPGetFileDirParms"]
         == [("2", "2", "a/../../a")]),
        ("half frame skipped", not any("stale" in r["path"] for r in rows)),
        ("six requests in the CSV", len(rows) == 6),
        ("a capture cut inside a record still parses",
         cut_res.returncode == 0 and "== phase down:" in cut_res.stdout),
        ("a queued request's server time starts at the reply before it",
         [r["server_us"] for r in piped_rows] == ["1000", "2000"]),
        ("names are tokens by default, markers and Icon\\r kept",
         [r["path"] for r in tok_rows
          if r["did"] == "17" and r["cmd"] == "FPGetFileDirParms"]
         == ["Icon\\r"]
         and sum("navmark-" in r["path"] for r in tok_rows) == 2
         and [same_token(r["path"]) for r in tok_rows
              if r["did"] == "57" and r["cmd"] == "FPGetFileDirParms"]
         == [True]),
        ("requests after a lost client segment are parsed",
         gap_res.returncode == 0 and "lost bytes" in gap_res.stderr
         and sorted(int(r["did"]) for r in gap_rows) == [1, 3, 4, 5]),
        ("the connection is numbered, not addressed",
         all(r["conn"] == "c1" for r in rows)),
    ]
    failed = [name for name, ok in checks if not ok]
    for name, ok in checks:
        print(("ok     " if ok else "FAILED ") + name)
    if failed:
        print(text)
        print(res.stderr)
        for r in rows:
            print(r)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
