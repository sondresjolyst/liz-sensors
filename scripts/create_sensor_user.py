import psycopg2
import subprocess
import getpass
import re
import os
import secrets
import hvac
from utils import create_or_update_mqtt_user, set_mqtt_acl, hash_password_pbkdf2, generate_salt

garge_name = "garge"
garge_environment = "dev"

mqtt_database = f"{garge_name}-{garge_environment}"

database_host = "tumogroup.com"
database_user_table = "EMQXMqttUsers"
database_acl_table = "EMQXMqttAcls"
database_port = 5432

vault_address = "https://vault.tumogroup.com"
vault_folder_name = f"{garge_name}_sensor_passwords"
vault_kv_name = f"{garge_name}-{garge_environment}"
vault_secret_password_key = "password"
vault_secret_username_key = "username"
vault_github_token = None

serial_port = 'COM14'
serial_speed = 9600

def main():
    global vault_github_token

    pg_user = input("Enter PostgreSQL admin username: ")
    pg_password = getpass.getpass("Enter PostgreSQL admin password: ")

    if not vault_github_token:
        vault_github_token = os.environ.get("VAULT_GITHUB_TOKEN")
    if not vault_github_token:
        vault_github_token = getpass.getpass("Enter your GitHub token for Vault authentication: ")

    try:
        print("Reading ESP MAC address using esptool...")
        result = subprocess.run(
            f'python -m esptool --port {serial_port} read_mac',
            capture_output=True, text=True, shell=True
        )
        if result.returncode != 0:
            raise Exception(f"esptool error: {result.stderr.strip()}")
        # Look for MAC address in esptool output
        mac_match = re.search(r'MAC: ([0-9A-Fa-f:]+)', result.stdout)
        if not mac_match:
            raise Exception(f"No MAC address found in esptool output: '{result.stdout}'")
        mac = mac_match.group(1).replace(':', '').lower()
        sensor_name = f"{garge_name}_{mac}"
        print(f"Sensor MAC address received: {mac}")
        print(f"Username to be created: {sensor_name}")
    except Exception as e:
        print(f"Error reading MAC address: {e}")
        exit(1)

    vault_path = f"{vault_folder_name}/{sensor_name}"

    try:
        client = hvac.Client(url=vault_address)
        login_response = client.auth.github.login(token=vault_github_token)
        if not client.is_authenticated():
            raise Exception("Vault authentication failed.")
        existing_secret = None
        try:
            existing_secret = client.secrets.kv.v2.read_secret_version(
                path=vault_path,
                mount_point=vault_kv_name,
                raise_on_deleted_version=False
            )
        except hvac.exceptions.InvalidPath:
            pass  # Secret does not exist

        # Check if the secret exists and is not deleted
        action = None
        if existing_secret and existing_secret.get('data') and existing_secret['data'].get('data'):
            print(f"Credentials already exist in Vault for {sensor_name}.")
            choice = input("Use existing credentials? (y/n): ").strip().lower()
            if choice == 'y':
                mqtt_password = existing_secret['data']['data'][vault_secret_password_key]
                print(f"Using existing password for {sensor_name} from Vault.")
            else:
                mqtt_password = secrets.token_urlsafe(16)
                client.secrets.kv.v2.create_or_update_secret(
                    path=vault_path,
                    secret={vault_secret_password_key: mqtt_password, vault_secret_username_key: sensor_name},
                    mount_point=vault_kv_name
                )
                action = 'updated'
                print(f"New password for {sensor_name} stored in Vault at {vault_kv_name}/{vault_path}")
        else:
            mqtt_password = secrets.token_urlsafe(16)
            client.secrets.kv.v2.create_or_update_secret(
                path=vault_path,
                secret={vault_secret_password_key: mqtt_password, vault_secret_username_key: sensor_name},
                mount_point=vault_kv_name
            )
            action = 'created'
            print(f"Password for {sensor_name} stored in Vault at {vault_kv_name}/{vault_path}")
    except Exception as e:
        print(f"Vault error: {e}")
        exit(1)

    salt = generate_salt()
    password_hash = hash_password_pbkdf2(mqtt_password, salt)

    sensor_acl = [
    {"action": "all", "permission": "allow", "topic": f"garge/devices/{garge_name}_{mac}/#", "qos": 0, "retain": 1},
    {"action": "all", "permission": "allow", "topic": f"garge/devices/{garge_name}_{mac}/#", "qos": 0, "retain": 0},
]

    try:
        db_connection = psycopg2.connect(
            dbname=mqtt_database,
            user=pg_user,
            password=pg_password,
            host=database_host,
            port=database_port
        )
        create_or_update_mqtt_user(
            db_connection,
            database_user_table,
            sensor_name,
            salt,
            password_hash,
            False
        )

        set_mqtt_acl(
            db_connection,
            database_acl_table,
            sensor_name,
            sensor_acl
        )
        db_connection.commit()
        if action == 'created':
            print(f"MQTT user '{sensor_name}' created in database.")
        elif action == 'updated':
            print(f"MQTT user '{sensor_name}' updated in database.")
        else:
            print(f"MQTT user '{sensor_name}' used existing credentials.")
        db_connection.close()
    except Exception as e:
        print(f"Database error: {e}")
        exit(1)

    send_to_esp = input("Send credentials to ESP EEPROM now? (y/n): ").strip().lower()
    if send_to_esp == 'y':
        try:
            import base64
            import serial
            ser = serial.Serial(serial_port, serial_speed, timeout=5)
            b64_user = base64.b64encode(sensor_name.encode('utf-8')).decode('ascii')
            b64_pass = base64.b64encode(mqtt_password.encode('utf-8')).decode('ascii')
            msg = f"SETMQTTCRED:{b64_user}:{b64_pass}\n"
            ser.write(msg.encode('utf-8'))
            ser.flush()
            try:
                response = ser.readline().decode('utf-8').strip()
                print(f"ESP response: {response}")
            except Exception:
                print("No response from ESP (timeout or not implemented)")
            ser.close()
        except Exception as e:
            print(f"Error sending credentials to ESP: {e}")

if __name__ == "__main__":
    main()
