/*
 *   $Id: mclock.ino,v 1.46 2021/05/27 08:24:11 gaijin Exp $
 *
 * Clock routines (using "mcurses", a cut-down curses library for microprocessors/controllers).
 *
 * NOTE THAT mcurses was originally written by Frank Meyer as a lightweight implementation
 *           of the curses library suitable for use on microprocessors with limited memory.
 *           It was further modified by ChrisMicro for use in the Arduino environment.
 *           See:-
 *                  http://www.mikrocontroller.net/articles/MCURSES  (Frank Meyer's original).
 *                  https://github.com/ChrisMicro/mcurses            (ChrisMicro's GitHub repo).
 *          
 * REMINDER:-  mcurses (and curses in general) starts at line 0, column 0, which is
 *             the top, L/H side of the screen.  The mcurses move() function takes
 *             Y-pos (row/line #) followed by X-pos (column #) as its arguments.
 *
 */
#include <TimeLib.h>

/* Display requires an update (flag). */
bool dupdt_f = FALSE;


/*
 * Update the time data fields for the NTP clock display.
 */
void updt_NTP() {
  uint8_t i;
  uint8_t n_hr, n_min, n_sec;

  for (i = NH1; i <= NS2; i++) {
    switch (i) {

      case NC1:		// Ignore colons.
      case NC2:
      case NH2:		// Both NH1 and NH2 handled in NH1 case.
      case NM2:		// Both NM1 and NM2 handled in NM1 case.
      case NS2:		// Both NS1 and NS2 handled in NS1 case.
        break;

      /* Handle -both- NH1 and NH2 updates. */
      case NH1:
        n_hr = hour();
        if (n_hr == 0) {
          cldisp[NH1].ddata_n = 0;
          cldisp[NH2].ddata_n = 0;
        } else {
          cldisp[NH2].ddata_n = n_hr % 10;
          cldisp[NH1].ddata_n = n_hr / 10;
        }
        if (cldisp[NH1].ddata_n != cldisp[NH1].ddata_c) {
          cldisp[NH1].ddata_c = cldisp[NH1].ddata_n;
          dupdt_f = TRUE;
        }
        if (cldisp[NH2].ddata_n != cldisp[NH2].ddata_c) {
          cldisp[NH2].ddata_c = cldisp[NH2].ddata_n;
          dupdt_f = TRUE;
        }
        break;

      /* Handle -both- NM1 and NM2 updates. */
      case NM1:
        n_min = minute();
        if (n_min == 0) {
          cldisp[NM1].ddata_n = 0;
          cldisp[NM2].ddata_n = 0;
        } else {
          cldisp[NM2].ddata_n = n_min % 10;
          cldisp[NM1].ddata_n = n_min / 10;
        }
        if (cldisp[NM1].ddata_n != cldisp[NM1].ddata_c) {
          cldisp[NM1].ddata_c = cldisp[NM1].ddata_n;
          dupdt_f = TRUE;
        }
        if (cldisp[NM2].ddata_n != cldisp[NM2].ddata_c) {
          cldisp[NM2].ddata_c = cldisp[NM2].ddata_n;
          dupdt_f = TRUE;
        }
        break;

      /* Handle -both- NS1 and NS2 updates. */
      case NS1:
        n_sec = second();
        if (n_sec == 0) {
          cldisp[NS1].ddata_n = 0;
          cldisp[NS2].ddata_n = 0;
        } else {
          cldisp[NS2].ddata_n = n_sec % 10;
          cldisp[NS1].ddata_n = n_sec / 10;
        }
        if (cldisp[NS1].ddata_n != cldisp[NS1].ddata_c) {
          cldisp[NS1].ddata_c = cldisp[NS1].ddata_n;
          dupdt_f = TRUE;
        }
        if (cldisp[NS2].ddata_n != cldisp[NS2].ddata_c) {
          cldisp[NS2].ddata_c = cldisp[NS2].ddata_n;
          dupdt_f = TRUE;
        }
        break;

      default:
        Serial.print("updt_NTP(): Unknown field in clock data.");
        break;
    }
  }
}


/*
 * Curses!  Build clock layout.
 */
void build_clock_display() {
  uint8_t fpos;

  /*
   * Loop through the preloaded field data and call display routines
   * based on the field-type.
   */
  for (fpos = 0; fpos < cdisf_pos; fpos++) {		// Screen position.
    switch (cldisp[fpos].ftype) {
      case DIGIT:
        drawDigit(cldisp[fpos].ddata_c, cldisp[fpos].ystart, cldisp[fpos].xstart);
        break;
      case COLON:
        drawColon(cldisp[fpos].ystart, cldisp[fpos].xstart);
        break;
      case STAT_SYS:
        drawStatus(STAT_SYS, cldisp[fpos].ystart, cldisp[fpos].xstart);
        break;
      case STAT_NTP:
        drawStatus(STAT_NTP, cldisp[fpos].ystart, cldisp[fpos].xstart);
        break;
      case STAT_RTC:
        drawStatus(STAT_RTC, cldisp[fpos].ystart, cldisp[fpos].xstart);
        break;
      default:
        Serial.println("[bld_clk_disp] Unknown field type.");
        break;
    }
  }							// End screen position loop.
}


/*
 * Curses!  Required by the mcurses library.
 *          This defines the function to be used
 *          as the actual output stream.  In this
 *          case we use TelnetStream, but this
 *          could be changed to "Serial" to use the
 *          ESP8266 UART instead.
 */ 
void Arduino_putchar(uint8_t c)
{
  TelnetStream.write(c);		// Define write stream for curses output.
}


/*
 * Curses!  Draw an individual digit (dig), starting at the top
 *          L/H origin and working downwards.
 */
void drawDigit(uint8_t dig, uint8_t ystart, uint8_t xstart) {
  uint8_t i, n = 0;

/*
 * #ifdef DEBUG
 *   Serial.print("D - Y: ");
 *   Serial.print(ystart);
 *   Serial.print(" - X: ");
 *   Serial.print(xstart);
 *   Serial.println("");
 * #endif
 */

  move(ystart, xstart);
  for (i = 0; i < digidots; i++, n++) {
    if (n >= digicols) {
      n = 0;
      ystart++;
    }
    move(ystart, xstart + n);
    if (digarray[dig][i]) {
      attrset(F_GREEN);
      addch(seg_ch);
    } else {
      attrset(F_BLACK);
      addch(' ');
    }
  }
  attrset(A_NORMAL);
//  refresh();
}


/*
 * Curses!  Draw a colon, starting at the top (ystart) and
 *          working downwards (both "dots" are at the same
 *          horizontal position - xstart).
 */
void drawColon(uint8_t ystart, uint8_t xstart) {
  attrset(F_GREEN | B_BLACK);
  mvaddch(ystart + 1, xstart, col_ch);
  mvaddch(ystart + 3, xstart, col_ch);
  attrset(A_NORMAL);
}


/* 
 * Return a start value for a centered string.
 *
 * Note that the display string should already have
 * been checked for length before we get here.
 */
uint8_t get_start(char *instr) {
//  const uint8_t max_w = COLS - 4;	// Width - box chars and lead/trail spaces.
  
  return ((MAX_WD - strlen(instr)) / 2);
}
 

/*
 * Curses!  Display status info on a single line.
 */
void drawStatus(uint8_t styp, uint8_t ystart, uint8_t xstart) {
//  if (curs_status_f == FALSE) return;	// Curses status not available yet.
  switch (styp) {
    case STAT_SYS:
      move(ystart, xstart);
      attrset(F_BLACK | B_WHITE);
      addstr("SYS>> ");
      addstr(sys_stat_buff);
      attrset(A_NORMAL);
      break;
    case STAT_NTP:
      move(ystart, xstart);
      attrset(F_BLACK | B_CYAN);
      addstr("NTP>> ");
      addstr(ntp_stat_buff);
      attrset(A_NORMAL);
      break;
    case STAT_RTC:
      move(ystart, xstart);
      attrset(F_BLACK | B_YELLOW);
      addstr("RTC>> ");
      addstr(rtc_stat_buff);
      attrset(A_NORMAL);
      break;
    default:
      Serial.println("[drawStatus] Unknown status type.");
      break;
  }
  attrset(A_NORMAL);
}
  

/*
 * Curses!  Draw a box using line primitives (mcurses box
 *          example code).
 *
 *     Args:- origin y, origin x, height h, width w.
 */
void drawBox(uint8_t y, uint8_t x, uint8_t h, uint8_t w)
{
  uint8_t line;
  uint8_t col;

  move (y, x);

  addch (ACS_ULCORNER);
  for (col = 0; col < w - 2; col++)
  {
    addch (ACS_HLINE);
  }
  addch (ACS_URCORNER);

  for (line = 0; line < h - 2; line++)
  {
    move (line + y + 1, x);
    addch (ACS_VLINE);
    move (line + y + 1, x + w - 1);
    addch (ACS_VLINE);
  }

  move (y + h - 1, x);
  addch (ACS_LLCORNER);
  for (col = 0; col < w - 2; col++)
  {
    addch (ACS_HLINE);
  }
  addch (ACS_LRCORNER);
}


/*
 * Cycle through the clock display.
 */
void run_dclock() {

  /*
   * Only update the display if a digit value has changed.
   */
  updt_NTP();
  updt_RTC();
  if (dupdt_f == TRUE) {
    dupdt_f = FALSE;
  } else {
    return;
  }

  build_clock_display();
  refresh();
  
}


/*
 * Curses "main()".
 */
void mcurs_setup()
{

  setFunction_putchar(Arduino_putchar);
  initscr();
  clear();
  refresh();

  /* Box outline to visualize mcurses screen size. */
  drawBox(0, 0, LINES - 1, COLS - 1);

  curs_status_f = TRUE;		// Enable curses-mode status fields.

  /* Uses lt_Delay() to generate a delay of 10-msec increments */
  while (stop_clock_f == FALSE) {	// Forever!
    curs_set(FALSE);
    lt_Delay(5);		// So 50-millisec delay.
    run_dclock();
  }

  /* 
   * Clean up the screen and reset everything to sane values before
   * switching back to normal, non-curses display mode.
   */
  curs_status_f = FALSE;	// Disable curses-mode status.
  clear();
  curs_set(TRUE);
  attrset(A_NORMAL);
  refresh();
  endwin();
}
