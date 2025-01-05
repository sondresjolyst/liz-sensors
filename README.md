# ReadTempAndHumidity

## Liz Sensor Voltmeter

![Liz Sensor Voltmeter](</assets/svg/Liz Sensor Voltmeter.svg>)

### Voltage Divider Calculation for Wemos D1 Mini

To measure a higher voltage than 3.3V using the Wemos D1 Mini's ADC port, you can create a voltage divider with 47kΩ and 4.7kΩ resistors.

#### Voltage Divider Formula

$`V_{out} = V_{in} \times \frac{R2}{(R1 + R2)}`$

##### Precision

The Wemos D1 Mini's ADC can read voltages between 0 and 3.3V and gives a 10-bit reading (0-1024).
To calculate the minimum precision in voltage, one need to divide the maximum voltage by the ADC resolution:
$`\text{Minimum precision} = \frac{3.3V}{1024} \approx 0.00322V`$

The voltage at the ADC pin is approximately 9.09% of the input voltage. When the ADC reading changes by one step (e.g., from 366 to 365), the input voltage changes by:

$`\Delta V_{in} \approx \Delta V_{ADC} \times \frac{R1 + R2}{R2}`$

$`\Delta V_{in} \approx 0.00322V \times \frac{47k\Omega + 4.7k\Omega}{4.7k\Omega} \approx 0.0355V`$

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

### Power Consumption Calculation

To estimate the power consumption of the Wemos D1 Mini:

#### Current Draw

Assuming an average current draw of 80mA (0.08A) when performing computations and connected to Wi-Fi.

#### Power Calculation

Using a 5V supply voltage:
$`\text{Power (P)} = \text{Voltage (V)} \times \text{Current (I)}`$

$`P = 5V \times 0.08A = 0.4W`$

#### Battery Life Estimation

With a 13V battery converted to 5V, taking into account the efficiency of the DC-DC converter (assumed to be 90% efficient):

$`\text{Effective Power Required} = \frac{\text{Power Consumption}}{\text{Efficiency}}`$

$`\text{Effective Power Required} = \frac{0.4W}{0.9} \approx 0.444W`$

#### Battery Current

$`I_{13V} = \frac{0.444W}{13V} \approx 0.034A = 34mA`$

#### Battery Capacity

If using a 13V motorcycle battery with a 6Ah (6000mAh) capacity:

Estimated Runtime:

$`\text{Runtime} = \frac{\text{Battery Capacity}}{\text{Current Draw}}`$

$`\text{Runtime} = \frac{6000mAh}{34mA} \approx 176 \, \text{hours}`$

To keep the battery above 40% charge, use only 60% of the battery's capacity:

$`\text{Usable Capacity} = 0.6 \times 72Wh = 43.2Wh`$

Revised Runtime:

$`\text{Runtime} = \frac{43.2Wh}{0.4W} = 108 \, \text{hours}`$

## Liz Sensor BME280

![Liz Sensor Voltmeter](</assets/svg/Liz Sensor BME280.svg>)
