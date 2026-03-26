/*
18-100 S25
Lab 06 (I2C Lab) Starter Code
Last updated: 2025-03-12 by Shirley Li
Edit note: adapted to Nano ESP32
*/
#include <Arduino.h>
#include <Wire.h>
#include "bytearray.hpp"
#include "i2c_helper.hpp"

// I2C device addresses — these are fixed hardware addresses, don't change them!
#define TMP_ADDR ((byte)0x48)   // TMP102 temperature sensor
#define BTN_ADDR ((byte)0x6F)   // Qwiic button
#define MEM_ADDR ((byte)0x50)   // EEPROM memory module
#define LCD_ADDR ((byte)0x72)   // LCD display (bonus)

static byte mem_pos = 0;  /* Current position in the EEPROM to write to.
                             Starts at 0. This should increment by 2 every time
                             the button is pressed (since we store 2 bytes per
                             temperature reading), BEFORE reading or writing. */

static int memAddr  = 0x0;
static int highAddr = 0;
static int lowAddr  = 0;
static int memData  = 0;
static int curTemp  = 0;
static float tempBrightness  = 0;

static int memAddrInit[3] = {highAddr, lowAddr, 0};
static bytearray memAddrArr(memAddrInit, 3);


void setup() {
  Serial.begin(115200);   // Initializes the USB serial connection at 115200 baud
  delay(500);
  Wire.begin();           // Initializes the I2C interface with default pinouts (SDA=A4, SCL=A5)
  delay(500);

  setBackLightColor(0xFF, 0xFF, 0x00);
}

void loop() {
  // TEMP
  curTemp = readCurrentTemp();
  Serial.printf("Current Temp: %d \n\n", curTemp);

  // BTN 
  if (getBtnState()) {
    Serial.println("Button Pressed!");

    memData = curTemp;
    tempBrightness = (curTemp - 22) / 10.0 * 255.0;
    tempBrightness = (tempBrightness > 0xFF) ? 0xFF : tempBrightness;
    tempBrightness = (tempBrightness < 0) ? 0 : tempBrightness;
    
    Serial.printf("Temp Bright: %d\n", tempBrightness);
    setBtnLED(tempBrightness);

    Serial.printf("Previous Temp: %d\n", memRead(highAddr, lowAddr));
    
    memAddr += 2;
    highAddr = memAddr >> 8;
    lowAddr  = memAddr & 0xFF;

    memWrite(highAddr, lowAddr, memData);
    Serial.printf("New Temp: %d\n", memRead(highAddr, lowAddr));
    Serial.printf("Current Mem Addr: %x -> %x %x\n\n\n", memAddr, highAddr, lowAddr);

    // LCD
    sendToLCD(bytearray("Temperature     Stored!"));
    
    setBackLightColor(0xFF, (0xFF - memAddr * 2 < 0) ? 0 : 0xFF - memAddr * 2, 0x00);
  } else {
    setBtnLED(0x00);
    sendToLCD(bytearray("Click to store  temperature!"));
  }

  // Loop delay — keep this at the bottom
  delay(200);
}

/** @brief Converts the high and low bytes of temperature values to a float.
 *
 *  This function is already complete — you don't need to modify it.
 *
 *  @param high  The most-significant byte read from the temperature sensor
 *  @param low   The least-significant byte read from the temperature sensor
 *  @return      The temperature in degrees Celsius as a float */
float convertTemp(byte high, byte low) {
  int temp = (high << 4) | (low >> 4);
  return temp * 0.0625;
}

/** @brief Reads the raw temperature bytes from the temperature sensor.
 *  @return Returns the two temperature bytes as a bytearray.
 *
 *  Hints:
 *   - The TMP sensor's I2C address is defined somewhere in this file...
 *   - The TMP sensor always returns the temperature from its default register,
 *     so you only need ONE i2c call here.
 *   - The register holds 16 bits = 2 bytes. How many bytes do you want to request?
 *   - See i2c_readFrom() in i2c_helper.hpp and/or lab writeup.
 */
bytearray getTemp() {
  // bytearray inputBytearray(TMP_)
  
  bytearray temp = i2c_readFrom(TMP_ADDR, 2);
  
  // i2c_readFrom(TMP_ADDR, 1);

  return temp; // modify this — return the result of your i2c read
}

/** @brief Returns the current temperature from the TMP102 as a float in Celsius.
 *  @return The temperature in degrees Celsius.
 *
 *  Hints:
 *  - What function can you call to retrieve the raw 2-byte temp reading?
 *  - Index into the bytearray using [index] (like in Python lists).
 *  - What function can you use to convert high and low bytes into a float temp value?
 */
float readCurrentTemp() {

  bytearray bytearrayTemp = getTemp();
  float temp = convertTemp(bytearrayTemp[0], bytearrayTemp[1]);
  
  // return convertTemp(bytearrayTemp[](0), bytearrayTemp[](1));   
  return temp;   
}

/** @brief Returns whether the Qwiic button is currently being pressed.
 *  @return true if the button is pressed, false otherwise.
 *
 *  Hints:
 *  - The button's I2C address is defined in this code...
 *  - You need to access the register with the button status. What's the address?
 *    You might want to check your writeup.
 *  - To select a register on a multi-register device, first WRITE the register
 *      address, then READ the value.
 *  - Check the BUTTON_STATUS register layout to find the index of the pressed status
 *    This is in your writeup! 
 *  - To isolate bit 2, use a bitwise AND mask 
 *  - Make sure you're returning a bool (true/false statement)!
 */
bool getBtnState() {
  int arr[1] = { 0x03 };
  bytearray addr(arr , 1);
  i2c_writeTo(BTN_ADDR, addr);
  bytearray btnData = i2c_readFrom (BTN_ADDR, 1);

  if (btnData[0] & 0x04) {
    // Serial.printf("Data: %d and %d are equal with output %d \n", btnData[0], 0x04, (btnData[0] & 0x04));
    return true;
  }

  return false; 
}

/** @brief Sets the brightness of the Qwiic button's LED.
 *  @param brightness  0 = off, 255 = max brightness, 1–254 = intermediate levels.
 *
 *  Hints:
 *  - The button's I2C address is defined in this code...
 *  - What address do you need to write the led brightness to? Check the writeup!
 *  - For a multi-register device, writing works like this:
 *    send [register_address, value] (this is a bytearray) in one i2c_writeTo() call.
 */
void setBtnLED(byte brightness) {

  int arr[2] = { 0x19, brightness };
  bytearray addr(arr , 2);
  i2c_writeTo(BTN_ADDR, addr);
  delay(20); // 20ms

  return; // remove this once your code is in place
}

/** @brief Writes a single byte of data to the given 12-bit address in the EEPROM.
 *
 *  The EEPROM uses a 12-bit memory address split into two parts:
 *    - high_addr: the upper 4 bits
 *    - low_addr:  the lower 8 bits
 *
 *  Hints:
 *  - Combine the address parts using bitwise operations. 
 *    What gates can you use? How can you move bits to the upper 4 places?
 *  - The EEPROM's device address is defined in this code...
 *  - A write sends 3 bytes in one i2c_writeTo() call. 
 *    What byes do we want to send?
 *  - After writing, you probably want to call delay(t) to give the EEPROM 
 *    time to save the value.
 *
 *  @param high_addr  Upper 4 bits of the 12-bit memory address
 *  @param low_addr   Lower 8 bits of the 12-bit memory address
 *  @param data       The byte value to store at that address
 */
void memWrite(int high_addr, int low_addr, int data) {

  int arr[3] = { high_addr, low_addr, data };
  bytearray addr(arr , 3);
  i2c_writeTo(MEM_ADDR, addr);
  delay(10); 

}

/** @brief Reads a single byte from the given 12-bit address in the EEPROM.
 *  @return The byte stored at that memory address.
 *
 *  Hints:
 *  - Combine the address the same way as in memWrite
 *  - Reading from the EEPROM is a two-step process:
 *      Step 1: Write the memory address you want to read from.
 *              Send 2 bytes. What bytes are we sending?
 *      Step 2: Read 1 byte back from the device.
 *  - What type does i2c_readFrom() return? 
 *    What type do we want this function to return? Are they the same?
 *
 *  @param high_addr  Upper 4 bits of the 12-bit memory address
 *  @param low_addr   Lower 8 bits of the 12-bit memory address
 */
byte memRead(int high_addr, int low_addr) {
  int arr[2] = { high_addr, low_addr };
  bytearray addr(arr , 2);
  i2c_writeTo(MEM_ADDR, addr);

  delay(10); 

  // Serial.printf("Memory Data: %x\n", i2c_readFrom (MEM_ADDR, 2));

  return i2c_readFrom(MEM_ADDR, 1)[0]; 
}

/** @brief Send Data to LCD
 *  @param data  Data to be sent
 */
void sendToLCD(bytearray data) {
  clearLCD();
  i2c_writeTo(LCD_ADDR, data);

  return;
}

void clearLCD() {
  int clearArr[2] = { 0x7C, 0x2D };
  bytearray clear(clearArr, 2);
  i2c_writeTo(LCD_ADDR, clear);
}

/** @brief Set backlight color for LCD
 *  @param r red value
 *  @param g green value
 *  @param b blue value
 */
void setBackLightColor(int r, int g, int b) {

  int data[5] = { 0x7C, 0x2B, r, g, b };
  i2c_writeTo(LCD_ADDR, bytearray(data, 5));

  return;
}