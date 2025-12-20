import math

current_a = 0.91406
current_b = 1.02092

# new measurements with current calibration
measured_voltages = [5.31395, 12.12718391]  # Measured voltages from the sensor
actual_voltages = [5.03, 12.67]          # Actual battery voltage

# Reverse the current calibration to get raw sensor readings
raw_voltages = []
for measured in measured_voltages:
    # measured = current_a * raw^current_b
    # raw = (measured / current_a)^(1/current_b)
    raw = (measured / current_a) ** (1 / current_b)
    raw_voltages.append(raw)

print("Raw sensor readings (before calibration):")
for i, raw in enumerate(raw_voltages):
    print(f"  Point {i+1}: {raw:.5f}V")
print()

# calculate new a and b using raw sensor readings
x1, y1 = raw_voltages[0], actual_voltages[0]
x2, y2 = raw_voltages[1], actual_voltages[1]

b = (math.log(y2) - math.log(y1)) / (math.log(x2) - math.log(x1))
a = math.exp(math.log(y1) - b * math.log(x1))

print(f"New calibration constants:")
print(f"const float a = {a:.5f};")
print(f"const float b = {b:.5f};")
print()

# Verify with raw readings
print("Verification:")
for raw, actual in zip(raw_voltages, actual_voltages):
    corrected = a * (raw ** b)
    error = abs(corrected - actual)
    print(f"Raw: {raw:.5f}V → Corrected: {corrected:.5f}V (Actual: {actual}V, Error: {error:.5f}V)")