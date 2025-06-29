Import("env")

producer = env.GetProjectOption("custom_producer_name")
garge_type = env.GetProjectOption("custom_garge_type")
sensor_type = env.GetProjectOption("custom_sensor_type")
prog_version = env.GetProjectOption("custom_version")

firmware_name = f"firmware_{producer}_{garge_type}_{sensor_type}_{prog_version}"

env.Replace(PROGNAME=firmware_name)