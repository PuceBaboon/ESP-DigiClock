/*
 *   $Id: mclock.h,v 1.8 2021/05/27 08:10:33 gaijin Exp $
 *
 */
// #ifdef __cplusplus
// extern "C" {
// #endif

// #ifndef __MCLOCK_H__
// #define __MCLOCK_H__

/*
 * Clock function prototypes.
 */
void drawDigit(uint8_t dig, uint8_t ystart, uint8_t xstart);
void drawColon(uint8_t ystart, uint8_t xstart);
void drawBox(uint8_t y, uint8_t x, uint8_t h, uint8_t w);
void drawStatus(uint8_t styp, uint8_t ystart, uint8_t xstart);
void mcurs_setup();
void updt_NTP();


/*
 * Choose a single character to use for displaying the
 * individual segments of the (large) digits.  Under
 * mcurses control, we could use coloured blocks, but
 * a character gives a truer retro feel to the overall
 * display.
 */
const char seg_ch = '#';
//const char seg_ch = '@';
//const char seg_ch = '0';
//const char seg_ch = '*';
const char col_ch = '0';


/* Digit overall charactertistics. */
const uint8_t digidots    = 25;		// Total "dots" to make segments.
const uint8_t digicols    = 5;		// Columns of dots on each row.


/*
 * Font defines for numerics.  The previous 3x5 font was just
 * too ugly.  Let's try a 5x5 version.
 */
const uint8_t digarray[10][25] = {
  {0,1,1,1,0, 1,0,0,0,1, 1,0,0,0,1, 1,0,0,0,1, 0,1,1,1,0},	// "0"
  {0,1,1,0,0, 0,0,1,0,0, 0,0,1,0,0, 0,0,1,0,0, 0,1,1,1,0},	// "1"
  {1,1,1,1,0, 0,0,0,0,1, 0,1,1,1,0, 1,0,0,0,0, 1,1,1,1,1},	// "2"
//  {0,1,1,1,0, 1,0,0,0,1, 0,0,1,1,1, 1,0,0,0,1, 0,1,1,1,0},	// "3" - Like "8" char.
  {1,1,1,1,1, 0,0,0,1,0, 0,0,1,1,0, 1,0,0,0,1, 0,1,1,1,0},	// "3" - Flat-top.
  {0,0,0,1,1, 0,0,1,0,1, 0,1,0,0,1, 1,1,1,1,1, 0,0,0,0,1},	// "4"
  {1,1,1,1,1, 1,0,0,0,0, 1,1,1,1,0, 0,0,0,0,1, 1,1,1,1,0},	// "5"
  {0,0,1,0,0, 0,1,0,0,0, 1,1,1,1,0, 1,0,0,0,1, 0,1,1,1,0},	// "6"
  {1,1,1,1,1, 0,0,0,0,1, 0,0,0,1,0, 0,0,1,0,0, 0,1,0,0,0},	// "7"
  {0,1,1,1,0, 1,0,0,0,1, 0,1,1,1,0, 1,0,0,0,1, 0,1,1,1,0},	// "8"
  {0,1,1,1,0, 1,0,0,0,1, 0,1,1,1,0, 0,0,0,1,0, 0,0,1,0,0},	// "9"
};

/*
 * Layout for clock display.
 *
 *   H H : M M : S S
 *
 * There are two clock displays on one virtual screen. 
 * The first for the current NTP time and the second for
 * the DS3231 RTC time.
 *
 * The displays are offset vertically on the 24x80 virtual
 * screen.
 */
const uint8_t cdisf_pos  = 19;		// Clock display field positions (count).
#define NCLK_LN		4		// Start line (y) for NTP clock.
#define RCLK_LN		13		// Start line (y) for RTC clock.
#define MAX_WD		COLS - 4	// Width - box chars and lead/trail spaces.

/*
 * Create buffers for system, NTP and RTC status messages.
 */
#define STAT_BUFSZ	MAX_WD - 6	// Line minus status prompt size.
char sys_stat_buff[STAT_BUFSZ];		// Holder for System messages. 
char ntp_stat_buff[STAT_BUFSZ];		// Holder for NTP status messages.
char rtc_stat_buff[STAT_BUFSZ];		// Holder for RTC status messages.

/*
 * Define text format types (for status strings).
 */
enum {
  LEFT,
  RIGHT,
  CENTRE,
};


/*
 * Define field types.
 */
enum {
  DIGIT,
  COLON,
  STAT_SYS,		// LINES - 1.
  STAT_NTP,		// NTP info.
  STAT_RTC,		// RTC info.
};


/*
 * Define the order of display of the clock
 * fields.
 * These don't change, although the data in the
 * fields themselves obviously does.
 */
enum {
  /* Subsection: NTP Clock Display */
  NH1,			// First digit of hour.
  NH2,			// Second digit of hour.
  NC1,			// First colon.
  NM1,			// First digit of minutes.
  NM2,			// Second digit of minutes.
  NC2,			// Second colon.
  NS1,			// First digit of seconds.
  NS2,			// Second digit of seconds.
  //
  NST,			// Status line for NTP.

  /* Subsection: RTC Clock Display */
  RH1,			// First digit of hour, etc.
  RH2,
  RC1,
  RM1,
  RM2,
  RC2,
  RS1,
  RS2,
  //
  RST,			// Status line for RTC.

  /* Subsection: System Status Display */
  SST,			// Status line for system messages.
};


/*
 * Struct for clock charcter data.
 *
 * We define a structure for the individual fields
 * and then immediately initialize an array of 
 * those structures with the data for the clock.
 */
struct ccdata {
  uint8_t ftype;      /* Field type. */
  uint8_t ystart;     /* Line/row start. */
  uint8_t xstart;     /* Column start. */
  uint8_t ddata_c;    /* Current data. */
  uint8_t ddata_n;    /* New data. */
  char *sdata;        /* Pointer to status info (Sys/NTP/RTC). */
} cldisp[] = {
  /* This is the initial field data for the clocks */
  { DIGIT, NCLK_LN, 16, 0, 0, NULL, },		// NH1.
  { DIGIT, NCLK_LN, 23, 1, 0, NULL, },		// NH2.
  { COLON, NCLK_LN, 30, 0, 0, NULL, },		// NC1.
  { DIGIT, NCLK_LN, 33, 2, 0, NULL, },		// NM1.
  { DIGIT, NCLK_LN, 40, 3, 0, NULL, },		// NM2.
  { COLON, NCLK_LN, 47, 0, 0, NULL, },		// NC2.
  { DIGIT, NCLK_LN, 50, 4, 0, NULL, },		// NS1.
  { DIGIT, NCLK_LN, 57, 5, 0, NULL, },		// NS2.
//
//  { STAT_NTP, (NCLK_LN + 7), 2, 0, 0, ntp_stat_buff, },	// NST. Below clock.
  { STAT_NTP, (NCLK_LN - 2), 2, 0, 0, ntp_stat_buff, },		// NST. Above clock.
//
  { DIGIT, RCLK_LN, 16, 6, 0, NULL, },		// RH1.
  { DIGIT, RCLK_LN, 23, 7, 0, NULL, },		// RH2.
  { COLON, RCLK_LN, 30, 0, 0, NULL, },		// RC1.
  { DIGIT, RCLK_LN, 33, 8, 0, NULL, },		// RM1.
  { DIGIT, RCLK_LN, 40, 9, 0, NULL, },		// RM2.
  { COLON, RCLK_LN, 47, 0, 0, NULL, },		// RC2.
  { DIGIT, RCLK_LN, 50, 1, 0, NULL, },		// RS1.
  { DIGIT, RCLK_LN, 57, 2, 0, NULL, },		// RS2.
//
//  { STAT_RTC, (RCLK_LN + 7), 2, 0, 0, rtc_stat_buff, },	// RST. Below clock.
  { STAT_RTC, (RCLK_LN - 2), 2, 0, 0, rtc_stat_buff, },		// RST. Above clock.
//
//
  { STAT_SYS, (LINES - 3), 2, 0, 0, sys_stat_buff, },		// SST.
};


// #endif		// ifndef __MCLOCK_H__
// #ifdef __cplusplus
// }
// #endif
