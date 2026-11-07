import base64
import json
import ssl
import sys
import urllib.request

endpoint = sys.argv[1]
payload = json.loads(sys.argv[2]) if len(sys.argv) > 2 else None
headers = {"Authorization": "Basic " + base64.b64encode(b"admin:fixture-password").decode()}
if payload is not None:
    headers["Content-Type"] = "application/json"
request = urllib.request.Request(
    "https://localhost:47990" + endpoint,
    data=json.dumps(payload).encode() if payload is not None else None,
    headers=headers,
)
with urllib.request.urlopen(request, context=ssl._create_unverified_context(), timeout=4) as response:
    print(json.dumps(json.load(response)))
