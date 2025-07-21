import requests
import json
import secrets
import string
import subprocess
import getpass
import re

EMQX_API = "https://emqx-dashboard.prod.tumogroup.com/api/v5"
EMQX_USER = "admin"

def random_password(length=16):
    return ''.join(secrets.choice(string.ascii_letters + string.digits) for _ in range(length))

def get_device_mac(port):
    result = subprocess.run(
        f'python -m esptool --port {port} read_mac',
        capture_output=True, text=True, shell=True
    )
    output = result.stdout + result.stderr
    print(output)
    for line in output.splitlines():
        if "MAC" in line:
            match = re.search(r'([0-9A-Fa-f]{2}[:]){5}[0-9A-Fa-f]{2}', line)
            if match:
                mac = match.group(0).replace(":", "").upper()
                print(f"DEBUG: Extracted MAC: {mac}")
                return mac
            match = re.search(r'[0-9A-Fa-f]{12}', line.replace(":", ""))
            if match:
                mac = match.group(0).upper()
                print(f"DEBUG: Extracted MAC (no colons): {mac}")
                return mac
    raise RuntimeError("Could not read MAC address from device.")

def provision_device(device_id, emqx_pass):
    username = f"dev_{device_id}"
    password = random_password()

    # 1. Create user (EMQX 5.x endpoint)
    resp = requests.post(
        f"{EMQX_API}/authentication/password_based:built_in_database/users",
        auth=(EMQX_USER, emqx_pass),
        json={"username": username, "password": password}
    )
    assert resp.status_code in (200, 201), resp.text

    # 2. Set ACL (EMQX 5.x endpoint)
    acl = [
        {"action": "allow", "permission": "publish", "topic": f"garge/devices/+/+/{device_id}/#"},
        {"action": "allow", "permission": "subscribe", "topic": f"garge/devices/+/+/{device_id}/#"},
        {"action": "deny", "permission": "all", "topic": "#"}
    ]
    resp = requests.post(
        f"{EMQX_API}/authorization/sources/built_in_database/rules",
        auth=(EMQX_USER, emqx_pass),
        json={"username": username, "rules": acl}
    )
    assert resp.status_code in (200, 201), resp.text

    # 3. Save credentials for flashing
    creds = {"mqtt_user": username, "mqtt_pass": password}
    with open(f"data/{device_id}_mqtt_creds.json", "w") as f:
        json.dump(creds, f)
    print(f"Provisioned {username}")

if __name__ == "__main__":
    port = input("Enter serial port (e.g., COM3 or /dev/ttyUSB0): ")
    emqx_pass = getpass.getpass("Enter EMQX admin password: ")
    device_id = get_device_mac(port)
    print(f"Detected device ID: {device_id}")
    provision_device(device_id, emqx_pass)