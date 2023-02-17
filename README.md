## Gas-Meter
Gas Meter with reed switch and ESP8266 using Light Sleep and Blynk
(which I can wake it up with a GPIOs).

This project helps you to monitor the consumption of your home gas meter remotely over the internet by using pulse counting methods.<br>

Many standards gas meters for electricity (in the GR or other countries) have a unique “spot” on their rotating disk, which can be read with a suitable magnetic sensor and appropriate  electronics.<br>
We can count these magnetic pulses by using a Hall effect sensor or a reed switch to determine gas consumption. Each pulse corresponds to 0.1 m³.<br>

### Concept: 
The plan is to use an ESP8266 RTC built-in timer and try to implement a light sleep mode on it.<br>
Each time the reed switch closes, the ESP wakes up from light sleep and therefore we can count the reboots.<br>

#### Why Light Sleep Mode? 
This mode of operation is useful to get minimal power consumption between each pluses.
In this mode, digital peripheral devices, most of the RAM, and CPUs are clock-controlled.

To do this, we set the maximum sleep time on ESP-12 and wake it up by triggering the GPIO02 pin(or any other GPIO pins).

#### Save Data on RTC Memories 
The ESP82xx has 512 bytes SRAM on the RTC part, called RTC memory. The data saved will not be deleted during light sleep.<br>
To save how often the ESP8266 wakes up from a light sleep, we use the RTC internal memory of the ESP82xx.<br>
The RTC memory is addressable per 32 bits.<br>
In total, there is 768 bytes (192 addressable blocks).<br>

0 .. 63 (256 bytes) Reserved for esp8266/Arduino library.<br>
In this structure I define two variables one for storing the count of reboots (_count) and another one for daily task (_lastDay).<br>

_count (This variable is used to store the count of  times that the ESP woke up from light sleep).<br>

![ScreenShot](https://www.forgani.com/wp-content/2021/12/arduino_gat_meter_circuit.jpg)

The Magnetic Reed switch is connected between GPIO02 and ground.<br>
GPIO02 use an external pullup resistor. In this case, when switch is open and magnet is away, the pin is high.<br>
When the switch is closed then the pin is low, it’s grounded.<br>

#### The Sleep for longest possible time for the ESP is:

#define FPM_SLEEP_MAX_TIME 0xFFFFFFF
wifi_fpm_do_sleep(FPM_SLEEP_MAX_TIME);
Finally, when the reed switch is closed due to the proximity of the magnet, it  wakes up, connect to wifi and when the reed switch is open then it goes back to sleep again. It waits for the next event.

### Assembly:

![ScreenShot](https://www.forgani.com/wp-content/2021/12/arduio_gas_meter-1.jpg)

It uses the Blynk for notifications. 
Blynk App is the easiest way to set up phone notification with wifi board.

![ScreenShot](https://www.forgani.com/wp-content/2021/12/blynk_gas_meter.jpg)


For more infos: [gas-meter-counter-with-reed-switch-for-measurement](https://www.forgani.com/electronics-projects/gas-meter-counter-with-reed-switch-for-measurement/)

