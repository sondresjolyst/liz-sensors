import getpass
import psycopg2
from utils import create_or_update_mqtt_user, set_mqtt_acl, hash_password, generate_salt, ACLEntry

garge_name = "garge"
garge_environment = "dev"

mqtt_database = f"{garge_name}-mqtt-{garge_environment}"
database_host = "tumogroup.com"
database_user_table = "mqtt_user"
database_acl_table = "mqtt_acl"
database_port = 5432

def main():
    print("MQTT Admin User Creation")
    pg_user = input("Enter PostgreSQL admin username: ")
    pg_password = getpass.getpass("Enter PostgreSQL admin password: ")

    username = input("Enter admin username: ").strip()
    password = getpass.getpass("Enter password: ").strip()

    salt = generate_salt()
    password_hash = hash_password(password, salt)

    admin_acl: list[ACLEntry] = [
        {"action": "all", "permission": "allow", "topic": "#", "qos": [0, 1, 2], "retain": 1}
    ]

    try:
        conn = psycopg2.connect(
            dbname=mqtt_database,
            user=pg_user,
            password=pg_password,
            host=database_host,
            port=database_port
        )
        create_or_update_mqtt_user(
            conn,
            database_user_table,
            username,
            password_hash,
            salt,
            is_superuser=True
        )
        set_mqtt_acl(conn, database_acl_table, username, admin_acl)
        conn.commit()
        print(f"Admin user '{username}' created/updated with full ACL.")
        conn.close()
    except Exception as e:
        print(f"Database error: {e}")

if __name__ == "__main__":
    main()
