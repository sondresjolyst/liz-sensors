# Wemos_D1_Mini_30359b

vinTest: 5.31395, multimeter Voltage: 5.03
vinTest: 13.16004, multimeter Voltage: 12.62

b = (ln(12.62) - ln(5.03)) / (ln(13.16004) - ln(5.31395))
b = 1.01435

a = exp(ln(5.03) - 1.01435 \* ln(5.31395))
a = 0.924146