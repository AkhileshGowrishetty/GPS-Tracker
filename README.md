# GPS Tracker Project

A GPS Tracker Prototype used for locating tracking vehicles.

**Hardware:**
1. ESP32
1. SIM7600EI

The SIM7600 module used in this project can be at rhydoLABZ. The PCB and 3D designed enclosure are specific to this module.

This tracker gets the current location coordinates using the SIM7600 module, and then sends them to AWS. A web site is hosted on AWS to display the coordinates from the tracker.

AWS IoT Core, DynamoDB and EC2 services from AWS are used for this project.

---

### Instructions:
- The file `secrets_template.h` has the variables that has to be edited. After editing, rename the file to `secrets.h` and place it in the same folder that has the `main.cpp` file.
    ```
    The folder structure has to be some thing like this:
    -- Project
        | -- 
        | -- 
        | -- src
            | -- secrets.h
            | -- main.cpp
            | -- SIM7600.cpp
            | -- SIM7600.h
        | -- 
    ```

- Follow the steps in [Documentation for AWS](./Documentation/AWS_IoT_Core_Documentation.md) and [Documentation for SIM7600](./Documentation/Setting_Up_SIM7600.md).
    > For deeper understanding of the AT Commands, the document "SIM7500_SIM7600 Series_ AT Command Manual" can be referred.

- This project was developed in [PlatformIO](https://platformio.org/). There are many tutorials which help in installing and uploading the program to the ESP32.

---
### Understanding the code:
- The code creates few tasks to control various peripherals. 
    1. LED control
    1. Battery monitoring system
    1. Active time management
    1. Fetch GPS co-ordinates and Publish to AWS
    1. Monitoring the Serial input. (Work in progress)

- A semaphore is used to control the number of the times the status LED blinks. Few FreeRTOS functions to handle tasks are used to suspend and resume the LED control task.

- To prevent the battery from discharging through the voltage divider used for voltage level detection, a MOSFET is used to enable the voltage divider. This task also switched OFF the SIM7600 module if the voltage is low.

---
### Troubleshooting:
- To disable MQTT functions, the line `#define MQTT_CONNECT` can be commented. By commenting it, the AT commands corresponding to MQTT connection, publishing are not sent to SIM7600 module.