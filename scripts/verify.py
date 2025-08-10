import hashlib

def verify_password(password, hash_hex, salt, iterations=300_000, hash_byte_size=24):
    dk = hashlib.pbkdf2_hmac('sha512', password.encode('utf-8'), salt.encode('utf-8'), iterations, dklen=hash_byte_size)
    return dk.hex() == hash_hex
