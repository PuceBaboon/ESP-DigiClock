/*
 *   $Id: i2c.ino,v 1.4 2021/05/27 08:24:11 gaijin Exp $
 */

#define MAX_BUFF 20

const uint8_t FIRST = 0x00;
const uint8_t LAST = 0x7F;
const uint8_t SPEED_1K = 100;

void i2c_read_byte(uint8_t sl_addr, uint8_t rd_addr);
void i2cdetect(uint8_t first, uint8_t last);
void i2cdetect();


void i2cdetect(uint8_t first, uint8_t last) {
    uint8_t i, address, error;
    uint8_t we_count = 0;
    char buff[10];

    // table header
    sprintf(buff, "   ");
    for (i = 0; i < 16; i++) {
        sprintf(buff, "%3x", i);
        TelnetStream.print(buff);
    }

    // addresses 0x00 through 0x77
    for (address = 0; address <= 119; address++) {
        if (address % 16 == 0) {
            sprintf(buff, "\r\n%02x:", address & 0xF0);
            TelnetStream.print(buff);
        }
        if (address >= first && address <= last) {
            Wire.beginTransmission(address);
            error = Wire.endTransmission();
            if (error == 0) {
                // Device found.
                sprintf(buff, " %02x", address);
                TelnetStream.print(buff);
            } else if (error == 1) {
                TelnetStream.print(" BE");	// BUS error.
            } else if (error == 2) {
                TelnetStream.print(" WE");	// WRITE error.
		we_count++;
            } else if (error == 3) {
                TelnetStream.print(" UE");	// Unspecified error.
            } else if (error == 4) {
                TelnetStream.print(" RE");	// READ error.
            } else if (error == 8) {
                TelnetStream.print(" CE");	// CLOCK STRETCHING error (timeout).
            } else if (error == 16) {
                TelnetStream.print(" ME");	// MASTER READ error (sent read req for 0 bytes).
            } else if (error == 32) {
                TelnetStream.print(" PE");	// POLLING error (ACK polling timeout).
            } else {
                // Something else completely.
                TelnetStream.print(" --");
            }
        } else {
            // Address not scanned.
            TelnetStream.print("   ");
        }
    }
    TelnetStream.println(F(""));
    TelnetStream.println(F("\t\t- - - - - -"));
    if (we_count > 115) {
        TelnetStream.println(F("No I2C devices detected on bus."));    
    } else {
        TelnetStream.println(F("BE = BUS Error.\t\tWE = WRITE Error"));
        TelnetStream.println(F("RE = READ Error.\tCE = CLOCK Error"));
        TelnetStream.println(F("ME = MASTER Error.\tPE = POLLING Error"));
        TelnetStream.println(F("\"  \" = Indicates Address Not Scanned"));
        TelnetStream.println(F("UE -OR- -- Are Unknown Errors"));
    }
    TelnetStream.println(F(""));
    TelnetStream.flush();
}


String macToStr(const uint8_t * mac) {
    String result;
    for (int i = 0; i < 6; ++i) {
        result += String(mac[i], 16);
        if (i < 5)
            result += ':';
    }
    return result;
}


void i2c_setup() {

    /* Initialize I2C */
    Wire.begin(I2C_SDA, I2C_SCL);       // Data, Clock, defined in user_config.h.
    Wire.setClockStretchLimit(1500);    // in µs.
}


void i2c_loop() {
   i2cdetect(0x03, 0x77);      // 0x03~0x77 is the default range.
}
