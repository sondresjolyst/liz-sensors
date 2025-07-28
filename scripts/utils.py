from typing import List, Literal, TypedDict, Union
from psycopg2.extensions import connection
import hashlib
import os

class ACLEntry(TypedDict):
    action: Literal["publish", "subscribe", "all"]
    permission: Literal["allow", "deny"]
    topic: str
    qos: Union[int, List[int]]
    retain: int

def create_or_update_mqtt_user(
    db_connection: connection,
    user_table: str,
    username: str,
    password_hash: str,
    salt: str,
    is_superuser: bool = False
) -> None:
    cur = db_connection.cursor()
    cur.execute(
        f"""
        INSERT INTO {user_table} (username, password_hash, salt, is_superuser)
        VALUES (%s, %s, %s, %s)
        ON CONFLICT (username) DO UPDATE
        SET password_hash = EXCLUDED.password_hash, salt = EXCLUDED.salt, is_superuser = EXCLUDED.is_superuser;
        """,
        (username, password_hash, salt, is_superuser)
    )
    cur.close()

def set_mqtt_acl(
    db_connection: connection,
    acl_table: str,
    username: str,
    acl_entries: List[ACLEntry]
) -> None:
    if not acl_entries:
        raise ValueError("acl_entries must be provided and not empty.")
    cur = db_connection.cursor()
    cur.execute(f"DELETE FROM {acl_table} WHERE username = %s;", (username,))
    values = []
    params = []
    for entry in acl_entries:
        qos_values = entry["qos"]
        if isinstance(qos_values, list):
            for qos in qos_values:
                values.append("(%s, %s, %s, %s, %s, %s)")
                params.extend([
                    username,
                    entry["action"],
                    entry["permission"],
                    entry["topic"],
                    qos,
                    entry["retain"]
                ])
        else:
            values.append("(%s, %s, %s, %s, %s, %s)")
            params.extend([
                username,
                entry["action"],
                entry["permission"],
                entry["topic"],
                qos_values,
                entry["retain"]
            ])
    if values:
        cur.execute(
            f"""
            INSERT INTO {acl_table} (username, action, permission, topic, qos, retain)
            VALUES {', '.join(values)}
            ON CONFLICT DO NOTHING;
            """,
            tuple(params)
        )
    cur.close()

def generate_salt(length: int = 16) -> str:
    return os.urandom(length).hex()

def hash_password(password: str, salt: str) -> str:
    return hashlib.sha256((password + salt).encode()).hexdigest()