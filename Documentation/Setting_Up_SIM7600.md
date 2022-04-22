# Connect SIM7600 to AWS IoT - Wiki

Documentation for Connecting SIM7600 to AWS IoT core.  
The contents of this wiki are to connect to AWS IoT core manually using a Serial Terminal on a PC. The AT commands are sent to the SIM7600 module through the terminal. This method can be used to check if the policies and the device certificates are correct.  
This can also be implemented using a Microcontroller.

> For deeper understanding of the AT Commands, the document "SIM7500_SIM7600 Series_ AT Command Manual" can be referred.

> References:  
> 1. [Hackster.io - Connecting SIM7600X-H to AWS Using MQTT and AT Commands](https://www.hackster.io/victorffs/connecting-sim7600x-h-to-aws-using-mqtt-and-at-commands-2a693c)
---

**Table of Contents**

1. [Configuring Serial Terminal](#1-configuring-serial-terminal)
1. [Download Certificates](#2-download-certificates)
1. [Configuring SSL in SIM7600](#3-configuring-ssl-in-sim7600)
1. [Starting the MQTT session and Connecting to Broker (AWS IoT Core)](#4-starting-the-mqtt-session-and-connecting-to-broker-aws-iot-core)
1. [Subscribe and Publish of messages to Topics](#5-subscribe-and-publish-of-messages-to-topics)
1. [Unsubscribe of Topics](#6-unsubscribe-of-topics)
1. [Disconnecting the Broker and Stop MQTT session](#7-disconnecting-the-broker-and-stop-mqtt-session)

---

## 1. Configuring Serial Terminal:
- A Serial Terminal with the feature to drag-and-drop a file to send has to be used. This feature is used to send the Certificates to the SIM7600 module. **CoolTerm** is used here. It can be downloaded from [here](http://freeware.the-meiers.org/).
- Set the **PORT** to the **COM PORT** of the USB to Serial converter. **Baudrate: 115200, Data Bits: 8, Parity: None, Stop Bits: 1, Flow Control: None**. Set the **New-line** to **CR+LF**.
- Connect and Test by sending `AT` and press **Enter**. `OK` message must be received from the SIM7600 module.

## 2. Download Certificates:
- The Certificates must be downloaded when registering a Thing in AWS Console. The instructions can be found [here](./AWS_IoT_Core_Documentation.md#1-setting-up-iot-core).
- The following certificates must be sent to the SIM7600 module.
    1. xxxx-certificate.pem.crt (Thing Certificate)
    1. xxxx-private.pem.key     (Private Key)
    1. AmazonRootCA1.pem        (Amazon Root Certificate)
- Note the size (in Bytes) of the above certificates. It can be found by checking the **Properties** of the file.
- Before sending the certificates, any previous certificates have to be deleted. To check the stored certificates, send the following command:  
    ```
    AT+CCERTLIST
    ```
- If only `OK` is received, then we can proceed to download the certificates. Else follow the steps to delete the certificates.
    - For example, if the received response is  
        ```
        +CCERTLIST: "ca_cert.pem"
        +CCERTLIST: "client_key.pem"

        OK
        ```
        This means that there are 2 certificates named `ca_cert.pem` and `client_key.pem` are stored in the SIM7600 module.
    - To delete a certificate, send the following to the SIM7600 module.
        ```
        AT+CCERTDELE=<filename>
        ```
        - For example, to delete the certificate `ca_cert.pem` in above response, send 
            ```
            AT+CCERTDELE="ca_cert.pem"
            ```
- Send the following commands to download the certificates to SIM7600:  
    ```
    AT+CCERTDOWN=<certificate_name>,<size(in bytes)>
    ```  
- After sending the above command drag-and-drop the file onto the Terminal.  

- For example, to send the Client Certificate of size 1280 bytes, we need to send 
    ```
    AT+CCERTDOWN="clientcert.pem",1280
    ```  
    The certificate sent will be stored in the SIM7600 module as `clientcert.pem`.  
    Then drag-and-drop the Thing Certificate onto the Terminal.

- Repeat these steps to send all the three certificates mentioned above.

## 3. Configuring SSL in SIM7600
- In this section, SIM7600 module will be configured to use SSL/TLS and also to use the certificates previously set for server and client authentication.
- First, the SSL version has to be set. AT Command to set the SSL version to use SSL 3.0, TLS 1.0, TLS 1.1, TLS 1.2:
    ```
    AT+CSSLCFG="sslversion",0,4
    ```
- Second, the authentication mode is set for server and client authentication mode. AT Command for the same is:
    ```
    AT+CSSLCFG="authmode",0,2
    ```
- Next, the client certificates are configured for the SSL context. AT Commands are:
    ```
    AT+CSSLCFG="cacert",0,<Amazon Root Certificate Filename>
    AT+CSSLCFG="clientcert",0,<Thing Certificate Filename>
    AT+CSSLCFG="clientkey",0,<Private Key Filename>
    ```
    - For example, if the Amazon Root Certificate is stored as file `cacert.pem`, Thing Certificate is stored as `clientcert.pem` and the Private Key is stored as `clientkey.pem`, then the AT Commands will be:
        ```
        AT+CSSLCFG="cacert",0,"cacert.pem"
        AT+CSSLCFG="clientcert",0,"clientcert.pem"
        AT+CSSLCFG="clientkey",0,"clientkey.pem"
        ```

## 4. Starting the MQTT session and Connecting to Broker (AWS IoT Core)
- First, the MQTT session is started. 
    ```
    AT+CMQTTSTART
    ```
- Second, the Client Name and the type (TCP or SSL/TLS) of server is set. Here the **ThingName** must be replaced with what was set when creating the Thing.
    ```
    AT+CMQTTACCQ=0,<ThingName>,1
    ```
- Next, the previously configured SSL context will be used set for MQTT context. AT Command:
    ```
    AT+CMQTTSSLCFG=0,0
    ```
- Finally, we connect to the MQTT broker. The endpoint of the MQTT broker is needed in this step. It can be found in the Settings page in the AWS IoT Console. AT Command is the following:
    ```
    AT+CMQTTCONNECT=0,"tcp://endpoint:8883",60,1
    ```
    -  If the **ThingName** set in previous step is not matched with what was set in the **Policy** when creating the **Thing**, then the SIM7600 module will not be able to connect to the server. The following error will be received from the module: 
        ```
        +CMQTTCONNECT: 0,6

        ERROR
        ```

## 5. Subscribe and Publish of messages to Topics
- To set the **Topic** for publishing, the following AT Command is used:
    ```
    AT+CMQTTTOPIC=0,<Publish Topic Length>
    ```
    - For example, if the publish topic is **PubTopic** `(8 characters)`, then the command is:
        ```
        AT+CMQTTTOPIC=0,8
        ```
- After entering the above command with the correct publish topic length, the SIM7600 module responds with **`>`**.
- On receiving the above character, the publish topic has to be sent with exact number of characters. The module accepts exactly the previously set number.
    - Example:
        ```
        > PubTopic
        ```
- When everything works, the module of responds with `OK`. 

- To set a message for the above topic, the following AT Command is used:
    ```
    AT+CMQTTPAYLOAD=0,<Message Length>
    ```
    - For example, if the message is **{"message":"Hello"}** `(19 characters)`, then the command is:
        ```
        AT+CMQTTPAYLOAD=0,19
        ```
After entering the above command with the correct message length, the SIM7600 module responds with **`>`**.
- On receiving the above character, the message has to be sent with exact number of characters. The module accepts exactly the previously set number.
    - Example:
        ```
        > {"message":"Hello"}
        ```
- When everything works, the module of responds with `OK`.
- To publish the message to the set topic, the following AT Command is used:
    ```
    AT+CMQTTPUB=0,<QOS>,<Publish Timeout>
    ```
- When everything works, the module of responds with
    ```
    OK
    
    +CMQTTPUB: 0,0
    ```
    -  If the **Publish Topic** set in previous step is not matchedwith what was set in the **Policy** when creating the **Thing**, then the SIM7600 module will disconnect from the server. The following error will be received from the module: 
        ```
        +CMQTTPUB: 0,6

        ERROR

        +CMQTTCONNLOST: 0,1
        ```
- To publish to the same topic again, the topic has to be set again.

- To set the **Topic** for subscribing, the following AT Command is used:
    ```
    AT+CMQTTSUBTOPIC=0,<Subscribe Topic Length>,<QOS Level>
    ```
- To subscribe to the topic, the following AT Command is used:
    ```
    AT+CMQTTSUB=0
    ```
- When the SIM7600 module receives a message to the subscribed topic, the following response is received:
    ```
    +CMQTTRXSTART: 0,<Topic Length>,<Payload Length>
    +CMQTTRXTOPIC: 0,<Topic Length>
    <Topic>
    +CMQTTRCPAYLOAD: 0,<Payload Length>
    <Payload>
    +CMQTTRXEND: 0
    ```

## 6. Unsubscribe of Topics
- To set a topic for unsubscribing, the following AT Command is used:
    ```
    AT+CMQTTUNSUBTOPIC=0,<Unsubscribe Topic Length>
    ```

- To unsubscribe the topic, the following AT Command is used:
    ```
    AT+CMQTTUNSUB=0,0
    ```

## 7. Disconnecting the Broker and Stop MQTT session
- First, we need to disconnect from the broker. The following AT Command is used:
    ```
    AT+CMQTTDISC=0,<Timeout>
    ```
- Next, we need to release the client. The following AT Command is used:
    ```
    AT+CMQTTREL=0
    ```
- Finally, we can stop the MQTT session. The following AT Command is used:
    ```
    AT+CMQTTSTOP
    ```