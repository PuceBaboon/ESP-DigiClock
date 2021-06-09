/*
 *   $Id: alarm.ino,v 1.52 2021/06/09 00:51:35 gaijin Exp $
 *  
 *  The output from the SQ/INT pin of the DS3231 is connected to
 *  the gate of a P-channel MOSFET in the positive supply line
 *  to the ESP01S. In addition, the ESP01S itself can also pull
 *  the gate of the MOSFET low via a GPIO pin, thus latching-on
 *  it's own supply voltage.
 *
 *  The I2C bus uses pins GPIO-0 (SDL) and GPIO-2 (CLK) on the
 *  ESP (both pulled up at the pin via 2K2 resistors and with
 *  additional 220-ohm current limiting resistors in series
 *  between the ESP GPIOs and the RTC (and any other devices on
 *  the I2C bus.
 *  
 *  Note that we are re-assigning the ESP01S RX pin to be a
 *  normal GPIO (GPIO3).  This means that we cannot receive
 *  UART data on this pin, so the ESP01S must be taken out of
 *  the sensor/RTC/latch board to be programmed (and forcing
 *  GPIO0 low for programming will also force GPIO3 into RX
 *  mode.
 *
 *  The ESP01S will boot when the DS3231 alarm1 is triggered 
 *  (thus taking the MOSFET gate low) and will latch on the
 *  the power-control MOSFET before starting any other task.
 *  Once the power is established and the main tasks have been
 *  performed (sensors read, data transferred via MQTT, etc), the
 *  ESP sets the next alarm time in the DS3231 registers via 
 *  the I2C bus (which will also cancel the current SQ/INT low
 *  state) and then completes any other required housekeeping
 *  before resetting the GPIO pin to remove it's own power.
 *
 *  USAGE -- The ESP01S will start-up a telnet server after
 *  connecting to WiFi, so you can simply telnet to either
 *  its hostname or IP address to see debug information (as
 *  the "RX" pin is in use as the power latch pin).
 *  Note that the telnet server is very simplistic and can only
 *  handle one session at a time.  Type "h" or "?" for help.
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "user_config.h"
#include <Wire.h>
#include <Ticker.h>		// Use Ticker for running telnet callback.
#include <TelnetStream.h>	// Provide support for telnet debug.
#include <ArduinoOTA.h>
#include "mcurses.h"
#include "mclock.h"


/*
 * Pack flags into bits.
 */
/*
typedef struct {
  uint16_t ntp_updt_rq :1;		// Trigger NTP time update from remote server.
  uint16_t rtc_updt_rq :1;		// Trigger RTC time update.
  uint16_t clk_updt_rq :1;		// Trigger (mcurses-based) clock display update.
  uint16_t rtc_sync_rq :1;		// Trigger RTC time/date set from fresh NTP data.
  uint16_t pwroff_f    :1;		// Power down system (MOSFET).
  uint16_t rtc_rng_f   :1;		// DS3231 is not present or not running.
  uint16_t curs_stat_f :1;		// Curses-based status display is active.
  uint16_t tlnt_stat_f :1;		// Telnet-based status display is active.
  uint16_t stop_clk_f  :1;		// Stop clock display (and exit curses) when TRUE.
  uint16_t ntp_syncd_f :1;		// Are we synced to an NTP server? (Not initially).
  uint16_t spare_5     :1;		// Cleverly disguised halt-and-catch-fire flag.
  uint16_t spare_4     :1;
  uint16_t spare_3     :1;
  uint16_t spare_2     :1;
  uint16_t spare_1     :1;
  uint16_t spare_0     :1;
} StatFlags;
static StatFlags sf = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
 *
 * Waiting for Godot (and inspiration).
 */

/*
 * Global state flags.
 *
 * If it's not obvious, flags ending in "_req" are set by requests, while those
 * ending in "_f" are simple status flags.
 */
bool ntp_update_req = FALSE;	// Trigger NTP time update from remote server.
bool rtc_update_req = FALSE;	// Trigger RTC time update.
bool clk_update_req = FALSE;	// Trigger (mcurses-based) clock display update.
bool rtc_sync_req   = FALSE;	// Trigger RTC time/date set from fresh NTP data.
bool poweroff_f     = FALSE;	// Power down system (MOSFET).
bool rtc_avail_f    = FALSE;	// RTC is present and responding on i2c bus.
bool rtc_running_f  = FALSE;	// DS3231 is not present or not running.
bool curs_status_f  = FALSE;	// Curses-based status display is active.
bool telnet_stat_f  = FALSE;	// Telnet-based status display is active.
bool stop_clock_f   = FALSE;	// Stop clock display (and exit curses) when TRUE.
bool ntp_synced_f   = FALSE;	// Are we synced to an NTP server? (Not initially).
bool ntp_syncomp_f  = FALSE;	// NTP sync completed (now do RTC adjust-time).

/*
 * Local defines from other source files.
 */
void ntp_setup();
void tstr_setup();
void rtc_setup();
void rtc_qtest();
void rtc_dispTemp();
void updt_RTC();


/*
 * Ticker task defines.
 */
void tickerTimerUpdate();			// Call timerX.update for all tasks.
void test_tnet_loop();
void test_mainloop();
void ntp_loop();
void tstr_loop();
void loopTicker();
void lt_Delay(uint32_t ltcount);
// == EXAMPLE ==>> Ticker timer1(printCountdown, 1000, 5);
// == EXAMPLE ==>> Ticker timer2(blink, 500);
// == EXAMPLE ==>> Ticker timer3(printCountUS, 100, 0, MICROS_MICROS);
Ticker timer1(test_mainloop, 2000, 0);		// RTC (slow) loop.
Ticker timer2(ntp_loop, 1000, 0);		// NTP (1-sec) loop.
Ticker timer3(tstr_loop, 5, 0);			// TelnetStream (fast) loop.
Ticker timer4(loopTicker, 10, 0);		// LoopTicker (10ms, so fast-ish).


/*
 * Update our scheduled tasks (Ticker).
 * 
 *   == You MUST add every new Ticker timer here ==
 * 
 */
void tickerTimerUpdate(){
  timer3.update();	// Telnet handler.
  timer4.update();	// LoopTick handler.
  timer2.update();	// NTP Handler.
  timer1.update();	// RTC Handler.
}


/* ---------------- Ticker loop/while delays ------------------ */
/*
 * Ticker loop/while delays, using callback "tick" counter (the
 * defined Ticker callback increments the "tick" count and your
 * loop or while function simply counts the ticks).
 */
uint32_t lt_ticks;				// Global.
void loopTicker()
{
  lt_ticks++;
}

/* 
 * EXAMPLE Ticker Delay.  This provides a Ticker-specific delay
 *                        function without preventing other
 *                        Ticker-based tasks from running.
 *
 * Call lt_Delay(count); from your code to get a non-blocking
 * delay.
 *
 */
void lt_Delay(uint32_t ltcount)
{
  uint32_t tempTime = lt_ticks;
  while (lt_ticks < tempTime + ltcount) {
    ESP.wdtFeed();
    tickerTimerUpdate();
  }
}

/*
 * ...and here's an example function using lt_Delay().
 *
 * Use multiples of 10-millisends (set in the timer4() definition
 * above) to set your required delay.  So, for example, an ltcount
 * of "10" will give a 100ms delay (10 * 10ms ticks), while an
 * ltcount value of "100" will give a 1-second delay.
 *
 *  #define MY_COUNT 50
 *  for (uint8_t i = 0; i < MY_COUNT; i++) {
 *    update(display);            // Do work here.
 *    lt_Delay(5);                // 50-millisec delay each time.
 *  }
 *
 * NOTE that the timing will not be exact, as the processor is
 * running other tasks between ticks and your own process (the
 * call to update(display); in this example) also takes some
 * finite time to execute.
 */
/* --------------  END Ticker loop/while delays ---------------- */


/*
 * Switch console output, depending upon whether curses display,
 * telnet or just plain old stdout is active.
 */
void Sout(const char *s_out) {
  if (curs_status_f == TRUE) {
    strncpy(sys_stat_buff, s_out, STAT_BUFSZ - 1);
  } else if (telnet_stat_f == TRUE) {
    TelnetStream.println(s_out);
    TelnetStream.flush();
  } else {
    Serial.println(s_out);
  }
}


/*
 * Define BCD conversion macros.
 */
#define bcd2dec(bcd_in) (bcd_in >> 4) * 10 + (bcd_in & 0x0f)
#define dec2bcd(dec_in) ((dec_in / 10) << 4) + (dec_in % 10)


/*
 * Power-Latch Functions
 *
 * We latch our own power on (and off) using a P-channel MOSFET 
 * connected in parallel with the physical (momentary contact)
 * power switch. When the power switch is pressed, one of the 
 * first things the ESP8266 needs to do is latch the power on.
 * When the current run of this program is finished, it simply
 * turns it's own power off again.
 *
 * ChkAutoOff() will check the run time since start-up and
 * automatically power the unit off if RUNT_MAX millis is
 * exceeded.
 */
void LatchOn() {
  pinMode(POWER_LATCH, OUTPUT);  /* Don't forget! */
  digitalWrite(POWER_LATCH, HIGH);
#ifdef DEBUG
  Serial.println("Power switch: << ON >>.");
#endif
}


void LatchOff() {
  pinMode(POWER_LATCH, OUTPUT);  /* Don't forget! */
  digitalWrite(POWER_LATCH, LOW);
  poweroff_f = FALSE;
#ifdef DEBUG
  Serial.println("Power switch: << OFF >>.");
#endif
}


void ChkAutoOff() {
  unsigned long tm_now = millis();
  if (tm_now >= RUNT_MAX) {
    LatchOff();
  }
}



/*
 *  == Header from original DS3231.ino ~ 2016 ==
 *
 * Unlike standard Arduino programs, virtually all of the action here happens in the setup()
 * function.  Our hardware consists of the ESP8266 module connected via I2C to a DS3231 RTC
 * module.  The DS3231 provides two essential functions.  The first is the actual temperature
 * measurement; we use the DS3231 on-chip sensor to provide temperature readings of the
 * ambient air temperature (it's accurate enough for our purposes).  The second function is
 * control of DC power to all of the electronics (including the DS3231 chip itself).  This is
 * implemented using the open-drain "INT" output of the DS3231 to control a P-channel MOSFET
 * in the +ve line from the battery pack.  When an alarm goes off, the INT line is actively
 * pulled low, taking the gate of the MOSFET low and switching it hard on.  This provides
 * main battery power, via a low-drop-out 3v3 regulator, to the ESP8266 and the DS3231.
 *
 * The setup() function is called at power on.  We want it to call all of the functions
 * which this particular application performs in a linear manner and then to power-off,
 * without dropping into the loop() function.
 *
 * BASIC FUNCTION
 *
 *	Initialize.
 *	Get temperature reading from DS3231.
 *	Initialize network.
 *	Initialize MQTT.
 *	Send temperature and timestamp.
 *
 */
void setup() {

 /*
  * === ESP01 ONLY ===
  *
  * Set alternate function for ESP01 RX pin (GPIO); in this case, it
  * becomes our POWER_LATCH pin (used to drive the MOSFET to control
  * power to the 3v3 circuitry, including the ESP01 itself).
  */
  Serial.begin(115200,SERIAL_8N1,SERIAL_TX_ONLY);
  pinMode(POWER_LATCH, FUNCTION_3);	// Set alternate (GPIO) function for RX...
  pinMode(POWER_LATCH, OUTPUT);		// ...and make it an output (POWER_LATCH).

  /* Before we do anything else, latch our own power supply on. */
  LatchOn();
  delay(250);

  Serial.println("Starting WiFi...");
  setup_wifi();
  delay(250);

  Serial.println("Starting NTP process...");
  ntp_setup();

  Serial.println("Starting Telnet process...");
  Serial.flush();
  TelnetStream.begin();


  /*
   * Start our scheduled tasks (Ticker.h).
   */
  Serial.println("Starting Ticker counters...");
  timer1.start();	// RTC (main, slow updates).
  timer2.start();	// NTP (1-sec update).
  timer3.start();	// Telnet (fast updates).
  timer4.start();	// LoopTick (10ms).

  /*
   * Start the other, ticker-dependant processes.
   */
  rtc_setup();

/* OTA (Over The Air) code.  DO NOT DELETE!! */
/* *INDENT-OFF* */
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 31732f297b52a5b173894a0e4a80169f
  // ArduinoOTA.setPasswordHash("31732f297b52a5b173894a0e4a80169f");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "progmem";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Starting update of " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("OTA Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
/* *INDENT-ON* */
/* End of OTA code. */

}	/* End of setup() */


/*
 * Called by Ticker.  Currently runs once every 2 seconds.
 * This is intended to be the trigger for RTC updates (read
 * of the current RTC date and time, as well as the temperature
 * reported by the DS3231 on-chip sensor).
 */
uint8_t rcnt = 0;
void test_mainloop() {
  if (rcnt++ % 5) {
    rtc_qtest();
  } else if (rcnt % 33) {
    rtc_dispTemp();
  }
}


/*  == DO NOT use delay()  -ANYWHERE- when using the Ticker library ==  */
void loop() {
  tickerTimerUpdate();
  ArduinoOTA.handle();
  if (poweroff_f == TRUE) {
    LatchOff();
  }
}

