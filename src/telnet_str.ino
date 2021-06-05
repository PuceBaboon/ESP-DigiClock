/*
 *   $Id: telnet_str.ino,v 1.19 2021/06/04 22:44:28 gaijin Exp $
 *
 * TelnetStream handling to provide over-the-air console to ESP01S
 * when the TX or RX pins are in use as standard GPIOs.
 * Provides an on-screen menu for simple ESP control and is very
 * lightweight in terms of program-space usage.
 */

#include <TelnetStream.h>

void tstr_setup() {
  TelnetStream.begin();
}

/*  Globals.  */
// int conn_tries = 0;		// WiFi re-connect counter.
uint8_t count_count = 0;	// Count of counts.


/*
 * Display current WiFi status.
 */
void Stat_WiFi() {
    TelnetStream.println();
    WiFi.printDiag(TelnetStream);
    TelnetStream.print(F("IP: "));
    TelnetStream.println(WiFi.localIP());
    TelnetStream.println();
    TelnetStream.flush();
}


/*
 * Display ESP01S status.
 */
void esp_status() {
  TelnetStream.println(F(""));
  TelnetStream.println("HEAP: " +String(ESP.getFreeHeap()));
  TelnetStream.println(" ADC: " +String(analogRead(A0)));
  TelnetStream.println("GPIO: " +String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16))));
  TelnetStream.println(F(""));
  TelnetStream.flush();
}


void command_info() {
  TelnetStream.println();
  TelnetStream.println(F(" c\t-- Clock (NTP and RTC)."));
  TelnetStream.println(F(" e\t-- Display ESP01S status"));
  TelnetStream.println(F(" h or ?\t-- This help screen."));
  TelnetStream.println(F(" i\t-- Start i2c detection of devices."));
  TelnetStream.println(F(" n\t-- Trigger NTP update from remote server."));
  TelnetStream.println(F(" p\t-- Toggle the MOSFET power latch to \"OFF\"."));
  TelnetStream.println(F(" q\t-- Quit."));
  TelnetStream.println(F(" r\t-- Set the DS3231 RTC time/date from NTP."));
  TelnetStream.println(F(" s\t-- Display current WiFi status."));
  TelnetStream.println(F(" t\t-- Time (display time and date)."));
  TelnetStream.println(F(""));
  TelnetStream.flush();
}


void tstr_loop() {
  char c;

  /*
   * Console menu.
   */
  while (TelnetStream.available()) {
    c = TelnetStream.read();

    switch (c) {
       case 3:
       case 5:
       case 10:
       case 13:
       case 24:
       case 31:
       case 251:
       case 253:
       case 255:
         break;

       case 'c':
       case 'C':
         stop_clock_f = FALSE;	// Enable clock loop.
         mcurs_setup();
         command_info();
         break;

       case 'e':
       case 'E':
         esp_status();
         break;

       case 'h':
       case 'H':
       case '?':
         command_info();
         break;

       case 'i':
       case 'I':
         i2c_setup();
         i2c_loop();
         break;

       case 'n':
       case 'N':
         clk_update_req = TRUE;
         break;

       case 'p':
       case 'P':
         poweroff_f = TRUE;
         break;

       case 'q':
       case 'Q':
         /*
          * If we receive the quit command while still in
          * curses mode, clean up the screen and reset 
          * before completing a normal quit.
          */
         if (curs_status_f == TRUE) {
           stop_clock_f = TRUE;		// Stop the clock loop.
           curs_status_f = FALSE;	// Disable curses-mode status.
           clear();
           curs_set(TRUE);		// Set cursor to "visible".
           attrset(A_NORMAL);		// Reset colours to normal.
           refresh();
           endwin();			// ...and end mcurses.
           command_info();
           return;
         }
         TelnetStream.println("Quitting..");
         TelnetStream.flush();
         TelnetStream.stop();
         break;

       case 'r':
       case 'R':
         /*
          * Flag a request to set the RTC time/date from NTP.
          */
         TelnetStream.println(F("Received request to set DS3231 RTC from NTP"));
         rtc_sync_req   = TRUE;
         rtc_update_req = TRUE;
         break;

       case 's':
       case 'S':
         TelnetStream.println(F("Status:"));
         Stat_WiFi();
         break;

       case 't':
       case 'T':
         clk_update_req = TRUE;
         break;

       default:
         /*
          * Special cases -- We have a stream of data available from TelnetStream, but some
          * of it may be unwanted telnet control protocol, which we're not interested in
          * (even though your telnet client is).  Let's ignore these common bytes coming
          * in over the stream:-  3 5 24 31 251 253 255.
          */
         if (c>=32 && c<=127){
           TelnetStream.println("Unknown command: " + c);
           break;
         } else if (c == 0){
           TelnetStream.print(".");
           break;
         } else {
           TelnetStream.print("[" + String(c, DEC) + "]");
           break;
         } 
     }
  }
}
