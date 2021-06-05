/*
 *   $Id: time-from-ntp.ino,v 1.20 2021/05/27 08:38:13 gaijin Exp $
 *
 * TimeNTP_ESP8266WiFi.ino
 * Example from Paul Stoffregen's Time library demostrating how
 * to sync time from an NTP source.
 *
 *  = NOTE = 
 *           The NTP server name and the timezone are both set
 *           in user_conf.h and -must- be configured for your
 *           specific installation.
 */

#include <TimeLib.h>
#include <WiFiUdp.h>

WiFiUDP Udp;
unsigned int localPort = 8888;	// Local UDP listen port.
time_t prevDisplay = 0;		// When the digital clock was last displayed.

time_t getNtpTime();
void updtNtpTimeStat();
void ntp_setup();
void ntp_loop();
void digitalClockDisplay();
void printDigits(int digits, bool colon);
void sendNTPpacket(IPAddress &address);


void ntp_setup()
{
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  if (timeStatus() == timeNotSet) {
    setSyncInterval(10);
    ntp_synced_f = FALSE;
  } else {
    setSyncInterval(300);
    ntp_synced_f = TRUE;
  }
}


/*
 * Update the (mcurses) NTP clock status line with the current date
 * when the system time is synced to an NTP server, otherwise display
 * a "not synchronized" message.
 *
 * *** IMPORTANT ***
 *
 * This function also updates the DS3231 RTC time registers from our
 * NTP-synced time, when the rtc_sync_req flag is TRUE (and when we're
 * already synced to the NTP server, of course).
 */
void updtNtpTimeStat(){
  /* Bug in TimeLib 1.6.0?? */
  static const char *shortDoW[] = { "XXX", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

  if (ntp_synced_f == TRUE) {

    /* If we have and RTC date/time update pending, take care of it first. */
    if (rtc_sync_req == TRUE) {
      rtc.adjust(DateTime(year(), month(), day(), hour(), minute(), second()));
      rtc_sync_req = FALSE;
      Sout("DS3231 updated from NTP source.");
      snprintf(rtc_stat_buff, STAT_BUFSZ, "\t%s %i %s %i\t[synced from: %s]",
                      shortDoW[weekday()], day(), monthShortStr(month()), year(), ntpServerName);
    }

    /* Update the NTP status line to show weekday, date, month and year. */
    snprintf(ntp_stat_buff, STAT_BUFSZ, "\t%s %i %s %i\t[synced to: %s]",
                      shortDoW[weekday()], day(), monthShortStr(month()), year(), ntpServerName);
  } else {
    snprintf(ntp_stat_buff, STAT_BUFSZ, "    == System clock -NOT- synchronized to NTP == ");
  }
}



void ntp_loop()
{
  if (ntp_update_req == TRUE) {
    ntp_setup();
    clk_update_req = TRUE;
    ntp_update_req = FALSE;
  }

  updtNtpTimeStat();
  if (timeStatus() != timeNotSet) {
    if (clk_update_req && (now() >= (prevDisplay + 10))) {
      prevDisplay = now();
      digitalClockDisplay();
      clk_update_req = FALSE;
    }
  } else {
    setSyncInterval(10);
    ntp_setup();
  }
}


 // Convert a byte to a string.
String GetCharToDisplayInDebug(char value) 
{
  if (value>=32 && value<=126){
    return String(value);
  } else if (value == 0){
    return ".";
  } else {
    return String("[" + String(value, DEC) + "]");
  } 
}


/*
 * Display date and time.
 */
void digitalClockDisplay()
{
  bool set_colon;
  TelnetStream.println(F(""));
  TelnetStream.print(year());
  TelnetStream.print("-");
  TelnetStream.print(month());
  TelnetStream.print("-");
  TelnetStream.print(day());
  TelnetStream.println(F(""));

  printDigits(hour(), (set_colon = FALSE));
  printDigits(minute(), (set_colon = TRUE));
  printDigits(second(), (set_colon = TRUE));
  TelnetStream.println(F(""));
  TelnetStream.println(F(""));
  TelnetStream.flush();
}


/*
 * Utility for digital clock display: prints preceding colons
 * and leading zeros where required.
 */
void printDigits(int digits, bool colon)
{
  if (colon)
    TelnetStream.print(F(":"));
  if (digits < 10)
    TelnetStream.print(F("0"));
  TelnetStream.print(digits);
}


/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  char lbuff[STAT_BUFSZ];	// We already know this is less than COLS.
  char *ntpbuff = lbuff;
  IPAddress ntpServerIP;	// NTP server's ip address.

  while (Udp.parsePacket() > 0) ; // discard any previously received packets

  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);

  snprintf(ntpbuff, STAT_BUFSZ, "Sending NTP request to: %s", ntpServerName);
  Sout(ntpbuff);

  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Sout("Received NTP Response.");
      
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Sout("No response to NTP request.");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
