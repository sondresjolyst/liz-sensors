# Calibration data

## Wemos_D1_Mini_30359b

vinTest: 5.34961, multimeter Voltage: 5.02
vinTest: 13.05305, multimeter Voltage: 12.46

b = (ln(12.46) - ln(5.02)) / (ln(13.05305) - ln(5.34961))
b = 1.01917

a = exp(ln(5.02) - b \* ln(5.34961))
a = 0.908698

## Wemos_D1_Mini_6ceb7b

vinTest: 5.31395, multimeter Voltage: 5.02
vinTest: 12.87473, multimeter Voltage: 12.46 !

b = (ln(12.46) - ln(5.02)) / (ln(12.87473) - ln(5.31395))
b = 1.0273

a = exp(ln(5.02) - b \* ln(5.31395))
a = 0.902573
