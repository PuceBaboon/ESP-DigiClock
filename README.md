# ESP-DigiClock
Old-school curses-based digital clock ...with extras.

# ESP8266 (Dual) Digital Clock
This is a curses-based digital clock for the ESP8266.

## What is it?
Nothing much very novel here, except that it has a dual display, with the top display showing (NTP controlled) system time and the bottom showing the time from an i2c-connected, DS3231 RTC chip.  There are two basic ideas here.  The first is to get some idea of the long-term stability and accuracy of the DS3231 and the second is just to be able to quickly and easily set an RTC from (a fairly accurate) NTP controlled source.


## Dependencies

ESP-DigiClock encapsulates a simple, telnet-based command-line interface to your ESP.  That is, the ESP connects via WiFi to your network (as normal) and you can then use old-school (insecure, but simple and lightweight) telnet to connect to the ESP and use a simple menu to execute basic tasks.  All you need is a telnet client (which is bundled with virtually all existing Linux/BSD distributions ...or "putty" on Windows).  The simple menu system will work reliably with most, normal terminals (if you have a 3-column by 200-row display, your experience may not be optimum ...ever!), but the "curses" mode demo works best with a window set to VT220 emulation mode (if you don't know what that means just congratulate yourself for being young and carry on anyway ...nothing will blow up [uhhh, don't hold me to that promise if you intend to run a nuclear power station with your ESP8266, okay?]).


Because ESP-DigiClock uses the Ticker library to handle timing, you basically have a very simple multi-tasking system in your ESP.  It doesn't run anything simultaneously, but it does allow for other tasks to carry on running if the "current" task happens to be sitting in a wait loop of some sort.  This is by no means bullet-proof, but Ticker call-backs give you much more flexibility than a blocking delay() call.  The only down side is that you must not *ever* call "delay()" itself anywhere (including from libraries) in your program (see the lt_Delay() function for an example of how to live without it).

##### HARDWARE
This program is built for some very specific hardware.  You obviously should have a DS3231 RTC connected via i2c to utilize the full functionality.  However, the system clock will run and display quite happily even if you don't (and even if you're not actually using NTP).

The hardware which I'm using is an ESP01S with the i2c bus mapped to GPIO1 and GPIO2.  The serial RX pin is also remapped to be a standard GPIO pin and is used by the ESP8266 to control it's own power supply via a p-channel MOSFET, latching the power on when it receives a brief power connection (from either as mmomentary switch, or from the alarm signal on the DS3231 activating the MOSFET) and turning it's own power off, once the current (pun intended) task is complete.

Once the ESP01S is programmed and installed in its permanent location, the RX pin is used as the power-latch pin so that the ESP can maintain power to itself, even when the RTC alarm signal (the wake-up signal) is removed.  This means that a normal serial connection to the board will no longer work as input/output (the TX "output" will still work, but the RX "input" will not).  The firmware contains a minimal telnet server, to provide this input/output (and debug) functionality over the network.
---


### SOFTWARE


#### How do I use it?

The application requires that you configure the following settings in src/user_config.h.  These are specific to your location.

**YOU MUST** configure at least... 

+ **ntpServerName  --**  This is the fully-qualified hostname of the NTP server you wish to sync to (for instance, pool.ntp.org).

+ **timeZone  --**  This should be a positive or negative number, denoting the offset of your location from UTC-0 (for instance, -3.5 ...looking at you, Newfoundland!).

Other presets are called by using single letter arguments:-

+ -i  --  "I"2C probe.  This will trigger a probe of all standard (non-extended) i2c addresses on the bus.
+ -m  --  "M"curses test. This will clear the screen, print a simple message in the centre (mcurses test), then return to normal.
+ -n  --  "N"TP call.   Try to force an NTP call to the specified server. Obviously will not work without a WiFi connection.
+ -w  --  "W"hat base?  This is just some semi-random silliness which you can use to verify that the ESP is still alive.


##### ISSUES

###### Unset or incorrectly set ntpServerName
Failing to set the NTP server name in user_config.h will result in the incorrect time displaying in the NTP clock (surprise!).  It will also result in slow and intermittent updates to the clock, as the application will be spending a disproportionate amount of time trying to contact the server.  Wose still, it will become very difficult to use the OTA (over-the-air) update function (usually, briefly disconnecting the ESP from power and then retrying the upload in the first few seconds following reconnection will result in a successful upload).

###### No RTC (DS3231) module detected
The RTC display will display all zeros if the RTC is unavailable (for whatever reason).  The status line at the bottom of the display will display the message "DS3231 flagged as -not- running" and will update every couple of seconds.  This doesn't affect the NTP clock display and is generally harmless.

###### When I exit the program, my terminal is left in a strange, unusable state
The digital clock display can be exited by inputting a "Q" character, followed by a carriage-return.  A second "Q" and carriage-return will exit you cleanly from the application and (normally) return your terminal to its previous settings.  If that doesn't happen, you can try using the sequence "control-J reset control-J" (with no spaces) to force your terminal to reset.  If you still can't see anything, try doing "carriage-return control-and-open-square-bracket q" to ensure that you're disconnected from the telnet session.

###### Can't use OTA (Over The Air) updates consistently
Try stopping the digital clock display before requesting an OTA update (if the clock is displaying valid NTP and RTC times, it can be too busy with terminal updates to catch an OTA request).  Also see the note on "Unset or incorrectly set ntpServerName", above.

###### The RTC always resets to the same date/time at power-on (not the current date and time).
The RTC initialization code will automatically check to see whether the DS3231 has flagged an ocillator-stopped condition at start-up.  If so, it will try to set the date and time from NTP (if NTP is already available), or it will set it from the compile-time of the binary.  This latter case is probably what you're seeing and the usual cause of this is a dead, or missing, RTC battery.

