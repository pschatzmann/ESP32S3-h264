#!/usr/bin/env python3
"""
Simple UDP receiver for CameraUDP.ino example.

Usage:
  python3 receiver.py --port 5000 --outdir captured

Behavior:
- If a received UDP packet length is > 3 and we heuristically detect our 3-byte header
  (2-byte seq, 1-byte last-flag), the script will reassemble chunks with the same seq
  until last-flag==1 and then write a .h264 file (frame_<n>.h264).
- Otherwise the packet is treated as a complete NAL and written as a single .h264 file.

This is a minimal example for testing and not robust for production use.
"""

import socket
import argparse
import os
from collections import defaultdict

parser = argparse.ArgumentParser()
parser.add_argument('--port', type=int, default=5000)
parser.add_argument('--outdir', type=str, default='captured')
args = parser.parse_args()

os.makedirs(args.outdir, exist_ok=True)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('0.0.0.0', args.port))
print(f'Listening on UDP port {args.port}...')

# buffer: seq -> bytearray
buffers = defaultdict(bytearray)
frame_counter = 0

while True:
    data, addr = sock.recvfrom(65536)
    if len(data) == 0:
        continue
    # Heuristic: if first 3 bytes look like our header (seq_hi, seq_lo, flags)
    if len(data) >= 3:
        seq = (data[0] << 8) | data[1]
        flags = data[2]
        payload = data[3:]
        # We'll assume it's a chunk if flags is 0 or 1 and payload length < 1400
        if flags in (0, 1) and len(payload) <= 1200:
            buffers[seq] += payload
            if flags & 1:
                # final chunk
                frame_counter += 1
                out_path = os.path.join(args.outdir, f'frame_{frame_counter:05d}.h264')
                with open(out_path, 'wb') as f:
                    f.write(buffers[seq])
                print(f'Wrote {out_path} ({len(buffers[seq])} bytes)')
                del buffers[seq]
            continue
    # otherwise treat as full NAL
    frame_counter += 1
    out_path = os.path.join(args.outdir, f'frame_{frame_counter:05d}.h264')
    with open(out_path, 'wb') as f:
        f.write(data)
    print(f'Wrote {out_path} ({len(data)} bytes)')
