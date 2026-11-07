"""A real command tree for the graph's timeout regression, never a VM."""
import json
import os
from pathlib import Path
import signal
import sys
import time

if sys.argv[2] == "stubborn":
    signal.signal(signal.SIGTERM, signal.SIG_IGN)
child = os.fork()
if child:
    pids = [os.getpid(), child]
    records = {pid: Path(f"/proc/{pid}/stat").read_text().rsplit(")", 1)[1].split()[19]
               for pid in pids}
    Path(sys.argv[1]).write_text(json.dumps(records))
while True:
    time.sleep(60)
