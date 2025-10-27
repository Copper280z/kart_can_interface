# CAN Sensor Interface #

This is firmware for a device that manages various sensors and broadcasts their
values over CAN.

## Hardware ##

This firmware is targeting the STM32H743 based MicoAir743 flight controller,
which is also sold under other brands and is available for ~$35-$50.

<https://micoair.com/flightcontroller_micoair743/>
<https://aeroselfie.myshopify.com/products/flight-controller-h743-for-drones>

This was chosen because it's relatively cheap, off the shelf, has connectors,
and has a CAN transciever. In the future other targets might be added.

## Building ##

The expected toolchain is `arm-none-eabi-gcc`, install via your favorite package
manager.

Other gcc versions will probably work too. Clang might, maybe.

```bash
CC=arm-none-eabi-gcc make
```

CC isn't specified in the makefile so that it's possible to use a tool
like Bear (<https://github.com/rizsotto/Bear>) to generate `compile_commands.json`.
Bear doesn't work well when cross compiling because by default it intercepts
CC, but we're not using the system default CC in most cases.

If you've got Bear installed:

```bash
CC=arm-none-eabi-gcc bear -- make
```
