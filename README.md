contact: tech@solarstik.com

## Used by

This is a ESP-IDF component, and it is used by project LESS (99-0000003). The old version is used in Magnethereal (99-0002000), but it won't be updated to this.

# How it works

It's important to note this is designed for ESP board only and currently it's only used and tested on RMK board designed internally. There are two major source
files and they share the same header file.

SDCard.cpp handles the SD card mounting. There are two protocols that can be used here. Are we using a SPI device? Yes -> Built-in SD card slot can't be used, so SDMMC
doesn't work for us. No -> Use SDMMC and the built-in SD card slot can be used. The SPI bus initialization is commented out because some other components will start the bus. If your project only uses SD card with SPI bus, you should initiate the SPI bus here. If there are multiple SPI devices, they can share all data lines, but they need to have individual cs lines. Any GPIO can act as a CS port.

dataLogging.cpp is used to log data into SD card. Modify the dataLogNow function accordingly and it will write the data into the sd card.
Call begin_SD() to start a task that will handle both mounting SD card and start logging data.
