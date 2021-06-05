/*
 * $Id: setup_wifi.ino,v 1.5 2021/05/27 08:24:11 gaijin Exp $
 * 
 * WiFi initialization (from inside main, Arduino setup() function).
 */

WiFiClient wifiClient;
const byte ip[] = WIFI_IPADDR;
const byte gateway[] = WIFI_GATEWY;
const byte netmask[] = WIFI_NETMSK;
const byte dnssrv[] = WIFI_DNSSRV;


void setup_wifi() {
    int conn_tries = 0;

#ifdef DEBUG
    Serial.print(F("Reset Info: "));
    Serial.println(ESP.getResetInfo());
#endif

    /*
     * An open-ended loop  here will flatten the battery on
     * the sensor if, for instance, the access point is 
     * down, so limit our connection attempts.
     */
    Serial.print("WiFi Stat: ");
    Serial.println(WiFi.status());      // Reputed to fix slow-start problem.
    WiFi.mode(WIFI_STA);
    WiFi.config(IPAddress(ip), IPAddress(gateway),
                IPAddress(netmask), IPAddress(dnssrv));
    WiFi.begin(STA_SSID, STA_PASS);
    yield();

    while ((WiFi.status() != WL_CONNECTED)
           && (conn_tries++ < WIFI_RETRIES)) {
	yield();
	ESP.wdtFeed();
        delay(100);
#ifdef DEBUG
        Serial.print(".");
#endif
    }
    if (conn_tries == WIFI_RETRIES) {
#ifdef DEBUG
        Serial.println(F("No WiFi connection ...sleeping."));
#endif
        ESP.reset();
    }

#ifdef DEBUG
    Serial.println();
    WiFi.printDiag(Serial);
    Serial.print(F("IP: "));
    Serial.println(WiFi.localIP());
#endif
}
