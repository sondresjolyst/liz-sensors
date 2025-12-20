from psycopg2.extensions import connection
from typing import List, Literal, TypedDict, Union
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
    salt: str,
    password_hash: str,
    is_superuser: bool = False,
) -> None:
    cur = db_connection.cursor()
    cur.execute(
        f"""
        INSERT INTO "{user_table}" ("Username", "Salt", "PasswordHash", "IsSuperuser")
        VALUES (%s, %s, %s, %s)
        ON CONFLICT ("Username") DO UPDATE
        SET "Salt" = EXCLUDED."Salt",
            "PasswordHash" = EXCLUDED."PasswordHash",
            "IsSuperuser" = EXCLUDED."IsSuperuser";
        """,
        (username, salt, password_hash, is_superuser),
    )
    cur.close()


def set_mqtt_acl(
    db_connection: connection,
    acl_table: str,
    username: str,
    acl_entries: List[ACLEntry],
) -> None:
    if not acl_entries:
        raise ValueError("acl_entries must be provided and not empty.")
    cur = db_connection.cursor()
    values = []
    params = []
    for entry in acl_entries:
        qos_values = entry["qos"]
        if isinstance(qos_values, list):
            for qos in qos_values:
                values.append("(%s, %s, %s, %s, %s, %s)")
                params.extend(
                    [
                        username,
                        entry["action"],
                        entry["permission"],
                        entry["topic"],
                        qos,
                        entry["retain"],
                    ]
                )
        else:
            values.append("(%s, %s, %s, %s, %s, %s)")
            params.extend(
                [
                    username,
                    entry["action"],
                    entry["permission"],
                    entry["topic"],
                    qos_values,
                    entry["retain"],
                ]
            )
    if values:
        cur.execute(
            f"""
            INSERT INTO "{acl_table}" ("Username", "Action", "Permission", "Topic", "Qos", "Retain")
            VALUES {", ".join(values)}
            ON CONFLICT DO NOTHING;
            """,
            tuple(params),
        )
    cur.close()


def generate_salt(length=16):
    return os.urandom(length).hex()


def hash_password_pbkdf2(
    password: str, salt: str, iterations: int = 300000, hash_byte_size: int = 32
) -> str:
    dk = hashlib.pbkdf2_hmac(
        "sha512",
        password.encode("utf-8"),
        salt.encode("utf-8"),
        iterations,
        dklen=hash_byte_size,
    )
    return dk.hex()
