#include "SIM7600.h"

/**
 * @brief Construct a new SIM7600::SIM7600 object
 * 
 * @param serial    Serial port connected to the SIM7600. 
 */
SIM7600::SIM7600(Stream &serial) : port(serial)
{
    port.setTimeout(defaultTimeout);
    gpio_reset_pin(SIM_POWER_EN);
    gpio_set_direction(SIM_POWER_EN, GPIO_MODE_OUTPUT);
}

/**
 * @brief Checks if the expected response is received from the Modem.
 * 
 * @param s         Pointer to the character array for expected response.
 * @param timeout   Timeout (in seconds) for readString function.
 * @return true     If the expected response is present.
 * @return false    If the expected response is not present.
 */
bool SIM7600::waitForResponse(const char *s, uint8_t timeout)
{
    waitingForResponse = true;
    port.setTimeout(timeout * 1000);
    String resp1 = port.readString();
    char *resp = (char *)resp1.c_str();
    ESP_LOGI("Wait4Resp", "%s\n\n", resp);

    vTaskDelay(500 / portTICK_PERIOD_MS);

    waitingForResponse = false;
    port.setTimeout(defaultTimeout);
    
    if (strstr(resp, s))
        return true;
    else
        return false;
}

/**
 * @brief Switch OFF echo from the SIM7600 module.
 * 
 * @return true     If echo has been switched off.
 * @return false    If echo has not been switched off.
 */
bool SIM7600::echoOFF()
{
    port.printf("ATE0\r");
    return waitForResponse("OK");
}

/**
 * @brief Dummy implementation
 * 
 * @return true 
 * @return false 
 */
bool SIM7600::start()
{
    return true;
}

/**
 * @brief Shutdown the SIM7600 module.
 * 
 * @return true 
 * @return false 
 */
bool SIM7600::shutdown()
{
    port.printf("AT+CPOF\r");
    return waitForResponse("OK");
}

/**
 * @brief Reset the SIM7600 module.
 * 
 * @return true 
 * @return false 
 */
bool SIM7600::reset()
{
    port.printf("AT+CRESET\r");
    return waitForResponse("OK");
}

/**
 * @brief Switch ON the SIM7600 module using the phototransistor and PNP transistor.
 * 
 */
void SIM7600::powerON()
{
    gpio_set_level(SIM_POWER_EN, 1);
}

/**
 * @brief Switch OFF the SIM7600 module using the phototransistor and PNP transistor.
 * 
 */
void SIM7600::powerOFF()
{
    gpio_set_level(SIM_POWER_EN, 0);
}

/**
 * @brief To check if the GPS Modem is ON.
 * 
 * @return true     If the GPS Modem is ON.
 * @return false    If the GPS Modem is OFF.
 */
bool GPS::isOn()
{
    port.printf("AT+CGPS?\r");
    return waitForResponse("+CGPS: 1,1", 3);
}

/**
 * @brief Switch ON the GPS Modem.
 * 
 * @return true     If the GPS Modem is switched ON successfully.
 * @return false    If the GPS Modem was not switched ON.
 */
bool GPS::begin()
{
    if (isOn())
        return true;

    port.printf("AT+CGPS=1\r");

    if (!waitForResponse("OK", 7))
        return false;

    return isOn();
}

/**
 * @brief Stop the GPS Session.
 * 
 */
bool GPS::stop()
{
    if (isOn())
    {
        port.printf("AT+CGPS=0\r");
        return waitForResponse("+CGPS: 0");
    }
    return true;
}

/**
 * @brief Used to cold start the GPS Session.
 * 
 * @return true     If the session is cold started successfully.
 * @return false    If the session is not cold started due to some error.
 */
bool GPS::coldStart()
{
    if (isOn())
        if (!stop())
            return false;

    port.printf("AT+CGPSCOLD\r");
    return waitForResponse("OK");
}

/**
 * @brief Used to hot start the GPS Session.
 * 
 * @return true     If the session is hot started successfully.
 * @return false    If the session is not hot started due to some error.
 */
bool GPS::hotStart()
{
    if (isOn())
        if (!stop())
            return false;

    port.printf("AT+CGPSHOT\r");
    return waitForResponse("OK");
}

/**
 * @brief Convert data from GPS Modem to decimal degrees and store it in data_t.
 * 
 * @param lat   Latitude (ddmm.mmmmm) from GPS Modem.
 * @param NS    North or South character from GPS Modem.
 * @param lon   Longitude (ddmm.mmmmm) from GPS Modem.
 * @param EW    East or West character from GPS Modem.
 */
void GPS::calcLatLong(double lat, char NS, double lon, char EW)
{
    data.latitude = (floor(lat / 100.0) + fmod(lat, 100.0) / 60.0) * (NS == 'N' ? 1 : -1);
    data.longitude = (floor(lon / 100.0) + fmod(lon, 100.0) / 60.0) * (EW == 'E' ? 1 : -1);
}

/**
 * @brief Converts Date and Time to type tm.
 * 
 * @param date  Date (ddmmyy) from GPS Modem.
 * @param time  Time (hhmmss.s) from GPS Modem.
 */
void GPS::formatDateTime(long date, double timeF)
{
    data.timeGPS.tm_year = (date % 100) + 100;
    date /= 100;

    data.timeGPS.tm_mon = (date % 100) - 1;
    date /= 100;

    data.timeGPS.tm_mday = date;

    long time = long(floor(timeF));

    data.timeGPS.tm_sec = (time % 100);
    time /= 100;

    data.timeGPS.tm_min = (time % 100);
    time /= 100;

    data.timeGPS.tm_hour = time;

    data.timestamp = mktime(&data.timeGPS);

#ifdef ARDUINO_AVR_UNO
    data.timestamp += UNIX_OFFSET;
#endif
}

/**
 * @brief Used to get the coordinates from the GPS Modem and store it the structure data.
 * 
 * @param GNSS      Set it to 'true' to use GNSS or set it to 'false' to use GPS.
 * @return true     If the coordinates from the modem are valid.
 * @return false    If the coordinates from the modem are invalid.
 */
bool GPS::getData(bool GNSS)
{
    if (!isOn())
        return false;

    port.printf((GNSS) ? "AT+CGNSSINFO\r" : "AT+CGPSINFO\r");
    
    waitingForResponse = true;

    port.setTimeout(1000);
   
    String resp1 = port.readString();
    int index = resp1.indexOf(": ");
    index += 2;

    waitingForResponse = false;

    resp1.remove(0, index);
    char *ptr, *posData;

    if (resp1.startsWith(","))
        return false;
    else
        ptr = (char *)resp1.c_str();

    double lat, lon, time;
    long date;
    char NS, EW;

    if (GNSS)
    {
        data.fixmode = atoi(ptr);

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        data.GPS_sv = atoi(ptr);

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        data.GLONASS_sv = atoi(ptr);

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        data.BEIDOU_sv = atoi(ptr);

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        lat = atof(ptr);

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        NS = ptr[0];

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        lon = atof(ptr);

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        EW = ptr[0];

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        date = atol(ptr);

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        time = atof(ptr);

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        data.altitude = atof(ptr);

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        data.speed = atof(ptr);
        data.speed *= 1.852;

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        data.course = atof(ptr);

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        data.dop[PDOP] = atof(ptr);

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        data.dop[HDOP] = atof(ptr);

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        data.dop[VDOP] = atof(ptr);
    }
    else
    {
        lat = atof(ptr);

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        NS = ptr[0];

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        lon = atof(ptr);

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        EW = ptr[0];

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        date = atol(ptr);

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        time = atof(ptr);

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        data.altitude = atof(ptr);

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        data.speed = atof(ptr);
        data.speed *= 1.852;

        posData = strchr(ptr, ',');
        ptr = posData + 1;
        data.course = atof(ptr);
    }

    calcLatLong(lat, NS, lon, EW);

    formatDateTime(date, time);

    return true;
}

/**
 * @brief Check if the required certificates are present in the SIM7600 modem.
 * 
 * @return true 
 * @return false 
 */
bool SSL::checkCertificates(const char *cacert, const char *clientcert, const char *clientkey)
{
    port.printf("AT+CCERTLIST\r");
    
    waitingForResponse = true;
    String certificateList_ = port.readString();
    waitingForResponse = false;

    char *certificateList = (char *) certificateList_.c_str();

    certs[CACERT]       = (strstr(certificateList, cacert)) ? true : false;
    certs[CLIENTCERT]   = (strstr(certificateList, clientcert)) ? true : false;
    certs[CLIENTKEY]    = (strstr(certificateList, clientkey)) ? true : false;

    if (certs[CACERT] && certs[CLIENTCERT] && certs[CLIENTKEY])
        return true;
    else
        return false;
}

/**
 * @brief Downloads the required Certificates into the SIM7600 modem from the encrypted nvs partition.
 * 
 * @todo Add code to get the certificates from the nvs partition and send them to the SIM7600 modem.
 * 
 * @return true 
 * @return false 
 */
bool SSL::downloadCertificates()
{
    if(!certs[CACERT])
    {
        
    }
    if(!certs[CLIENTCERT])
    {

    }
    if(!certs[CLIENTKEY])
    {

    }
    return true;
}

/**
 * @brief Set the SSL context for MQTT.
 * 
 * @return true 
 * @return false 
 */
bool SSL::configureSSL(const char *cacert, const char *clientcert, const char *clientkey)
{
    port.printf("AT+CSSLCFG=\"sslversion\",0,4\r");
    bool status = waitForResponse("OK");

    port.printf("AT+CSSLCFG=\"authmode\",0,2\r");
    status &= waitForResponse("OK");

    port.printf("AT+CSSLCFG=\"cacert\",0,\"");
    port.printf("%s", cacert);
    port.printf("\"\r");
    status &= waitForResponse("OK");

    port.printf("AT+CSSLCFG=\"clientcert\",0,\"");
    port.printf("%s", clientcert);
    port.printf("\"\r");
    status &= waitForResponse("OK");

    port.printf("AT+CSSLCFG=\"clientkey\",0,\"");
    port.printf("%s", clientkey);
    port.printf("\"\r");
    status &= waitForResponse("OK");

    return status;
}



/**
 * @brief Starts the MQTT service by activating the PDP context. Must be used before any other MQTT related operations.
 * 
 * @return True if the service is started successfully.
 * @return false 
 */
bool MQTT::begin()
{
    port.printf("AT+CMQTTSTART\r");

    return waitForResponse("OK");
}

/**
 * @brief Stops the MQTT service. 
 * 
 * @return true 
 * @return false 
 */
bool MQTT::end()
{
    port.printf("AT+CMQTTSTOP\r");

    return waitForResponse("OK");
}

/**
 * @brief Used to acquire a MQTT client.
 * 
 * @return true 
 * @return false 
 */
bool MQTT::acquireClient()
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    // port.printf("AT+MQTTACCQ=0,\"ESP%d%d%d%d%d%d\",1\r", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    port.printf("AT+CMQTTACCQ=0,\"SIM7600_Test\",1\r");
    return waitForResponse("OK");
}

/**
 * @brief Used a release a MQTT client.
 * 
 * @return true 
 * @return false 
 */
bool MQTT::releaseClient()
{
    port.printf("AT+CMQTTREL=0\r");
    return waitForResponse("OK", 1);
}

/**
 * @brief Used to set the SSL context to connect to a SSL/TLS MQTT server.
 * 
 * @return true 
 * @return false 
 */
bool MQTT::setSSLContext()
{
    port.printf("AT+CMQTTSSLCFG=0,0\r");
    return waitForResponse("OK");
}

/**
 * @brief Used to connect to a MQTT server.
 * 
 * @param serverAddress Server Address of the MQTT server. (include 'tcp://' in the character array).
 * @param serverPort    Port number of the MQTT server.
 * @return true 
 * @return false 
 */
bool MQTT::connect(const char *serverAddress, unsigned int serverPort)
{
    port.printf("AT+CMQTTCONNECT=0,\"");
    port.printf("%s:%u", serverAddress, serverPort);
    port.printf("\",60,1\r");

    return waitForResponse("+CMQTTCONNECT: 0,0", 5);
}

/**
 * @brief Used to disconnect from the MQTT server.
 * 
 * @return true 
 * @return false 
 */
bool MQTT::disconnect()
{
    port.printf("AT+CMQTTDISC=0,60\r");
    return waitForResponse("OK");
}

/**
 * @brief Used to set the Publish Topic and the corresponding Payload.
 * 
 * @param topic     Topic to which payload has to be published.
 * @param payload   Payload of the message.
 * @return true 
 * @return false 
 */
bool MQTT::setPublishTopicPayload(char *topic, char *payload)
{
    unsigned int topicLength = strlen(topic);
    unsigned int payloadLength = strlen(payload);

    port.printf("AT+CMQTTTOPIC=0,%u\r", topicLength);
    waitForResponse(">", 1);

    port.printf("%s", topic);
    waitForResponse("OK");

    port.printf("AT+CMQTTPAYLOAD=0,%u\r", payloadLength);
    waitForResponse(">", 1);

    port.printf("%s", payload);
    return waitForResponse("OK");
}

/**
 * @brief Publish the message to the MQTT server.
 * 
 * @return true 
 * @return false 
 */
bool MQTT::publish()
{
    port.printf("AT+CMQTTPUB=0,0,120\r");
    return waitForResponse("OK", 5);
}