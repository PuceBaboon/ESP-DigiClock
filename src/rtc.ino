/*
 *   $Id: rtc.ino,v 1.25 2021/06/09 00:51:35 gaijin Exp $
 *  
 *   RTC/DS3231 specific code.
 *
 *  Note that, quite separate to the digital clock display and setting the
 *  DS3231 RTC from our NTP synchronized system clock, the primary function
 *  of the RTC in this application is to provide power to the ESP01S at
 *  regular intervals via a P-channel MOSFET connected to the alarm line of
 *  the DS3231.
 *  The ESP01S will take temperature readings at regular intervals, using
 *  the temperature sensor built in to the DS3231 chip.  Because we want to
 *  take these readings at 10~15 minute intervals, we use the DS3231's alarm-2 
 *  functionality to do a very simple "+n-minutes" calculation.  We use the
 *  A2M mask settings to give us full-minute control, without bothering with
 *  seconds accuracy (in this case the application doesn't warrant it).
 *  
 */

/*
 * NOTE - This version uses the Adafruit (JeeLib-based) lib
 *        library, as it also contains the get-temperature
 *        call, in addition to a bunch of other handy
 *        utilities.
 */
#include <RTClib.h>
RTC_DS3231 rtc;

uint8_t alarm_sleep_mins = ALM2_SLEEP_MINS;	// Time between alarms (set in user_config.h).
char temp_buff[20];

/*
 * Define BCD conversion macros.
 */
#define bcd2dec(bcd_in) (bcd_in >> 4) * 10 + (bcd_in & 0x0f)
#define dec2bcd(dec_in) ((dec_in / 10) << 4) + (dec_in % 10)


/*
 * Declare local functions.
 */
void rtc_dispTemp();
void updt_RTC();
void rtc_qtest();
void set_next_alarm();
void rtc_setup();
void rtc_mainloop();


/*
 * Use ALM_SLEEP_MINS (set in user_config.h) to configure the next wake-up for
 * the ESP01S by configuring the external, battery-backed, DS3231 RTC chip to
 * pull its SQ/ALM line low (and thus turn on the power via the P-MOSFET).
 */
void set_next_alarm(void) {
  if(!rtc.setAlarm2(rtc.now() + TimeSpan(0, 0, ALM2_SLEEP_MINS, 0), DS3231_A2_Minute)) {
    Sout("RTC ERROR: Alarm-2 set failed!");
  } else {
    Sout("Alarm-2 Set.");  
  }
}


void rtc_setup() {
  Wire.begin(I2C_SDA, I2C_SCL);		// Set the ESP01S I2C pins to:- 0=SDA, 2=SCL.
  Wire.setClockStretchLimit(1500);	// To prevent lock-ups at power-on (in µs).  

  /* 
   * Initialize the RTC. 
   */
  if(!rtc.begin()) {
    Sout("RTC: Unable to connect to the DS3231 RTC.");
    rtc_running_f = FALSE;
    return;
  } else {
    rtc_avail_f = TRUE;			// RTC is available and responding,
  }					// although clock might not be running
    
  /* 
  * To statically set the date and time, use:-
  *
  *    rtc.adjust(DateTime(2021, 5, 21, 3, 2, 1));  // Y,M,D,H,M,S.
  *         -or-
  * To automatically set it to the date and time of compilation:-
  *
  *    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  */
  if(rtc.lostPower()) {
    if (ntp_synced_f == TRUE) {
      Sout("RTC: Lost Power: Resetting time...");
      rtc_sync_req = TRUE;
    } else {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      Sout("RTC: Lost Power: Set RTC to binary compile time.");
    }
  }
    
  // We're not using the 32K signal. 
  Sout("RTC: Disabling the 32K output.");
  rtc.disable32K();
    
  // Set the DS3231 alarm-1 and alarm-2 flags, to FALSE to prevent false triggers at power-on.
  Sout("RTC: Clearing alarms (1 & 2).");
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
  // ...and specifically turn off Alarm-1, as we only use Alarm-2.
  Sout("RTC: Disabling unused Alarm-1");
  rtc.disableAlarm(1);
    
  // Set the SQ/INT pin to interrupt mode (disabling square-wave).
  Sout("RTC: Setting SQW pin to interrupt-on-alarm mode.");
  rtc.writeSqwPinMode(DS3231_OFF);
  
  /*
   * Now that we've completed the initialization, set
   * the global flag to let the system know the RTC is
   * running.
   */
  Sout("RTC: Flagging DS3231 RTC as running.");
  rtc_running_f = TRUE;

/*
    DS3231_clear_a2f();		// Our hardware adds a delay to switch-off at this point.
    set_next_alarm();		// ...so quickly go off and set the next alarm while we still have power.
*/

}	/* End of rtc_setup() */


/*
 * Quick test of RTC functionality.
 */
void rtc_qtest() {

  /*
   * Do some basic sanity checks of the RTC
   * before trying to access it for real.
   */
  if(!rtc.begin()) {
    Sout("RTC ERROR: Not initialized.");
    return;
  } else if (rtc.lostPower()) {
    Sout("RTC WARNING:  Lost power -- possible RTC battery problem.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  rtc_avail_f = TRUE;			// RTC is available and responding,
					// although clock might not be running

  /*
   * Update the RTC curses-based clock display (including
   * the RTC status-line display).
   */
  updt_RTC();
}


/*
 * Display the tempewrature measured by the DS3231 on-board
 * sensor (which is usually fairly accurate "ambient").
 */
void rtc_dispTemp() {
  sprintf(temp_buff, "Temp: %2.2fC", rtc.getTemperature());
}


/*
 * Update the time data fields for the RTC clock display.
 */
void updt_RTC() {
  uint8_t i;
  uint8_t r_hr  = 0;
  uint8_t r_min = 0;
  uint8_t r_sec = 0;

  /* Get the time from the DS3231 RTC. */
  if (rtc_avail_f != TRUE) {
    Sout("RTC: DS3231 is flagged an -not- available.");
  }
  if (rtc_running_f == TRUE) {
    // Sout("RTC: Fetching date/time from DS3231.");
    DateTime rtc_time = rtc.now();
    char dbuff[] = "DDD MMM DD YYYY";
    snprintf(rtc_stat_buff, STAT_BUFSZ - 1, "\t%s\t\t%s", rtc_time.toString(dbuff), temp_buff);
    r_hr = rtc_time.hour();
    r_min = rtc_time.minute();
    r_sec = rtc_time.second();
  }
   
  for (i = RH1; i <= RS2; i++) {
    switch (i) {

      case RC1:		// Ignore colons.
      case RC2:
      case RH2:		// Both RH1 and RH2 handled in RH1 case.
      case RM2:		// Both RM1 and RM2 handled in RM1 case.
      case RS2:		// Both RS1 and RS2 handled in RS1 case.
        break;

      /* Handle -both- RH1 and RH2 updates. */
      case RH1:
        if (r_hr == 0) {
          cldisp[RH1].ddata_n = 0;
          cldisp[RH2].ddata_n = 0;
        } else {
          cldisp[RH2].ddata_n = r_hr % 10;
          cldisp[RH1].ddata_n = r_hr / 10;
        }
        if (cldisp[RH1].ddata_n != cldisp[RH1].ddata_c) {
          cldisp[RH1].ddata_c = cldisp[RH1].ddata_n;
          dupdt_f = TRUE;
        }
        if (cldisp[RH2].ddata_n != cldisp[RH2].ddata_c) {
          cldisp[RH2].ddata_c = cldisp[RH2].ddata_n;
          dupdt_f = TRUE;
        }
        break;

      /* Handle -both- RM1 and RM2 updates. */
      case RM1:
        if (r_min == 0) {
          cldisp[RM1].ddata_n = 0;
          cldisp[RM2].ddata_n = 0;
        } else {
          cldisp[RM2].ddata_n = r_min % 10;
          cldisp[RM1].ddata_n = r_min / 10;
        }
        if (cldisp[RM1].ddata_n != cldisp[RM1].ddata_c) {
          cldisp[RM1].ddata_c = cldisp[RM1].ddata_n;
          dupdt_f = TRUE;
        }
        if (cldisp[RM2].ddata_n != cldisp[RM2].ddata_c) {
          cldisp[RM2].ddata_c = cldisp[RM2].ddata_n;
          dupdt_f = TRUE;
        }
        break;

      /* Handle -both- RS1 and RS2 updates. */
      case RS1:
        if (r_sec == 0) {
          cldisp[RS1].ddata_n = 0;
          cldisp[RS2].ddata_n = 0;
        } else {
          cldisp[RS2].ddata_n = r_sec % 10;
          cldisp[RS1].ddata_n = r_sec / 10;
        }
        if (cldisp[RS1].ddata_n != cldisp[RS1].ddata_c) {
          cldisp[RS1].ddata_c = cldisp[RS1].ddata_n;
          dupdt_f = TRUE;
        }
        if (cldisp[RS2].ddata_n != cldisp[RS2].ddata_c) {
          cldisp[RS2].ddata_c = cldisp[RS2].ddata_n;
          dupdt_f = TRUE;
        }
        break;

      default:
        Serial.print("updt_RTC(): Unknown field in clock data.");
        break;
    }
  }
}


void rtc_mainloop() {

  if (rtc_update_req == TRUE) {
    /* Do clever things(?) here. */
    rtc_update_req = FALSE;	// Reset global flag.
  }

  /* Retrieve and print the current time. */
  rtc_qtest();

  // whether a alarm happened happened, or perhaps happened?
  Serial.print(" Alarm1: ");
  Serial.print(rtc.alarmFired(1));
  Serial.print(" Alarm2: ");
  Serial.println(rtc.alarmFired(2));
  // control register values (see https://datasheets.maximintegrated.com/en/ds/DS3231.pdf page 13)
  Serial.print(" Control: 0b");
  // Serial.println(read_i2c_register(DS3231_ADDRESS, DS3231_CONTROL), BIN);

  // resetting SQW and alarm 1 flag
  // using setAlarm1, the next alarm could now be configurated
  if(rtc.alarmFired(1)) {
      rtc.clearAlarm(1);
      Serial.println("Alarm cleared");
  }
}
