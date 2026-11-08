"""teeny.protocol -- shared Teeny <-> host protocol helpers.

Frame layout:
    [0]    MAGIC 0x47
    [1]    TYPE
    [2:3]  LEN (payload length, <= 512)
    [4]    SEQ
    [5:5+LEN]  PAYLOAD
    [5+LEN]    CKSUM = XOR of bytes 1..payload_end (MAGIC excluded)
"""

MAGIC = 0x47
MAX_PAYLOAD = 512
HEADER = 5
CKSUM_BYTES = 1

PKT_NONE = 0x00
PKT_BOOT = 0x01
PKT_STATUS = 0x02
PKT_LOG = 0x03
PKT_CMD_REQ = 0x10
PKT_CMD_RSP = 0x11
PKT_FRAME = 0x20
PKT_INPUT = 0x21
PKT_FPS = 0x22
PKT_FRAME_END = 0x23
PKT_FRAME_RLE = 0x24
PKT_RESET = 0x30

PKT_NAMES = {
    PKT_BOOT: "BOOT", PKT_STATUS: "STATUS", PKT_LOG: "LOG",
    PKT_CMD_REQ: "CMD_REQ", PKT_CMD_RSP: "CMD_RSP",
    PKT_FRAME: "FRAME", PKT_INPUT: "INPUT", PKT_FPS: "FPS",
    PKT_FRAME_END: "FRAME_END", PKT_FRAME_RLE: "FRAME_RLE",
    PKT_RESET: "RESET",
}

DISPLAY_WIDTH = 160
DISPLAY_HEIGHT = 144


class Packet:
    __slots__ = ("type", "seq", "payload")

    def __init__(self, type_, seq, payload):
        self.type = type_
        self.seq = seq
        self.payload = payload

    def name(self):
        return PKT_NAMES.get(self.type, f"0x{self.type:02x}")


def encode(type_, payload=b"", seq=0):
    n = len(payload)
    head = bytes([MAGIC, type_ & 0xFF, (n >> 8) & 0xFF, n & 0xFF, seq & 0xFF])
    cksum = 0
    for b in bytes([type_ & 0xFF, (n >> 8) & 0xFF, n & 0xFF, seq & 0xFF]) + bytes(payload):
        cksum ^= b
    return head + bytes(payload) + bytes([cksum])


def decode_rle_row(data):
    if not data or data[0] >= DISPLAY_HEIGHT:
        raise ValueError("bad RLE row y")
    out = bytearray()
    body = data[1:]
    i = 0
    while i < len(body):
        token = body[i]
        i += 1
        if token & 0x80:
            lit = (token & 0x7F) + 1
            if i + lit * 2 > len(body):
                raise ValueError("truncated RLE literal")
            out += body[i:i + lit * 2]
            i += lit * 2
        else:
            if i + 2 > len(body):
                raise ValueError("truncated RLE run")
            out += body[i:i + 2] * token
            i += 2
    if len(out) != DISPLAY_WIDTH * 2:
        raise ValueError(f"RLE row expands to {len(out)} bytes")
    return bytes(out)


class FramedParser:
    def __init__(self):
        self.buf = bytearray()
        self.drops = 0
        self.packets = 0

    def feed(self, data):
        self.buf.extend(data)
        out = []
        while True:
            idx = self.buf.find(bytes([MAGIC]))
            if idx < 0:
                self.buf.clear()
                break
            if idx > 0:
                self.drops += idx
                del self.buf[:idx]
            if len(self.buf) < HEADER:
                break
            header = self.buf[1:HEADER]
            payload_len = (header[1] << 8) | header[2]
            if payload_len > MAX_PAYLOAD:
                self.drops += 1
                del self.buf[0]
                continue
            need = HEADER + payload_len + CKSUM_BYTES
            if len(self.buf) < need:
                break
            frame = bytes(self.buf[:need])
            del self.buf[:need]
            cksum = 0
            for b in frame[1:-1]:
                cksum ^= b
            if cksum != frame[-1]:
                self.drops += 1
                continue
            self.packets += 1
            out.append(Packet(frame[1], frame[4], frame[HEADER:-1]))
        return out
