/*
 * $Id: user_config.h,v 1.10 2021/06/04 22:56:43 gaijin Exp $
 *
 * Configuration for local set-up (ie:- Access-point ID & PWD).
 */
#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#define DEBUG		1		// Verbose output. Set "#undef" to disable
// #undef DEBUG				// ( <== like this).

#ifndef TRUE
#define TRUE true
#endif
#ifndef FALSE
#define FALSE false
#endif


/*
 * Hardware-specific defines.
 */
// I2C GPIO Defines.
// ESP12 ->>  #define I2C_SDA	2
// ESP12 ->>  #define I2C_SCL	14
#define I2C_SDA		0	// GPIO-0  -- For the ESP01S.
#define I2C_SCL		2	// GPIO-2  -- For the ESP01S.
#define POWER_LATCH	3	// GPIO-3  -- For the ESP01S (default "RX" pin).


/*
 * NTP Configuration.
 *
 * If you have an NTP server on your local network you should use it.  Failing
 * that, try to use one of the recommended NTP pools for your geographic area.
 * Failing that, "pool.ntp.org" will try and do the right thing for both you and
 * the global NTP servers by spreading the load.
 */
// static const char ntpServerName[] = "pool.ntp.org";			/*  !! CHANGE ME !!  */
// const int timeZone = 0;	// UTC/GMT.				/*  !! CHANGE ME !!  */
// const int timeZone = -5;	// CDT US Central (Summer).		/*  !! CHANGE ME !!  */
static const char ntpServerName[] = "myNTPserver.myDomain.org";		/*  !! CHANGE ME !!  */
const int timeZone = 9;		// JST - Japan Standard Time.		/*  !! CHANGE ME !!  */


/*
 * WIFI SETUP
 *
 * Number of times to test WiFi connection status before 
 * giving up.
 */
#define WIFI_RETRIES	20

/* Assign a static IP address to your ESP8266 */
#define WIFI_IPADDR	{ 192, 168, 1, 4 }				/*  !! CHANGE ME !!  */
#define WIFI_NETMSK	{ 255, 255, 255, 0 }				/*  !! CHANGE ME !!  */
#define WIFI_GATEWY	{ 192, 168, 1, 1 }				/*  !! CHANGE ME !!  */
#define WIFI_DNSSRV	{ 192, 168, 1, 1 }				/*  !! CHANGE ME !!  */
#define WIFI_CHANNEL	11						/*  !! CHANGE ME !!  */

#define STA_SSID	"MyAccessPoint"					/*  !! CHANGE ME !!  */
#define STA_PASS	"MySecretPasswd"				/*  !! CHANGE ME !!  */
#define STA_TYPE	AUTH_WPA2_PSK

#define CLIENT_SSL_ENABLE
#define DEFAULT_SECURITY	0

#define RUNT_MAX	3 * 60 * 1000	// Max-Run-Time set to 3-minutes.
#define ALM2_SLEEP_MINS		2	// "2" for testing, 10~20 for normal operations.


/* LAST LINE */
#endif		// __USER_CONFIG_H__
