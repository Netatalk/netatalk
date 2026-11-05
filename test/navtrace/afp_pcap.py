#!/usr/bin/env python3
"""Per-request view of an AFP-over-TCP capture taken on the server.

usage: afp_pcap.py [-p port] [-l levels] [--csv out.csv] [--keep-names]
                   capture.pcap

Reads a classic pcap file (tcpdump -w), reassembles each TCP connection to
the AFP port, splits both directions into DSI frames, pairs every request
with its reply by DSI request ID and prints, per phase:

  - requests and server time per AFP command
  - the time the client spent between a reply and its next request
  - how many names the request paths carry, and how many parent hops
  - the DIDs requested most often

Server time is the reply's first byte minus the later of the request's last
byte and the previous reply's last byte on the connection, since afpd serves
one request at a time; all times are as the server's capture saw them.
Phases start at a request whose path names
"navmark-<label>", which the Mac sends with
  stat /Volumes/<share>/navmark-<label>
Without markers the whole capture is one phase.

Names are per-run tokens unless --keep-names: each distinct name maps to one
token through a keyed hash whose key is random and never written, so the
report shows the paths' shape and which names repeat, and no name. The
markers and the Finder's own names (Icon\\r, .DS_Store, ...) stay as they
are. Connections are numbered in the order of their first packet.

No third-party module is needed.  Run it with python3 -I.
"""

import argparse
import bisect
import collections
import csv
import hashlib
import secrets
import struct
import sys

AFP_CMDS = {
    1: "FPByteRangeLock", 2: "FPCloseVol", 3: "FPCloseDir", 4: "FPCloseFork",
    5: "FPCopyFile", 6: "FPCreateDir", 7: "FPCreateFile", 8: "FPDelete",
    9: "FPEnumerate", 10: "FPFlush", 11: "FPFlushFork",
    14: "FPGetForkParms", 15: "FPGetSrvrInfo", 16: "FPGetSrvrParms",
    17: "FPGetVolParms", 18: "FPLogin", 19: "FPLoginCont", 20: "FPLogout",
    21: "FPMapID", 22: "FPMapName", 23: "FPMoveAndRename", 24: "FPOpenVol",
    25: "FPOpenDir", 26: "FPOpenFork", 27: "FPRead", 28: "FPRename",
    29: "FPSetDirParms", 30: "FPSetFileParms", 31: "FPSetForkParms",
    32: "FPSetVolParms", 33: "FPWrite", 34: "FPGetFileDirParms",
    35: "FPSetFileDirParms", 36: "FPChangePassword", 37: "FPGetUserInfo",
    38: "FPGetSrvrMsg", 39: "FPCreateID", 40: "FPDeleteID",
    41: "FPResolveID", 42: "FPExchangeFiles", 43: "FPCatSearch",
    48: "FPOpenDT", 49: "FPCloseDT", 51: "FPGetIcon", 52: "FPGetIconInfo",
    53: "FPAddAPPL", 54: "FPRemoveAPPL", 55: "FPGetAPPL", 56: "FPAddComment",
    57: "FPRemoveComment", 58: "FPGetComment", 59: "FPByteRangeLockExt",
    60: "FPReadExt", 61: "FPWriteExt", 62: "FPGetAuthMethods",
    63: "FPLoginExt", 64: "FPGetSessionToken", 65: "FPDisconnectOldSession",
    66: "FPEnumerateExt", 67: "FPCatSearchExt", 68: "FPEnumerateExt2",
    69: "FPGetExtAttr", 70: "FPSetExtAttr", 71: "FPRemoveExtAttr",
    72: "FPListExtAttrs", 73: "FPGetACL", 74: "FPSetACL", 75: "FPAccess",
    76: "FPSpotlightRPC", 78: "FPSyncDir", 79: "FPSyncFork",
    122: "FPZzzzz", 192: "FPAddIcon",
}

DSI_CMDS = {1: "DSICloseSession", 2: "DSICommand", 3: "DSIGetStatus",
            4: "DSIOpenSession", 5: "DSITickle", 6: "DSIWrite",
            8: "DSIAttention"}

# names the Finder makes itself, which no token needs to hide
KEEP_NAMES = {"Icon\r", ".DS_Store", ".Trashes", ".TemporaryItems",
              "Temporary Items", "Network Trash Folder", ".Spotlight-V100",
              ".fseventsd", ".metadata_never_index", ".VolumeIcon.icns",
              ".com.apple.timemachine.donotpresent", ".localized"}


class Names:
    """Replace names by per-run tokens, one token per distinct name."""

    def __init__(self, keep):
        self.keep = keep
        self.key = secrets.token_bytes(16)

    def token(self, name):
        if self.keep or name in KEEP_NAMES or name.startswith("navmark-"):
            return name
        h = hashlib.blake2b(name.encode("utf-8"), key=self.key, digest_size=5)
        return "n" + h.hexdigest()

# segments that may wait behind a lost one before zeros fill its place
HOLE_SEGMENTS = 256
# the largest DSI frame, netatalk's largest server quantum: resync() looks
# that far for frames that line up, and a hole beyond it is a lost stream
MAX_FRAME = 256 * 1024 * 1024
RESYNC_WINDOW = MAX_FRAME

# offset of the path type byte in the AFP request, for requests that carry a
# volume, a DID and a path; the DID sits at offset 4 in all of them
PATH_AT = {
    3: None, 6: 8, 7: 8, 8: 8, 9: 18, 23: None, 25: 8, 26: 12, 28: 8,
    29: 10, 30: 10, 34: 12, 35: 10, 66: 18, 68: 22, 69: 30, 72: 20,
    73: 14, 75: 30, 78: None,
}


def read_pcap(path):
    """Yield (timestamp, link payload) for each record of a classic pcap."""
    with open(path, "rb") as f:
        hdr = f.read(24)
        if len(hdr) < 24:
            sys.exit("not a pcap file")
        magic = struct.unpack("<I", hdr[:4])[0]
        if magic in (0xa1b2c3d4, 0xa1b23c4d):
            endian = "<"
        elif magic in (0xd4c3b2a1, 0x4d3cb2a1):
            endian = ">"
        else:
            sys.exit("not a classic pcap file (pcapng? convert with "
                     "'tcpdump -r in -w out' or editcap -F pcap)")
        nano = magic in (0xa1b23c4d, 0x4d3cb2a1)
        linktype = struct.unpack(endian + "I", hdr[20:24])[0]
        while True:
            rec = f.read(16)
            if len(rec) < 16:
                return
            sec, frac, incl, _orig = struct.unpack(endian + "IIII", rec)
            data = f.read(incl)
            if len(data) < incl:            # the capture was cut off
                return
            ts = sec + frac / (1e9 if nano else 1e6)
            yield linktype, ts, data


def ip_payload(linktype, data):
    """The IP packet inside a link-layer frame, or None."""
    if linktype == 1:                       # Ethernet
        if len(data) < 14:
            return None
        off, etype = 14, struct.unpack(">H", data[12:14])[0]
        while etype in (0x8100, 0x88a8):    # VLAN tags
            if len(data) < off + 4:
                return None
            etype = struct.unpack(">H", data[off + 2:off + 4])[0]
            off += 4
        return data[off:] if etype in (0x0800, 0x86dd) else None
    if linktype == 0:                       # BSD loopback
        if len(data) < 4:
            return None
        family = struct.unpack("<I", data[:4])[0]
        if family not in (2, 24, 28, 30):
            family = struct.unpack(">I", data[:4])[0]
        return data[4:] if family in (2, 24, 28, 30) else None
    if linktype == 113:                     # Linux cooked
        return data[16:]
    if linktype == 276:                     # Linux cooked v2
        return data[20:]
    if linktype in (101, 12, 14):           # raw IP
        return data
    return None


def tcp_segment(ip):
    """(src, dst, sport, dport, seq, flags, payload) of a TCP packet."""
    if not ip:
        return None
    version = ip[0] >> 4
    if version == 4:
        ihl = (ip[0] & 0x0f) * 4
        if len(ip) < 20 or ihl < 20 or ip[9] != 6:
            return None
        total = struct.unpack(">H", ip[2:4])[0]
        src, dst = ip[12:16], ip[16:20]
        tcp = ip[ihl:total] if total >= ihl else ip[ihl:]
    elif version == 6:
        if len(ip) < 40:
            return None
        plen = struct.unpack(">H", ip[4:6])[0]
        src, dst = ip[8:24], ip[24:40]
        nxt, off = ip[6], 40
        # hop-by-hop, routing and destination options carry their length;
        # a fragment header is eight bytes
        while nxt in (0, 43, 44, 60):
            if len(ip) < off + 2:
                return None
            step = 8 if nxt == 44 else (ip[off + 1] + 1) * 8
            nxt, off = ip[off], off + step
        if nxt != 6:
            return None
        tcp = ip[off:40 + plen]
    else:
        return None
    if len(tcp) < 20:
        return None
    sport, dport, seq = struct.unpack(">HHI", tcp[:8])
    doff = (tcp[12] >> 4) * 4
    flags = tcp[13]
    return src, dst, sport, dport, seq, flags, tcp[doff:]


class Stream:
    """One direction of a TCP connection, in order, with byte timestamps."""

    def __init__(self):
        self.data = bytearray()
        self.base = None         # sequence number of data[0]
        self.ends = []           # end offset of each appended segment
        self.times = []          # its timestamp
        self.pending = {}        # out-of-order segments by offset
        self.gaps = 0
        self.holes = set()       # offsets where zeros stand for lost bytes

    def add(self, seq, flags, payload, ts):
        if self.base is None:
            self.base = (seq + 1) & 0xffffffff if flags & 0x02 else seq
            if flags & 0x02:
                return
        if not payload:
            return
        off = (seq - self.base) & 0xffffffff
        if off > 0x7fffffff:     # before the stream's start: retransmission
            return
        self.pending.setdefault(off, (payload, ts))
        self._drain()

    def _drain(self, fill=False):
        while self.pending:
            have = len(self.data)
            ready = [o for o in self.pending if o <= have]
            if not ready:
                # a segment the capture lost: once enough wait behind it, or
                # at the end, zeros take its place, so later bytes keep their
                # offsets, and dsi_frames() resyncs past them
                if not fill and len(self.pending) < HOLE_SEGMENTS:
                    return
                nxt = min(self.pending)
                if nxt - have > MAX_FRAME:
                    # a jump no frame spans: the stream is lost from here
                    self.pending.clear()
                    self.gaps += 1
                    return
                self.holes.add(have)
                self.data += bytes(nxt - have)
                self.ends.append(len(self.data))
                self.times.append(self.pending[nxt][1])
                self.gaps += 1
                continue
            for o in sorted(ready):
                payload, ts = self.pending.pop(o)
                tail = payload[have - o:] if o < have else payload
                if tail:
                    self.data += tail
                    self.ends.append(len(self.data))
                    self.times.append(ts)
                have = len(self.data)

    def finish(self):
        self._drain(fill=True)

    def time_at(self, offset):
        """Timestamp of the segment that carried byte offset."""
        i = bisect.bisect_right(self.ends, offset)
        return self.times[min(i, len(self.times) - 1)]


def frame_at(data, pos):
    """The DSI header fields at pos, or None where no header can start."""
    if pos + 16 > len(data):
        return None
    flags, cmd, reqid, code, length, reserved = struct.unpack(
        ">BBHiII", data[pos:pos + 16])
    if flags > 1 or cmd not in DSI_CMDS or reserved != 0 \
            or length > MAX_FRAME:
        return None
    # a request's payload starts with an AFP command
    if flags == 0 and cmd in (2, 6) and (
            length == 0 or (pos + 16 < len(data)
                            and data[pos + 16] not in AFP_CMDS)):
        return None
    return flags, cmd, reqid, code, length


def resync(data, pos, holes=frozenset()):
    """The first offset from pos where three frames follow one another, or
    frames that end at the capture's end or at a hole, or whole frames and
    then one the capture cut."""
    for start in range(pos, min(len(data), pos + RESYNC_WINDOW)):
        p, whole = start, 0
        while whole < 3:
            f = frame_at(data, p)
            if f is None:
                break
            p += 16 + f[4]
            if p > len(data):
                if whole:
                    whole = 3
                break
            whole += 1
            if p == len(data) or p in holes:
                whole = 3
        if whole >= 3:
            return start
    return None


def dsi_frames(stream):
    """Yield (start, end, flags, command, reqid, code, payload) frames.

    A capture that starts inside a session, or that lost bytes, resumes at
    the next offset where frames line up."""
    data, pos = stream.data, resync(stream.data, 0, stream.holes)
    while pos is not None and pos + 16 <= len(data):
        f = frame_at(data, pos)
        if f is None:
            stream.gaps += 1
            pos = resync(data, pos + 1, stream.holes)
            continue
        flags, cmd, reqid, code, length = f
        end = pos + 16 + length
        if end > len(data):
            return               # the capture ended inside the frame
        yield pos, end, flags, cmd, reqid, code, bytes(data[pos + 16:end])
        pos = end


def decode_path(buf, at, tokens):
    """(names, hops, text) of the AFP path whose type byte is at offset at.

    As the server reads it: the first NUL of a run separates two names, and
    each further NUL of the run climbs to the parent. tokens gives each name
    the text shows."""
    if at is None or at >= len(buf):
        return 0, 0, ""
    ptype = buf[at]
    if ptype == 3 and at + 7 <= len(buf):
        n = struct.unpack(">H", buf[at + 5:at + 7])[0]
        raw = buf[at + 7:at + 7 + n]
    elif ptype in (1, 2) and at + 2 <= len(buf):
        raw = buf[at + 2:at + 2 + buf[at + 1]]
    else:
        return 0, 0, ""
    names, hops, out, i = 0, 0, [], 0
    while i < len(raw):
        if raw[i] == 0:
            run = 0
            while i < len(raw) and raw[i] == 0:
                run += 1
                i += 1
            hops += run - 1
            out.extend([".."] * (run - 1))
            continue
        j = raw.find(b"\0", i)
        j = len(raw) if j < 0 else j
        out.append(tokens.token(raw[i:j].decode("utf-8", "replace"))
                   .replace("\r", "\\r"))
        names += 1
        i = j
    return names, hops, "/".join(out)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("pcap")
    ap.add_argument("-p", "--port", type=int, default=548)
    ap.add_argument("-l", "--levels", type=int, default=0,
                    help="folder levels walked in each phase, for per-level "
                         "figures")
    ap.add_argument("--csv", help="write every request to this CSV file")
    ap.add_argument("--keep-names", action="store_true",
                    help="show the names in the paths, not per-run tokens")
    args = ap.parse_args()
    tokens = Names(args.keep_names)

    conns = collections.OrderedDict()
    for linktype, ts, data in read_pcap(args.pcap):
        seg = tcp_segment(ip_payload(linktype, data))
        if seg is None:
            continue
        src, dst, sport, dport, seq, flags, payload = seg
        if dport == args.port:
            key, side = (src, sport, dst, dport), "c2s"
        elif sport == args.port:
            key, side = (dst, dport, src, sport), "s2c"
        else:
            continue
        # a client SYN on a four-tuple that carried data opens a new
        # connection: the old one keeps its label under another key
        if side == "c2s" and flags & 0x02 and not flags & 0x10 \
                and key in conns and conns[key]["c2s"].base is not None:
            conns[key + (len(conns),)] = conns.pop(key)
        conn = conns.get(key)
        if conn is None:
            conn = {"c2s": Stream(), "s2c": Stream(),
                    "label": "c%d" % (len(conns) + 1)}
            conns[key] = conn
        conn[side].add(seq, flags, payload, ts)

    rows = []
    for key, conn in conns.items():
        for s in (conn["c2s"], conn["s2c"]):
            s.finish()
        replies = {}
        for start, end, flags, cmd, reqid, code, payload in \
                dsi_frames(conn["s2c"]):
            if flags == 1:
                replies.setdefault(reqid, []).append(
                    (conn["s2c"].time_at(start), conn["s2c"].time_at(end - 1),
                     code))
        for start, end, flags, cmd, reqid, code, payload in \
                dsi_frames(conn["c2s"]):
            if flags != 0 or cmd not in (2, 6) or not payload:
                continue
            afp = payload[0]
            t_req = conn["c2s"].time_at(end - 1)
            t_first = conn["c2s"].time_at(start)
            did = vol = None
            if afp in PATH_AT and len(payload) >= 8:
                vol, did = struct.unpack(">HI", payload[2:8])
            names, hops, path = decode_path(payload, PATH_AT.get(afp), tokens)
            reply = None
            for r in replies.get(reqid, []):
                if r[0] >= t_req:
                    reply = r
                    replies[reqid].remove(r)
                    break
            rows.append({
                "conn": conn["label"],
                "t": t_first, "t_req_end": t_req,
                "t_reply": reply[0] if reply else None,
                "t_reply_end": reply[1] if reply else None,
                "cmd": AFP_CMDS.get(afp, "AFP%d" % afp),
                "result": reply[2] if reply else None,
                "vol": vol, "did": did, "names": names, "hops": hops,
                "path": path,
            })

    rows.sort(key=lambda r: r["t"])
    phase = "all"
    last_end = {}
    for r in rows:
        if "navmark-" in r["path"]:
            phase = r["path"].split("navmark-", 1)[1].split("/")[0] or "all"
            r["marker"] = True
        r["phase"] = phase
        prev = last_end.get(r["conn"])
        start = r["t_req_end"] if prev is None else max(r["t_req_end"], prev)
        r["server_us"] = ((r["t_reply"] - start) * 1e6
                          if r["t_reply"] is not None else None)
        r["client_gap_us"] = (r["t"] - prev) * 1e6 if prev is not None \
            and r["t"] >= prev else None
        if r["t_reply_end"] is not None:
            last_end[r["conn"]] = r["t_reply_end"]

    for conn in conns.values():
        for side in ("c2s", "s2c"):
            if conn[side].gaps:
                print("warning: %s %s: lost bytes, frames after the loss "
                      "are skipped; capture with a larger tcpdump -B"
                      % (conn["label"], side), file=sys.stderr)

    if args.csv:
        with open(args.csv, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["t", "phase", "conn", "cmd", "result", "vol", "did",
                        "names", "hops", "path", "server_us",
                        "client_gap_us"])
            for r in rows:
                w.writerow(["%.6f" % r["t"], r["phase"], r["conn"], r["cmd"],
                            r["result"], r["vol"], r["did"], r["names"],
                            r["hops"], r["path"],
                            "" if r["server_us"] is None
                            else "%.0f" % r["server_us"],
                            "" if r["client_gap_us"] is None
                            else "%.0f" % r["client_gap_us"]])

    report(rows, args.levels)


def report(rows, levels):
    phases = collections.OrderedDict()
    for r in rows:
        if not r.get("marker"):
            phases.setdefault(r["phase"], []).append(r)
    if not phases:
        print("no AFP requests found")
        return
    for phase, rs in phases.items():
        wall = (max(r["t_reply_end"] or r["t"] for r in rs) - rs[0]["t"])
        server = sum(r["server_us"] or 0 for r in rs)
        gaps = sum(r["client_gap_us"] or 0 for r in rs)
        unanswered = sum(1 for r in rs if r["server_us"] is None)
        print("== phase %s: %d requests, %.3f s wall, %.3f s server, "
              "%.3f s client gaps, %d unanswered"
              % (phase, len(rs), wall, server / 1e6, gaps / 1e6, unanswered))
        if levels:
            print("   per level: %.1f requests, %.1f ms server"
                  % (len(rs) / levels, server / 1e3 / levels))
        by = collections.OrderedDict()
        for r in rs:
            by.setdefault(r["cmd"], []).append(r)
        print("   %-20s %6s %10s %9s %9s %7s" % (
            "command", "n", "server_ms", "mean_us", "max_us", "errors"))
        for cmd, cr in sorted(by.items(), key=lambda kv: -sum(
                r["server_us"] or 0 for r in kv[1])):
            lat = [r["server_us"] for r in cr if r["server_us"] is not None]
            errs = sum(1 for r in cr if r["result"] not in (None, 0))
            print("   %-20s %6d %10.1f %9.0f %9.0f %7d" % (
                cmd, len(cr), sum(lat) / 1e3,
                sum(lat) / len(lat) if lat else 0, max(lat) if lat else 0,
                errs))
        hist = collections.Counter((r["names"], r["hops"]) for r in rs
                                   if r["path"] or r["did"] is not None)
        if hist:
            print("   path shape (names, parent hops): " + ", ".join(
                "%s x%d" % (k, v) for k, v in sorted(hist.items())))
        dids = collections.Counter(r["did"] for r in rs
                                   if r["did"] is not None)
        if dids:
            print("   most requested DIDs: " + ", ".join(
                "%d x%d" % (d, n) for d, n in dids.most_common(8)))
        slow = sorted((r for r in rs if r["server_us"] is not None),
                      key=lambda r: -r["server_us"])[:5]
        for r in slow:
            print("   slow: %8.0f us %-18s did %s path %r" % (
                r["server_us"], r["cmd"], r["did"], r["path"]))


if __name__ == "__main__":
    main()
