/*
    Arduino dynamic DNS library

    by Eliza Weisman
    eliza@elizas.website

 */

#ifndef Dynamic_DNS_h
#define Dynamic_DNS_h

#if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
#else
    #include "WProgram.h"
#endif

#ifdef Ethernet
    #ifdef Ethernet2
        #include <Ethernet2.h>
    #else
        #include <Ethernet.h>
    #endif
#else
    // TODO: add support for Wifi, etc
#endif

#ifdef ArduLog_h
    #include <ArduLog.h>
    #define DDNS_DEBUGLN(m) DEBUGLN(m)
    #define DDNS_DEBUG(m) DEBUG(m)
#else
    #define DDNS_DEBUGLN(m) {}
    #define DDNS_DEBUG(m) {}
#endif

#include <WString.h>
#include <stdint.h>

#define CHECK_IP_HOST "api.ipify.org"

enum class UpdateResult { Error, Updated, Unchanged };

String get_public_ip(void) {
    EthernetClient client;
    DDNS_DEBUG("getting public IP");
    do {
        // close any connection before send a new request.
        // This will free the socket on the WiFi shield
        client.stop();
        #ifdef DEBUG
            Serial.print('.');
        #endif

    } while (!client.connect(CHECK_IP_HOST, 80));
    #ifdef DEBUG
        Serial.println();
    #endif
    DDNS_DEBUGLN("commected to " CHECK_IP_HOST);
    client.println(F("GET / HTTP/1.0"));
    client.println(F("Host: " CHECK_IP_HOST ));
    client.println(F("Connection: close"));
    client.println();
    String buf = "";
    while(client.connected() && !client.available()) {
        delay(1);
    }
    while (client.connected() || client.available()) {
        char c = client.read();
        buf = buf + c;
        #ifdef DEBUG
            Serial.print(c);
        #endif
    }

    client.stop();
    #ifdef DEBUG
        Serial.println();
    #endif
    String ip = buf.substring(buf.lastIndexOf('\n') + 1);
    return ip;
}

/* Base class representing a generic dynamic DNS service */
template <const char* REQUEST_URL>
class DynamicDNS {
protected:
    String domain;
    String last_addr;

    virtual String make_request(void) = 0;

    DynamicDNS(String domain_name)
        : domain { domain_name }
        , last_addr { "" }
        {};

public:
    UpdateResult update(void) {
        DDNS_DEBUG("Updating ");
        #ifdef DEBUG
            Serial.println(REQUEST_URL);
        #endif

        String addr = get_public_ip();

        DDNS_DEBUG("Current public IP is ");
        #ifdef DEBUG
            Serial.println(addr);
        #endif

        if (addr == last_addr) {
            DDNS_DEBUGLN("IP address unchanged");
            return UpdateResult::Unchanged;
        }
        EthernetClient client;
        last_addr = addr;


        if (client.connect(REQUEST_URL, 80)) {
            DDNS_DEBUG("Connected to ");
            #ifdef DEBUG
                Serial.println(REQUEST_URL);
            #endif
            client.println(this->make_request());
            client.println();

            while(client.connected()) {
                while(client.available()) {
                    // The server will not close the connection until it has
                    // sent the last packet and this buffer is empty
                    char read_char = client.read();
                    Serial.write(read_char);
                    // TODO: actually interpret response
                }
            }
            client.stop();
            return UpdateResult::Updated;
        } else {
            return UpdateResult::Error;
        }
    }
};
constexpr const char NAMECHEAP[] = "dynamicdns.park-your-domain.com";

class NamecheapDDNS : public DynamicDNS<NAMECHEAP> {
private:
    String host;
    String pass;
protected:
    /* Builds the Namecheap dynamic DNS update string */
    String make_request(void) {
        return String(F("GET /update?"))
                + F("host=") + this->host
                + F("&domain=") + this->domain
                + F("&password=") + this->pass
                + F(" HTTP/1.0\n"
                    "Host: dynamicdns.park-your-domain.com\n"
                    "Connection: close");
    }

public:
    NamecheapDDNS(String domain_name, String password)
        : DynamicDNS<NAMECHEAP>(domain_name)
        , pass { password }
        , host { "@" }
        {};

    NamecheapDDNS(String domain_name, String password, String host_name)
        : DynamicDNS<NAMECHEAP>(domain_name)
        , pass { password }
        , host { host_name }
        {};

};

#endif
