# ReadTempAndHumidity

## Liz Sensor Voltmeter

![Liz Sensor Voltmeter](</assets/svg/Liz Sensor Voltmeter.svg>)

### Voltage Divider Calculation for Wemos D1 Mini

To measure a higher voltage than 3.3V using the Wemos D1 Mini's ADC port, you can create a voltage divider with 47kΩ and 4.7kΩ resistors.

#### Voltage Divider Formula

$`V_{out} = V_{in} \times \frac{R2}{(R1 + R2)}`$

#### Given Values

- $`R1 = 47kΩ`$ (47000Ω)
- $`R2 = 4.7kΩ`$ (4700Ω)
- $`V_{out} = 3.3V`$ (ADC input maximum)

#### Calculate Maximum

$`V_{in} = \frac{V_{out} \times (R1 + R2)}{R2}`$

Plugging in the values:

$`V_{in} = \frac{3.3V \times (47000Ω + 4700Ω)}{4700Ω}`$

$`V_{in} = 3.3V \times \frac{51700Ω}{4700Ω}`$

$`V_{in} \approx 36.3V`$

## Liz Sensor BME280

![Liz Sensor Voltmeter](</assets/svg/Liz Sensor BME280.svg>)
