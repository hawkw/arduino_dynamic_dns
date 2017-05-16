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

String get_public_ip(EthernetClient client) {
    if (client.connect("checkip.dyndns.com", 80)) {
        client.println("GET / HTTP/1.0\n" +
                       "Host: checkip.dyndns.com\n");
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
    return buf;
}

class DynamicDNS {
protected:
    String domain;
    String last_addr;
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
        return "GET /update?"
                + "host=" + this->host
                + "&domain=" + this->domain
                + "&password=" + this->pass
                + "HTTP/1.0";
    }

public:
    NamecheapDDNS( EthernetClient ethernet_client
                 , String domain_name
                 , String password)
        : client { ethernet_client }
        , domain { domain_name }
        , pass { password }
        , host { "@" }
        , last_addr (get_public_ip(ethernet_client))
        ();

    NamecheapDDNS( EthernetClient ethernet_client
                 , String domain_name
                 , String password
                 , String host_name)
        : client { ethernet_client }
        , domain { domain_name }
        , pass { password }
        , host { host_name }
        , last_addr (get_public_ip(ethernet_client))
        ();

    UpdateResult update(void) {
        if (String addr = get_public_ip(this->client) == last_addr) {
            return UpdateResult::Unchanged;
        }

        last_addr = addr;

        if (client.connect("https://dynamicdns.park-your-domain.com/", 80)) {
            client.println(this.request());
            client.println("Host: https://dynamicdns.park-your-domain.com/");

            while(client.connected()) {
                while(client.available()) {
                    // The server will not close the connection until it has
                    // sent the last packet and this buffer is empty
                    char read_char = client.read();
                    Serial.write(read_char);
                    // TODO: actually interpret response
                }
            }
            return UpdateResult::Updated;
        } else {
            return UpdateResult::Error;
        }
    }
};

#endif
