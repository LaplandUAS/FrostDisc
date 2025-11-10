![He4T Logo](/img/he4tlogo0.png "Project logo")
# FrostDisc
HE4T ForstDisc
Road embedded ultra-low power temperature gradient sensor built with wireless LoRaWAN technology. This repository contains the system firmware, and printed circuit board designs.
The device features:
* 4x [TMP117](https://www.ti.com/lit/ds/symlink/tmp117.pdf?ts=1762762775582) High-Accuracy temperature sensors.
* MultiTech [xDot](https://multitech.com/wp-content/uploads/86002182.pdf) LoRaWAN module.
* [MSP430G2231](https://www.ti.com/lit/ds/symlink/msp430g2231-q1.pdf?ts=1762703573675) microcontroller.

The device's energy source also features a battery pack and a super capacitor power reserve maintained by an [Epishine](https://www.epishine.com/) PV-cell. The assembly is embedded into a durable resin cast.

![Mockup 3d render](/img/frostdisc0.gif "Frostdisc mockup")

## Hardware setup
Required tools and hardware:
* Soldering iron
* USB-UART adapter
* MSP-FET programmer
* FrostDisc MK.4

### Radio
First, connect your USB-UART adapter to the edge connector on the FrostDisc MK.4 board. The edge is an [AVX10](https://datasheets.kyocera-avx.com/OpenEndedCardEdge_00-9159.pdf)-edge connector, but temporary soldering or pogo pins work fine too.
Follow this pinout (fig.1). The diagram shown connects to the top-layer of the edge connector.
![UART connection](/img/connector0.png "AVX10 adapter board")
Open a serial terminal software (Like PuTTY or CoolTerm), and connect to your your USB-UART adapter with these parameters:
| Parameter     |     Value     |
| ------------- |:-------------:|
| Baudrate      | 115200        |
| Data Bits     | 8             |
| Parity        | None          |
| Stop Bits     | 1             |
Confirm proper connection with:
```
>ATI
ATIMultiTech xDot
Firmware : 4.1.32-mbed60800
Library  : 4.1.30-mbed60800
MTS-Lora : 4.1.21-mbed60800

OK
```
> [!WARNING]
> Skip commands marked with '*' before writing parameters if you intend to test your connection manually first!
> Only apply these commands after you've checked your connection. The command order given below is is for bulk-programming without any manual testing.

Apply these parameters in this order:

| Parameter                 |  Description                                 |
| ------------------------- |:--------------------------------------------:|
| AT+DI?                    | DevEUI, write down for your LoRaWAN provider |
| AT+NA=<Your DevAddr>      | Match with your LoRaWAN provider (HEX-tring) |
| AT+DSK=<Your AppSKey>     | Match with your LoRaWAN provider (HEX-tring) |
| AT+NSK=<Your NSK>         | Match with your LoRaWAN provider (HEX-tring) |
| AT+NJM=0                  | Sets radio in ABP mode                       |
| AT+WP=6,0,1               | Sets wakeup pin *                            |
| AT+WM=1                   | Sets wakeup mode to "On interrupt" *         |
| ATE0                      | Disable echo (optional) *                    |
| AT&W                      | Write parameters                             |
| ATZ                       | Soft reset                                   |

Please refer to your LoRaWAN network service provider's instructions for pairing, or your gateway's user manual if you have your own.

Further information on the xDot AT-syntax can be found [here](https://multitech.com/wp-content/uploads/S000768-xDot-AT-Command-Guide.pdf).

You can manually test your connection with your LoRaWAN network of choice after configuring both ends by issuing the following AT command:

```
>AT+SENDB=DEADBEEF

OK
```
Note that in order to comply with EU868 restrictions, the transmission channel will be disabled for a few minutes after each `AT+SEND`.

### MCU

The MCU firmware is uploaded using an MSP-FET programmer for TexasInstruments MSP430-based microcontrollers. You can also use the debug-part of an MSP430 Launchpad development board.
The firmware is flashed using the CodeComposer Studio IDE.

Hook up your programmer of choice to the FrostDisc MK.4 header J.3 (fig.2)
![Programmer hookup](/img/mcudiag0.png "MCU hookup diagram")

In CodeComposer Studio, right click the active project and choose "Properties" (fig.3).

![Properties dialog](/img/ccstudioproperties0.png "Project properties")
In the General settings, make sure that the device variant is "MSP430G2231". Then plug your programmer to your PC's USB-port and click "Identify". After the debugger identifies your programmer, it should show up in the drop down menu as `TI MSP430 Launchpad`, or `TI MSP430 USB1`.
For the linker command file, click browse and use the .cmd file provided in the 'src' directory. Click apply and Close.

Next, click the "Flash" icon on the top left corner (fig.4)

![Flash icon](/img/ccstudioflashbtn0.png "Click flash")

Disconnect the programmer from the FrostDisck MK.4 once the firmware flash completes.

Next, solder a bridge over JP1, bridging it's topmpost and middle pad. Bridge the SJ2 pads (fig.5)

![Solder jumpers](/img/frostdiscjumpers0.png "Solder jumper bridges")

### Initial tests
Before connecting the battery pack and temperature sensors, test if the system transmits data properly. Hook up a 3.3V power supply to the VCC and GND pins on the programming header and monitor your LoRaWAN gateway traffic. A small antenna may be needed for your radio if the RSSI is poor. After 10-minutes, you should be receiving the first packets from your device, which should be in a hexadecimal string format. The payload consists of 4 16-bit temperature readings followed by a single 16-bit value indicating the current battery charge.

Example payload:
`data: 0AFF0AFF0AFF0AFF3024D`
| Sensor 1      | Sensor 2      | Sensor3       | Sensor 4      | Core VCC      |
| ------------- |:-------------:|:-------------:|:-------------:|:-------------:|
| 0AFF          | 0AFF          | 0AFF          | 0AFF          | 024D          |

All the sensor readings will display the same value since they're not connected. For decoding payloads, we have ([a terminal program]repository here) for interfacing with an MQTT broker.

### Sensors and Battery pack

After confirming succesful downlink, disconnect the device from your power supply and solder thin 24-26AWG wires to the TMP117 sensor module (fig.6).

![TMP117 module](/img/frostdiscsensors0.png "TMP117 subassembly")

Take note of the labels above the pads.
Solder to the corresponding pads on the FrostDisc MK.4 board (fig.7).

![I2C Bus](/img/frostdiscsensors1.png "TMP117 bus").

Mount the battery pack on the J1 connector on the top edge of board, and attach the assembly to the 3D-printed cage.
TODO: Add cage CAD model
