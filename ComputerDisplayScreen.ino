#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define LINE_1 2
#define LINE_2 12
#define LINE_3 22

// LCD for DEBUG
char ESC = 0xFE;
const char CLS = 0x51;
const char CURSOR = 0x45;
const char LCD_LINE1 = 0x00;
const char LCD_LINE2 = 0x40;
const char LCD_LINE3 = 0x14;
const char LCD_LINE4 = 0x54;
const bool isDebug = false;

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);



struct Message {
  float cpu_temp;
  float mem_total;
  float mem_free;
  float mem_avail; 
  float disk_total;
  float disk_free;        
};



// For Simple Serial Protocol
const long BAUDRATE = 9600;
const long CHAR_TIMEOUT = 500; // Millis
const char CMD_ID_RECEIVE = 'r';
const char CMD_ID_SEND = 's';
const char BOT = '&';
const char EOT = '@';
const int DATA_ITEMS = 9;
const char DATA_SEP = '|';
/* Our message is:
  BOT         1 byte
  separator:  1 bytes
  msg_type:   1 byte
  separator:  1 bytes
  cpu_temp;   4 byte
  separator:  1 bytes
  mem_total:  4 bytes
  separator:  1 bytes  
  mem_free:   4 bytes
  separator:  1 bytes
  mem_avail:  4 bytes
  separator:  1 bytes
  disk_total: 4 bytes
  separator:  1 bytes
  disk_free:  4 bytes
  separator:  1 bytes
  eot:        1 byte
  -------------------------
  Total = 32 bytes
*/
const int MAX_BYTES = 40; 


// For message
Message msg;


char buffer[MAX_BYTES];

void clearLCD() {
    Serial1.write(ESC);
    Serial1.write(CLS);
    delay(10);
}

void printLCDScreen(const char* text, char whichLine = LCD_LINE1, bool cls = true) {
    if(cls) {
      clearLCD();
    }

    Serial1.write(ESC);
    Serial1.write(CURSOR);
    Serial1.write(whichLine);
    delay(10);
    Serial1.print(text);  
  }

void setupLCD() {
  Serial1.begin(9600);
  delay(10);
  
   // Initialize LCD module
  Serial1.write(ESC);
  Serial1.write(0x41);
  Serial1.write(ESC);
  Serial1.write(0x51);

  // Set Contrast
  Serial1.write(ESC);
  Serial1.write(0x52);
  Serial1.write(40);

  // Set Backlight
  Serial1.write(ESC);
  Serial1.write(0x53);
  Serial1.write(8);

  Serial1.write(ESC);
  Serial1.write(CLS);

  Serial1.print(" NKC Electronics");

  // Set cursor line 2, column 0
  Serial1.write(ESC);
  Serial1.write(CURSOR);
  Serial1.write(LCD_LINE2);

  Serial1.print(" 16x2 Serial LCD");  
}


void debug(String debugMsg, int delayMillis = 2000) {
  if(isDebug) {
    printLCDScreen(debugMsg.c_str());
    delay(delayMillis);
  }
}

void parseField(int which_field, String field) {
  switch(which_field) {
    // BOT
    case 0:
      if(!field.indexOf(BOT) > 0) {
        debug("Missing BOT: " + field);
      }
      break;
    
    // Message Type      
    case 1:
      if(!field.indexOf(CMD_ID_RECEIVE) > 0) {
        debug("Invalid message type" + field);
      }
      break;
    case 2:
      msg.cpu_temp = field.toFloat();
      break;

    case 3:
      msg.mem_total = field.toFloat();
      break;      

    case 4:
      msg.mem_avail = field.toFloat();
      break;

    case 5:
      msg.mem_free = field.toFloat();
      break;
      
    case 6:
      msg.disk_free = field.toFloat();
      break;
      
    case 7:
      msg.disk_total = field.toFloat();
      break;

    case 8:
      if(!field.indexOf(EOT) > 0) {
        debug("Invalid BOT" + field);
      }   
      break;   
    default:
      debug("Invalid field: FieldNum[" + String(which_field) +
        "], Value = " + field);
  }
}

bool parseMessage(String message) {
  debug("String to parse:" + message);
  String field = "";
  int whichField = 0;
  int start_pos = 0;
  int pos = message.indexOf(DATA_SEP);

  while(pos >= 0) {
    field = message.substring(start_pos, pos);
    debug(message.substring(start_pos, pos));
    parseField(whichField, field);
    start_pos = pos + 1;
    whichField++;
    pos = message.indexOf(DATA_SEP, start_pos);    
  }

  if(start_pos < message.length()) {
    debug(message.substring(start_pos, message.length()));
  }

  displayMessage();
}

void displayMessage() {
  display.clearDisplay();

  // Temp
  display.setCursor(0, LINE_1);
  display.print("TEMP: ");
  display.print(msg.cpu_temp, 1);

  // Memory
  display.setCursor(0, LINE_2);
  display.print("MEM:  ");
  display.print(msg.mem_total, 1);
  display.print("/");
  display.print(msg.mem_free, 1);
  display.print("/");
  display.print(msg.mem_avail, 1);

  // Disk
  display.setCursor(0, LINE_3);
  display.print("DISK: ");
  display.print(msg.disk_free);
  display.print("/");
  display.print(msg.disk_total);

  display.display();

  delay(1000);
}


void onError(uint8_t errorNum) {
  display.clearDisplay();
  display.setTextSize(2);
  display.print("Error: ");
  display.print(errorNum);
  display.display();

  debug("SSP Error: " + String(errorNum));
}

void clearBuffer() {
  for(int x = 0; x < MAX_BYTES;x++) {
    buffer[x] = '\0';    
  }
}

void dumpBytes() {
  for(int x = 1; x < MAX_BYTES; x++) {
    char c = buffer[x];

    debug("CHAR[" + String(x) + "] = " + String(c), 1000); 
  }
}

void readMessage() {
  clearLCD();
  clearBuffer(); 
  debug("Reading message...");

  int bytes_read = Serial.readBytesUntil(EOT, buffer, MAX_BYTES);

  //dumpBytes();


  debug("Number of bytes read:" + String(bytes_read));

  buffer[bytes_read] = '\0';

  String readStr = String(buffer);
  
  debug(readStr);

  debug("Message read.");
  parseMessage(readStr);
}

void flushSerialInput() {
  int bytes_read = 0; 
  int total_bytes_read = 0;
  
  if(Serial.available()) {}

    bytes_read = Serial.readBytes(buffer, MAX_BYTES);  
    total_bytes_read += bytes_read;
    
    while(Serial.available() && bytes_read > 0) {
      bytes_read = Serial.readBytes(buffer, MAX_BYTES);
      total_bytes_read += bytes_read;
    }  

    debug("Total bytes flushed from serial input: " + String(total_bytes_read));
}


void scanI2CBus() {
  byte error, address;
  int nDevices;

  debug("Scanning...");

  nDevices = 0;
  for(address = 1; address < 127; address++ ) 
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      String addr("I2C device found at address 0x");
      
      if (address<16) 
        addr += "0";

      addr += String(address,HEX);        
      addr += "  !";
      debug(addr);

      nDevices++;
    }
    else if (error==4) 
    {
      String err = String("Unknown error at address 0x");
      if (address<16) 
        err += "0";
      err += String(address,HEX);

      debug(err);
    }    
  }
  if (nDevices == 0)
   debug("No I2C devices found\n");
  else
    debug("done\n");

  delay(5000);           // wait 5 second to view results
}
void setup() {
  Serial.begin(9600);
  Wire.begin();

  setupLCD();

  debug("Preparing to scan I2C bus...");
  
  //scanI2CBus();

  debug("Done scanning I2C bus");

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  debug("Starting I2C display");
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    debug("I2C Display failed, aborting...");
    for(;;); // Don't proceed, loop forever
  }

  debug("I2C Display succeeded");

  // Clear the buffer
  display.clearDisplay();

  display.setTextSize(1);

  // Line 1
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(LINE_1, 0);             // Start at top-left corner
  display.print("Ready");
  
  // Line 2
  display.setCursor(LINE_2, 16);
  display.print("Awaiting message.");
  display.display();

  // Flush any extraneous bytes in the serial buffer
  flushSerialInput();  
  debug("Setup done");
}

void loop() {
  //ssp.loop();
  if(Serial.available()) {
    readMessage();    
  }

  delay(1000);  
}
