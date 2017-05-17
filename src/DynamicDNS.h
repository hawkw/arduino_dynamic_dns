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

#include <WString.h>
#include <stdint.h>

enum class UpdateResult { Error, Updated, Unchanged };

/* Base class representing a generic dynamic DNS service */
class DynamicDNS {
protected:
    String domain;
    String last_addr;
    EthernetClient client;

    DynamicDNS(EthernetClient ethernet_client, String domain_name)
        : domain { domain_name }
        , client { client }
        , last_addr (get_public_ip(ethernet_client))
        {};

    static String get_public_ip(EthernetClient client) {
        if (client.connect(F("checkip.dyndns.com"), 80)) {
            client.println(F("GET / HTTP/1.1\nHost: checkip.dyndns.com\n"));
            client.println();
        } else {
            return;
        }

        String buf = "";
        while(client.connected() && !client.available()) {
            delay(1);
        }
        while (client.connected() || client.available()) {
            buf = buf + client.read();
        }

        client.stop();
        return buf;
    }

public:
    virtual UpdateResult update(void) = 0;
};

class NamecheapDDNS : public DynamicDNS {
private:
    String host;
    String pass;
    EthernetClient client;

    /* Builds the Namecheap dynamic DNS update string */
    String request(void) {
        return String(F("GET /update?"))
                + F("host=") + this->host
                + F("&domain=") + this->domain
                + F("&password=") + this->pass
                + F("HTTP/1.0");
    }

public:
    NamecheapDDNS( EthernetClient ethernet_client
                 , String domain_name
                 , String password)
        : DynamicDNS(ethernet_client, domain_name)
        , pass { password }
        , host { "@" }
        {};

    NamecheapDDNS( EthernetClient ethernet_client
                 , String domain_name
                 , String password
                 , String host_name)
        : DynamicDNS(ethernet_client, domain_name)
        , pass { password }
        , host { host_name }
        {};

    UpdateResult update(void) {
        String addr = get_public_ip(this->client);

        if (addr == last_addr) {
            return UpdateResult::Unchanged;
        }

        last_addr = addr;

        if (client.connect(F("https://dynamicdns.park-your-domain.com/"), 80)) {
            client.println(this->request());
            client.println(F("Host: https://dynamicdns.park-your-domain.com/"));

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

#endif
