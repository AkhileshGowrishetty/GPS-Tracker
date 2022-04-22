#ifndef SIM7600_H
#define SIM7600_H

#include "Arduino.h"
#include <time.h>

class SIM7600
{
    public:
        SIM7600(Stream &serial);
        bool isModuleON();
        bool waitForResponse(const char *s,uint8_t timeout=3);
        bool echoOFF();
        bool start();
        bool shutdown();
        bool reset();
        void powerON();
        void powerOFF();
        bool waitingForResponse = false;

    protected:
        Stream &port;
        gpio_num_t SIM_POWER_EN = GPIO_NUM_4;
        long defaultTimeout = 3000;


};

class GPS: public SIM7600
{
    public:
        GPS(Stream &serial1):SIM7600(serial1){}
        
        typedef struct
        {
            int8_t fixmode;
            int8_t GPS_sv;
            int8_t GLONASS_sv;
            int8_t BEIDOU_sv;
            double latitude;
            double longitude;
            tm timeGPS;
            double altitude;
            double speed;
            double course;
            double dop[3];
            time_t timestamp;
        }data_t;
        data_t data;

        

        bool isOn();
        bool begin();
        bool stop();
        bool coldStart();
        bool hotStart();
        void calcLatLong(double lat, char NS, double lon, char EW);
        void formatDateTime(long date, double time);
        bool getData(bool GNSS=true);
        
        
        
    private:
        enum
        {
            PDOP = 0,
            HDOP,
            VDOP
        };
        
        
};

class SSL: public SIM7600
{
    public:
        SSL(Stream &serial1):SIM7600(serial1){}
        
        bool checkCertificates(const char *cacert, const char *clientcert, const char *clientkey);
        
        // TODO
        bool downloadCertificates();
        bool configureSSL(const char *cacert, const char *clientcert, const char *clientkey);
        
        // Not implemented as not used in this project.
        bool deleteCertificate(char *certificate);

    private:
        bool certs[3];

        enum
        {
            CACERT = 0,
            CLIENTCERT,
            CLIENTKEY
        };

};

class MQTT: public SIM7600
{
    public:
        MQTT(Stream &serial1):SIM7600(serial1){}

        bool begin();
        bool end();
        bool acquireClient();
        bool releaseClient();
        bool setSSLContext();
        bool connect(const char *serverAddress, unsigned int serverPort);
        bool disconnect();
        bool setPublishTopicPayload(char *topic, char *payload);
        bool publish();


};

#endif