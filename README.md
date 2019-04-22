# Generic-ESP8266-IoT-Framework

Generic ESP8266 IoT Framework

This repository holds an example of a generic IoT device which can be used in conjunction with the Instructable series on home automation [here](https://www.instructables.com/id/Home-Automation-12/).
It has been successfully tested with the following devices; ESP8266-01, ESP8266-07 and ESP8266-12


## Directory contents

### GenericIoTFrameWorkESP8266_1

Contains the GenericIoTFrameWorkESP8266_1.ino Arduino sketch along with the sub-directory 'data'

#### data

These files are to be copied to the device as SPIFFs and are used by the system at start up to configure the device and serve up a web page if the device cannot connect to your home network for whatever reason. 

There are two (main) text files named 'secvals.txt' and 'index.htm'

index.htm : This is the source of the 'Generic IoT Device Configuration Home Page'

secvals.txt : contains six entries used by the IoT device to connect to your home network. You will need to add your own SSID and P/W here.


