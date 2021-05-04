/*
 * iButton Cloner for Arduino Nano - Ella Jameson
 * Reading tested on RW1990 and DS1996
 * Writing only tested on RW1990
 * 
 * The fob has 16 persistent memory slots selected by 4 GPIO pins.
 * 
 * The READ button is for reading fob into current momory slot.
 * Hold the button, the red light means it is ready to read.
 * Tap the fob, 5 green blinks mean the code was memorized.
 * 
 * The WRITE button is for writing the code in the current memory
 * slot to the fob. Before pressing write, firmly press the fob
 * against the contacts. Press WRITE. The red flickering light
 * means the fob is being programmed. NO NOT REMOVE THE FOB.
 * 5 green blinks means programming was successful.
 * 5 red blinks means no code is saved in this memory slot.
 * 
 * Holding both buttons for 3 seconds will clear the current
 * memory slot. The LED will blink red 3 times slowly, then
 * green 3 times quickly after the memory slot is cleared.
 * 
 * The serial console allows reading the memory slot, writing
 * new codes to the slot, and clearing the slot. When setting
 * a memory slot via the serial console, you can set an ASCII
 * name up to 8 characters that will display in the serial
 * console when reading the slot. Clearing the slot either
 * through the console or buttons will erase the name.
 *
 *
 * PCB for this project can be found at: tinyurl.com/y8ftf3fu
 * 
 * 
 *  3.3V
 *    |                   /
 *   <                   /
 *     > 4.7k           /
 *   <                 /
 *     >              /
 *    |              /
 *    |--GPIO       /
 *    |         |-||
 *    |--CENTER(| ||
 *              |-||
 *    |-------SHELL
 *    |
 *   GND
 *   
 *   The serials are 8 bytes long, but the first is always 0C (the family code),
 *   and the last is always a CRC (calculated value). So, only 6 bytes are user-programmable.
 *   Therefore, the serial console only allows the user access to those 6 unique bytes.
 * 
 * When pasting batch command strings to the serial input, it's best to enable SW and HW flow control. (PlatformIO monitor, CTRL+T, CTRL+H)
 * 
 * You can use the following command string to wiping the data in EEPROM (All memory slots):
 * am0cm1cm2cm3cm4cm5cm6cm7cm8cm9cmAcmBcmCcmDcmEcmFcd
 */

#define PRINT_UID_to_serial Serial.print(F("<UID> AT ")); Serial.println(__LINE__);

#define USE_SERIAL true  //set to true for Serial output, false for no Serial at all.
#define S if(USE_SERIAL)Serial

#define PRINT_DEBUG_SERIAL false  //set to true for Serial output, false for no Serial at all.
#define SDBGprint if(PRINT_DEBUG_SERIAL)Serial.print
#define SDBGprintln if(PRINT_DEBUG_SERIAL)Serial.println


#define IBUTTON 10 //iButton center, see above graphic

#define RED 15 //Red LED annode
#define GREEN 6 //Green LED annode

#define READ 8 //Grounding button for reading
#define WRITE 9 //Grounding button for writing

#include <Arduino.h>
#include <EEPROM.h>
#include <OneWire.h>


const PROGMEM int SLOT[] = {9, 7, 6, 5}; //pins for activeMemSlot address, LSB first, grounding switches

OneWire ibutton(IBUTTON);

void(* resetFunc) (void) = 0; //declare reset function @ memory address 0, essentially reseting the whole program, without rebooting the MCU itself...

byte addr[8]; //Buffer for address for iButton.search();
byte code[8]; //Buffer for manipulations with address;
bool read_pressed, write_pressed;
int8_t serial_choice = -1; //0=show, 1=edit, 2=clear, 3=dump, 4=read iBtn, 5=write iBtn, 6=list iBtn, -1=none
byte activeMemSlot = 0; //only lower nibble (first 4 bits of the byte) is used
bool advancedMode = false;


void printMenu() { //Serial menu...
  S.println(F("===Welcome to iButton Cloner Serial Console==="));
  S.println(F("Enter 'L' to LIST / detect / read connected iButton."));
  S.println(F("Enter 'S' to SHOW contents of the currently active memory slot."));
  S.println(F("Enter 'R' to READ FROM iButton to currently active memory slot."));
  S.println(F("Enter 'W' to WRITE TO iButton from currently active memory slot."));
  S.println(F("Enter 'E' to EDIT the currently active memory slot."));
  S.println(F("Enter 'C' to CLEAR the currently active memory slot."));
  S.println(F("Enter 'D' to DUMP all memory slots."));
  S.println(F("Enter '|' (pipe) to stop code execution, until iButton is detected (allows for batch operation / command queuing)."));
  S.println(F("Enter 'A' to enter / toggle ADVANCED options mode."));
  if (advancedMode) {
  S.println();
  S.println(F("===Advanced commands==="));
  S.println(F("Enter 'M' to change active memory slot."));
  }
  S.println();
}

//todo: Make serial_parser return serial_choice & feed it into other function supposed to run correct menu function...
//todo: Maybe make it output the character, or an int, and we can get rid of global serial_choice completely!
void serial_parser() { 
  delay(30);                          //wait for serial to catch up
  byte currChar;
  // while(Serial.available() > 0) {  //while data is left in the serial buffer
  if(Serial.available() > 0) {        //Replaced the while above.
    currChar = Serial.read();         //read (pop) a byte off the serial buffer
    if(serial_choice == -1) {         //if so serial state is set to run
      currChar = toupper(currChar);   //capitalize

      switch (currChar) {
      case 'S':           //SHOW command
        serial_choice = 0;
        break;
      case 'E':           //WRITE command
        serial_choice = 1;
        break;
      case 'C':           //CLEAR command
        serial_choice = 2;
        break;
      case 'D':           //DUMP command
        serial_choice = 3;
        break;
      case 'R':           //READ_FROM_IBUTTON command
        serial_choice = 4;
        break;
      case 'W':           //WRITE_TO_IBUTTON command
        serial_choice = 5;
        break;
      case 'L':           //LIST_IBUTTON command
        serial_choice = 6;
        break;
      case '|':           //WAIT command
        serial_choice = 7;
        break;
      case 'A':           //ADVANCED menu
        serial_choice = 8;
        break;
      case 'M':           //MEMORY_SELECT menu
        serial_choice = 9;
        break;
      default:            //For easier adding of new characters.
        S.print("You have written: ");
        S.println((char)currChar);
        S.println(currChar, HEX);
        S.println(currChar);
        break;
      }
    }
  }                                   //END of reading from serial
}

int writeByte(byte data){
  int data_bit;
  for(data_bit=0; data_bit<8; data_bit++){
    if (data & 1){
      digitalWrite(IBUTTON, LOW); pinMode(IBUTTON, OUTPUT);
      delayMicroseconds(60);
      pinMode(IBUTTON, INPUT); digitalWrite(IBUTTON, HIGH);
      delay(10);
    } else {
      digitalWrite(IBUTTON, LOW); pinMode(IBUTTON, OUTPUT);
      pinMode(IBUTTON, INPUT); digitalWrite(IBUTTON, HIGH);
      delay(10);
    }
    data = data >> 1;
  }
  return 0;
}

void blinkPin(int LED, int cycles, int period) {
  for(int i=0; i<cycles; i++) {
    digitalWrite(LED, HIGH);
    delay(period/2);
    digitalWrite(LED, LOW);
    delay(period/2);
  }
}

void clear_serial() {
#if USE_SERIAL == true
  S.println(F("[WARNING] Clearing serial input. Any queued commands are lost now."));
  while(Serial.available() > 0) {
    // char a = Serial.read(); //unnecessary??
    Serial.read();
  }
#endif
}

void wait_for_serial_input() {  //While there is NO serial input... Wait... //todo: should we add "timeout" parameter here?
#if USE_SERIAL == true
  while (Serial.available() < 1) {  //While there is NO serial input...
    delay(1);                       //Wait...
    yield();
  }
#endif
}

void bad_form() { //Called on some invalid serial input, not always though. Mainly fom edit_slot() function
#if USE_SERIAL == true
  Serial.flush();
#endif
  clear_serial();
  S.println(F("[WARNING] Exiting from any functions to the main menu!"));
  delay(50);    //If there won't be this delay, the serial won't finisht printing & glitches out.
  resetFunc();  //When this runs, it appears, that it'll reset loop / end all functions. I'm not actually sure what's the purpose of this though...
}

int hex_digit_val_dec(char ch) {
  int returnType;
  switch(ch)
  {
    case '0':
      returnType = 0;
      break;
    case  '1' :
      returnType = 1;
      break;
    case  '2':
      returnType = 2;
      break;
    case  '3':
      returnType = 3;
      break;
    case  '4' :
      returnType = 4;
      break;
    case  '5':
      returnType = 5;
      break;
    case  '6':
      returnType = 6;
      break;
    case  '7':
      returnType = 7;
      break;
    case  '8':
      returnType = 8;
      break;
    case  '9':
      returnType = 9;
      break;
    case  'A':
      returnType = 10;
      break;
    case  'B':
      returnType = 11;
      break;
    case  'C':
      returnType = 12;
      break;
    case  'D':
      returnType = 13;
      break;
    case  'E':
      returnType = 14;
      break;
    case  'F' :
      returnType = 15;
      break;
    default:
      returnType = -1;
      break;
  }
  return returnType;
}

int hexstr_to_int(String hex) {
  int ret = 0;
  if(hex.length() == 2) {
    int temp = hex_digit_val_dec(toupper(hex[0]));

    if(temp == -1) {
      return -1;
    }
    ret += temp * 16;

    temp = hex_digit_val_dec(toupper(hex[1]));
    if(temp == -1) {
      return -1;
    }
    ret += temp;
  }
  else {
    return -1;;
  }
  return ret;
}

void update_slot() {  //Make a memory slot active according to GPIO pins
  if (advancedMode) return; //IF is in advanced mode, do not change activeMemSlot.

  activeMemSlot = 0x00;
  for(int x = 0; x < 4; x++) {
    activeMemSlot = activeMemSlot | (digitalRead(SLOT[x]) << x);
  }
  for(int x = 0; x < 8; x++) {
    code[x] = EEPROM.read(x + (activeMemSlot << 5));
  }
}

bool set_active_mem_slot(char c) {  //If it's one of the valid numbers, change activeMemSlot
  if (hex_digit_val_dec(c) != -1) {
        activeMemSlot = hex_digit_val_dec(c);
        return true;
  }
  return false;
}

bool slot_is_full(byte memSlot) { //Check whether the given memory slot contains saved (any) data
  byte result = 0x00;
  for(int x = 0; x < 8; x++) {
    result = result | EEPROM.read(x + (memSlot << 5));
  }
  if(result == 0x00) {
    return false;
  }
  else {
    return true;
  }
}

bool serial_parse_hex(int result[], uint8_t arraySize) {  //Reads hex values from serial in "0x01, 0x02, 0x03, ..." format. 

  // int amount = sizeof(result) / sizeof(result[0]);  //We can't do this :( We must pass the size as parameter...

  /* When you use a name of the array as an argument in a function call, 
  the compiler treats it as a pointer whose value is the address of the first element of the array, 
  using pointer notation or using array notation.

  The function can use the value of that pointer to access (read and/or write) any element in the array.

  Since the function has no way of knowing the size of the array unless you tell it, 
  the length of the array is commonly used as a parameter (in addition to the name of the array).
  https://forum.arduino.cc/t/passing-array-as-parameter/44562/3
  */

  String hexstr[arraySize];
  for(int x = 0; x < arraySize; x++) {                  ////sizeof(hexstr) / sizeof(hexstr[0]) will give back total number of Bytes, not array length.                        
    byte justTheFirstOne = Serial.read();
    if(justTheFirstOne != '0') {                          ////See https://www.arduino.cc/reference/en/language/variables/utilities/sizeof/#_notes_and_warnings
      if (justTheFirstOne == 10 || justTheFirstOne == 13) { //if it has been just CR (10) or LF (13), then:
        clear_serial();                                     //clear the serial input, (undesirable?)
        return false;                                       //and return false.
      }
      S.println(F("[ERROR] No leading 0 found (before leading x).\n"));
      bad_form();
      return false;
    }
    if(tolower(Serial.read()) != 'x') {
      S.println(F("[ERROR] No leading x found (before 2 hex digits).\n"));
      bad_form();
      return false;
    }
    hexstr[x] = (char)Serial.read();
    hexstr[x] += (char)Serial.read();
    if(x < arraySize-1) {
      if(Serial.read() != ',') {
        S.println(F("[ERROR] No comma found between values (after 2 hex digits).\n"));
        bad_form();
      return false;
      }
      if(Serial.read() != ' ') {
        S.println(F("[ERROR] No space found between values (after comma).\n"));
        bad_form();
      return false;
      }
    }
  }

  for(int x = 0; x < arraySize; x++) {
    result[x] = hexstr_to_int(hexstr[x]);
    if(result[x] == -1) {
      S.println(F("[ERROR] Invalid hex character.\n"));
      bad_form();
      return false;
    }
  }
  return true;
}

void write_content_to_mem_slot(byte memSlot, int result[], uint8_t arraySize) {
  
  byte start[8];                          //All 8 Bytes that store the iButton data
  if (arraySize == 6 && false) {          //arraySize of 6 is the original one, that the cycle was written for... (ADDED FALSE TO PROHIBIT THIS BLOCK OF CODE FROM RUNNING. EVER.)
                                          //Also lots of debug prints. Everywhere...
  SDBGprintln(F("<DEBUG> Is at arraySize 6 "));
  SDBGprint(F("<DEBUG>(FIRST) Writing to EEPROM at address ")); SDBGprint(0 + (memSlot << 5)); SDBGprint(F(" content: ")); SDBGprintln(0x0C);
  
    //EEPROM.write(0 + (memSlot << 5), 0x0C);   //I'm not really sure, why is this needed, like why do we need to write 0x0C as the first byte...
    //THE PREVIOUS LINE, JUST BRICKED MY iBUTTON WHEN I EDITED THE DATA FROM ANOTHER WORKING ONE,
    //AND THEN WROTE FROM THE EDITED SLOT TO RW1990. THUS, THIS WILL BE PROHIBITED UNTIL IT'S CLEAR, AS TO WHY IT'S HERE! 
    start[0] = 0x0C;
    for(int x = 0; x < 6; x++) {
    SDBGprint(F("<DEBUG>(FOR) Writing to EEPROM at address ")); SDBGprint(x + (memSlot << 5) + 1); SDBGprint(F(" content: ")); SDBGprintln((byte)result[x]);

      EEPROM.write(x + (memSlot << 5) + 1, (byte)result[x]);
      start[x + 1] = (byte)result[x];
    }

  SDBGprint(F("<DEBUG>(LAST) Writing to EEPROM at address ")); SDBGprint(7 + (memSlot << 5)); SDBGprint(F(" content: ")); SDBGprintln(ibutton.crc8(start, 7));

    EEPROM.write(7 + (memSlot << 5), ibutton.crc8(start, 7));
  } 
  else if (arraySize == 7) {                  //arraySize of 7 is about a one more, so we autofill only the last Byte - checksum

  SDBGprintln(F("<DEBUG> Is at arraySize 7 "));
  SDBGprint(F("<DEBUG>(FIRST) Writing to EEPROM at address ")); SDBGprint(0 + (memSlot << 5)); SDBGprint(F(" content: ")); SDBGprintln((byte)result[0]);

    EEPROM.write(0 + (memSlot << 5), (byte)result[0]);          //Writing to 0-th Byte in EEPROM
    start[0] = (byte)result[0];

    for(int x = 1; x < 7; x++) {

    SDBGprint(F("<DEBUG>(FOR) Writing to EEPROM at address ")); SDBGprint(x + (memSlot << 5)); SDBGprint(F(" content: ")); SDBGprintln((byte)result[x]);

      EEPROM.write(x + (memSlot << 5), (byte)result[x]);        //Writing 1 to 7-th byte in EEPROM
      start[x] = (byte)result[x];
    }

  SDBGprint(F("<DEBUG>(LAST) Writing to EEPROM at address ")); SDBGprint(7 + (memSlot << 5)); SDBGprint(F(" content: ")); SDBGprintln(ibutton.crc8(start, 7));

    EEPROM.write(7 + (memSlot << 5), ibutton.crc8(start, 7));   //Writing to 8-th Byte in EEPROM
  } 
  else if (arraySize == 8) {                  //arraySize of 8 is all of the data, so... yeah, we just do that...
    for(int x = 0; x < 8; x++) {

    SDBGprint(F("<DEBUG>(FOR) Writing to EEPROM at address ")); SDBGprint(x + (memSlot << 5)); SDBGprint(F(" content: ")); SDBGprintln((byte)result[x]);

      start[x] = (byte)result[x];
      EEPROM.write(x + (memSlot << 5), (byte)result[x]);
    }
  }

  #if PINT_DEBUG_SERIAL == true
  SDBGprint(F("<DEBUG> size of the array \"start\" is: "));
  byte serialtmp = sizeof(start) / sizeof(start[0]); SDBGprintln(serialtmp);
  #endif

  SDBGprint(F("<DEBUG> Contents of the array \"start\" are as follows: "));
  for (size_t i = 0; i < sizeof(start) / sizeof(start[0]); i++)
  {
    SDBGprint(start[i]); SDBGprint(F(", "));
  }
  SDBGprintln();
}

void edit_slot(byte memSlot, byte numberOfBytes) {  //Submenu for editing currently active slot's data and name
  update_slot();

  S.print(F("[INPUT] Waiting for code to program slot: "));
  S.println(memSlot, HEX);
  S.print(F("In the following format: "));

  int arraySize = 7;
  if (advancedMode) arraySize = numberOfBytes;
  
  SDBGprint(F("<DEBUG> advanced mode? ")); SDBGprintln(advancedMode);   //todo: press "return / enter" to just rename the slot.
  SDBGprint(F("<DEBUG> arraySize: ")); SDBGprintln(arraySize);
  if (advancedMode && arraySize == 8) {                                 //todo: modify this, so it's possible to edit range from just 1 (first) to all 8 (all of them)
    S.println(F("0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 (ALL 8 Bytes)"));
  } else if (arraySize == 7) {
    S.println(F("0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 (7 Bytes - last one is autofilled checksum)"));
  } else {
    S.println(F("0x01, 0x02, 0x03, 0x04, 0x05, 0x06 (6 Bytes)"));
  }

  S.println(F("Or just press <return> to keep the code in the memory slot unchanged. (Any invalid input will cause this also)"));

  while(Serial.available() <= 0);               //todo: can we use wait_for_serial_input() function here? Should we add "timeout" parameter to it?
  delay(50);
  
  while(Serial.available() > 0) {               //We start reading input from the serial here...

    int result[arraySize];

    if (serial_parse_hex(result, arraySize)) {  //returns true if all of the serial input was successfully parsed, otherwise returns false.

    #if PINT_DEBUG_SERIAL == true
    SDBGprint(F("<DEBUG> size of the array \"result\" is: "));
    byte serialtmp = sizeof(result) / sizeof(result[0]); SDBGprintln(serialtmp);
    #endif

    SDBGprint(F("<DEBUG> Contents of the array \"result\" are as follows: "));
    for (size_t i = 0; i < sizeof(result) / sizeof(result[0]); i++)
    { SDBGprint(result[i]); SDBGprint(F(", ")); }
    SDBGprintln();
    
    write_content_to_mem_slot(memSlot, result, arraySize);

    S.print(F("[SUCCESS] Code saved to slot "));
    S.print(memSlot, HEX);
    S.print("!\n");
    }

    S.println(F("[INPUT] Waiting for name (up to 8 characters)..."));
    while(Serial.available() < 1) delay(1);
    delay(100);                               //let serial catch up
    for(int x = 0; x < 8; x++) {
      if(Serial.available() < 1) {
        EEPROM.write(x + 8 + (memSlot << 5), 0x00);
      } else {
        byte c = Serial.read();
        if ((c == 13) || (c == 10)) c = 0x00; //if c is Line-Feed or Carriage-Return, convert it to 0x00
        EEPROM.write(x + 8 + (memSlot << 5), c);
      }
    }
    S.println(F("[SUCCESS] Name saved!\n"));
    clear_serial();                           //Need to remove this for batch command run?
  }
}

void print_mem(int lower, int upper, byte memSlot) {
  if(slot_is_full(memSlot)) {
    for (byte x = lower; x < upper; x++) {  
      byte curr_val = EEPROM.read(x + (memSlot << 5));
      S.print("0x");
      if (curr_val<0x10) {S.print("0");}
      S.print(curr_val,HEX);
      if (x < upper - 1) S.print(", ");
    }
  }
  else {
    S.print("<EMPTY SLOT>");
  }
}

void print_mem_name(byte memSlot) { //todo: Check if a memSlot is valid (0-F / 0-15) (time to make a is_slot_valid function?)
  if(EEPROM.read(0 + 8 + (memSlot << 5)) == 0x00) {
    S.print("<NONAME>");                          //Changed from "<UNNAMED>" to "<NONAME>" for exactly 8 characters
  }
  else {
    for(int x = 0; x < 8; x++) {
      byte value = EEPROM.read(x + 8 + (memSlot << 5));
      if(value == 0x00) { //If the saved value is 0...
        value = 0x20;     //We print a ' ' (space) character. For padding...
      }
      S.print((char)value);
    }
  }
}

void dump_all_mem_slots_to_serial() { //Writes all the slots to the serial, marks the currently active one.
  for(byte memSlot = 0x00; memSlot < 0x10; memSlot++) {
    S.print(memSlot, HEX);              //Print number of slot in HEX
    S.print(" : ");
    print_mem_name(memSlot);
    S.print(" : ");
    if (advancedMode)
    {
      print_mem(0, 8, memSlot);         //In Advanced mode
    } else {
      print_mem(0, 7, memSlot);         //not in Advanced mode
    }
    if (memSlot == activeMemSlot) S.print(F("  <<ACTIVE>>  "));
    S.println();
    }
}

//todo: Implement CRC checking to make sure that iButton device is present (not just garbage). See ibutton.search() desc.

bool detect_iButton() { //Returns TRUE if the iButton was detected, FALSE if any error ocurred

  if (!ibutton.search(addr)){           //read attached ibutton and assign value to buffer "addr"
    digitalWrite(RED, HIGH);
    ibutton.reset_search();
    delay(1);
    digitalWrite(RED, LOW);
    return false;                       //Returns FALSE if no iButton could be detected for any reason
  }
  ibutton.reset_search();               //If we don't reset, the next ibutton.search will fail.

  S.print(F("An address / ID of the currently connected iButton is: "));
  for (byte x = 0; x < 8; x++) {        //Print current ID to the serial...
    byte curr_val = addr[x];
    S.print("0x");
    if (curr_val<0x10) {S.print("0");}
    S.print(curr_val,HEX);
    if (x < 8 - 1) S.print(", ");
  }

  blinkPin(GREEN, 5,750);              //Signal operation success via green LED
  while(!digitalRead(READ)) delay(1);

  S.println();
  return true;                          //Returns TRUE after successful execution
}

bool read_iButton() { //Returns TRUE if the read was successful, FALSE if any error ocurred
  update_slot();
  if (!ibutton.search(addr)){           //read attached ibutton and assign value to buffer "addr"
    digitalWrite(RED, HIGH);
    ibutton.reset_search();
    delay(1);
    digitalWrite(RED, LOW);
    return false;                       //Returns FALSE if no iButton could be detected for any reason
  }
  ibutton.reset_search();               //If we don't reset, the next ibutton.search will fail.

  for(int i = 0; i < 8; i++) {
    EEPROM.write(i + (activeMemSlot << 5), addr[i]);
    code[i] = addr[i];
  }

  blinkPin(GREEN, 5, 150);              //Signal operation success via green LED
  while(!digitalRead(READ)) delay(1);
  return true;                          //Returns TRUE after successful execution
}

bool write_iButton() { //Returns TRUE if the write was successful, FALSE if any error ocurred
  update_slot();
  if(slot_is_full(activeMemSlot)) {
    if (!ibutton.search(addr)) {  //read attached ibutton and assign value to buffer "addr"
      digitalWrite(RED, HIGH);
      ibutton.reset_search();
      delay(1);
      digitalWrite(RED, LOW);
      return false;               //Returns FALSE if no iButton could be detected for any reason
    }
                                   
    ibutton.skip();               // This is code preparing RW1990 to be written to...
    ibutton.reset();              // THESE LINES ARE VITAL
    ibutton.write(0x33);          // I thought they were just for reading, but without these lines,
    ibutton.skip();               // writing will brick the fob forever! I broke 6 writing this program.
    ibutton.reset();
    ibutton.write(0xD5);
    
    for (byte x = 0; x<8; x++){
      digitalWrite(RED, HIGH);
      delay(5);
      writeByte(code[x]);
      digitalWrite(RED, LOW);
      delay(15);
    }
                                  //todo: Implement readback to make sure everything was written correctly...?
    ibutton.reset();
    ibutton.reset_search();       //If we don't reset, the next ibutton.search will fail.
    blinkPin(GREEN, 5, 150);
    while(!digitalRead(WRITE)) delay(1);

    return true;                  //TRUE, as the writing should be successfully done at this point.
  } else {
    blinkPin(RED, 5, 150);
    while(!digitalRead(WRITE)) delay(1);
    return false;                 //FALSE, as the slot is empty
  }
}

void function_caller() { //todo: Merge this with the serial parser function?
  switch (serial_choice) {          //Execute apropiate command 
      case 0:                         //Show
        S.println(F("===SHOW contents of the currently active memory slot==="));
        update_slot();
        if(slot_is_full(activeMemSlot)) {
          S.print(F("[INFO] Reading code from device slot... "));
          S.print(activeMemSlot, HEX); S.print(" : ");
          print_mem_name(activeMemSlot); S.print(" : ");
          if (advancedMode) {                 //If in advanced mode,
          print_mem(0, 8, activeMemSlot);     //print all of the 8 bytes...
          } else {                            //If not in advanced mode however...
          print_mem(0, 7, activeMemSlot);     //Don't print the last byte (CRC, a calculated cyclical redundancy check)
          // print_mem(1, 7, activeMemSlot);     //Don't print the first byte (0x0C, family code) and last byte (CRC, a calculated cyclical redundancy check)
          }
          S.println("\n");
        }
        else {
          S.print("[ERROR] No code stored in slot ");
          S.print(activeMemSlot, HEX);
          S.println(" yet.\n");
        }
        break;

      case 1:                         //Edit
        S.println(F("===Manually EDITING the currently active memory slot==="));
        if (advancedMode) {
          edit_slot(activeMemSlot, 8);
        } else {
          edit_slot(activeMemSlot, 7);
        }
        break;

      case 2:                         //Clear
        S.println(F("===CLEAR the currently active memory slot==="));
        update_slot();
        S.print(F("[INFO] Clearing slot "));
        S.print(activeMemSlot, HEX);
        S.println("...");
        
        for(int x = 0; x < 16; x++) {
          EEPROM.write(x + (activeMemSlot << 5), 0x00);
        }
          
        S.print(F("[SUCCESS] Slot "));
        S.print(activeMemSlot, HEX);
        S.println(F(" cleared!\n"));
        break;

      case 3:                         //Dump
        S.println(F("===DUMP all memory slots==="));
        S.println(F("[INFO] Dumping all slots..."));
        S.println();
        dump_all_mem_slots_to_serial();
        S.println();
        S.println(F("[SUCCESS] Done dumping all slots!"));
        S.println();
        break;

      case 4:                         //read iBtn
        S.println(F("===READ FROM iButton to currently active memory slot==="));
        S.println(F("[INFO] Reading from the iButton & saving to currently selected slot!"));

        if (read_iButton()) {
          S.println(F("[SUCCESS] Read was successful!"));
        } else {
          S.println(F("[ERROR] An error has occurred during an attemt to read the iButton!"));
          S.println(F("[INFO] Check your electrical connections!"));
        }

        S.println();
        break;
      
      case 5:                         //write iBtn
        S.println(F("===WRITE TO iButton from currently active memory slot==="));
        S.println(F("[INFO] Reading from the currently selected slot & writing to the iButton!"));

        if (write_iButton()) {
          S.println(F("[SUCCESS] Data from the current memory slot should be successfully written to the iButton."));
          S.println(F("[INFO] You can check by reading the iButton now!"));
        } else {
          S.println(F("[ERROR] An error has occurred during an attemt to write to the iButton!"));
          S.println(F("[INFO] It might be that your currently selected slot is EMPTY!"));
          S.println(F("[INFO] Check if reading does work. If not, "));
          S.println(F("[INFO] check your electrical connections!"));
        }

        S.println();
        break;

      case 6:                         //list iBtn
        S.println(F("===LIST / detect / read connected iButton==="));
        if (detect_iButton()) {
          S.println(F("[SUCCESS]"));
        } else {
          S.println(F("[ERROR] An error has occurred during an attemt to read the iButton!"));
          S.println(F("[INFO] Check your electrical connections!"));
        }
        S.println();
        break;

      case 7:                         //wait iBtn
        S.println(F("===Waiting for iButton device==="));
        S.println(F("[WARNING] Code execution is being blocked until iButton device is detected!"));
        S.println(F("[INFO] You can now queue commands and inputs for those commands."));
        S.println(F("[INFO] On bad input, however, rest of the queued commands will be invalidated!"));
        byte throwaway[8];
        while(!ibutton.search(throwaway)) {
          ibutton.reset_search();
          yield();
          delay(1);
          if (Serial.available() > 0) {
            //todo: Allow this state to be breakable by some special command / character.
            //todo: Or maybe even better - timeout!
          }
        }
        ibutton.reset_search();
        S.println(F("[SUCCESS] An iButton device has been successfully detected!"));

        delay(250);               //Debounce delay. Adjust as needed.

        S.println();
        break;

      case 8:                         //Advanced mode
        advancedMode = !advancedMode;
        S.println();

        S.println(F("===Advanced mode==="));
        if (advancedMode) {
        S.println(F("[INFO] Active memory slot selection via serial console has been ENABLED."));
        S.println(F("[INFO] Meaning you can now change memory slot by utilizing menu entry 'M'."));
        S.println(F("[WARNING] As a consequence of that, it WON'T be possible to change active memory slot via GPIO!"));
        S.println();
        S.println(F("[INFO] You CAN now see & edit the first and last bytes in iButton addresses."));
        S.println(F("[WARNING] Beware, that you can destroy your iButton by writing bad modification of these bytes!"));
        } else {
        S.println(F("[INFO] Active memory slot selection via serial console has been DISABLED."));
        S.println(F("[INFO] Meaning active memory slots WILL be changing according to GPIO!"));
        S.println();
        S.println(F("[INFO] You now CAN NOT see & edit the first and last bytes in iButton addresses."));
        }
        update_slot();              //So that we'll switch active mem slot to one in accordance of the GPIO.

        S.println();
        break;

      case 9:                         //Manual memory (activeMemSlot) change
        if (!advancedMode) break; //todo

        //display all the slots?
        
        S.println(F("===Change active memory slot==="));
        S.print(F("[INFO] Currently active memory slot is: ")); S.println(activeMemSlot, HEX);
        S.println(F("[INFO] You can press 'W' to wipe all memory slots at once." ));
        S.println(F("[INFO] Write 0-to-F to select active memory slot!"));
        S.println(F("[INFO] You can enter 'L' to show list of the memory slots, \n[INFO] or enter 'X' to cancel!"));
        S.println();

        while(true) {
          wait_for_serial_input();     //Wait while there are NO data in the serial buffer...
          char ch = toUpperCase(Serial.read());
          if (ch == 'X') break;              //If it's 'X', cancel / break.

          if (ch == 'L') {                   //If it's 'L', print the availble memory slots.
            S.println(F("List of the availble memory slots:"));
            dump_all_mem_slots_to_serial();
            S.println();
            continue;
          }
          if (ch == 'W') {                   //If it's 'W', prepare to WIPE ALL MEMORY SLOTS! "(W)IPE"
            S.println(F("[WARNING] YOU ARE ATTEMTING TO WIPE ALL MEMORY SLOTS IN THE DEVICE'S MEMORY!"));
            S.println(F("[WARNING] TO CONFIRM THIS OPERATION FINISH THE WORD \"(W)IPE\" (you've already wrote the \"W\")."));
            S.println(F("[INFO] YOU MUST WRITE THE \"IPE\" IN CAPITAL LETTERS."));
            S.println(F("[INFO] To skip this confirmation, you can just write \"WIPE\" the next time after selecting menu option 'm'."));
            S.println();

            PROGMEM const byte KEY[3] = {'I', 'P', 'E'};  //KEY CHARACTERS for confirming wipe.
            byte serialBuffer[3];
            for(int x = 0; x < 3; x++) {
              wait_for_serial_input();
              serialBuffer[x] = Serial.read();
              if (serialBuffer[x] != KEY[x]) {            //IF the letters - as they're coming in - doesn't match with the KEY, abort!
                S.println(F("[ERROR] Invalid input! Wipe cancelled!"));
                clear_serial();
                /* Okay, so the break won't work here, as it breaks the FOR loop, not the WHILE loop.
                * We might get away, with return, which might complicate something in the future though...
                * And then, there is is this... https://en.cppreference.com/w/cpp/language/break#Explanation
                * AKA "use goto". But I'd be crucified for this, not that much people will read this :D
                */
                return;
              }
            }                                             //IF we got through the FOR cycle, means the serial input matches the KEY!
            
            for(byte memSlot = 0x00; memSlot < 0x10; memSlot++) { //FOR every slot, one at a time...
              for(int x = 0; x < 16; x++) {
                EEPROM.write(x + (memSlot << 5), 0x00);     //CLEAR one.
              }
            }

            S.println(F("[SUCCESS] All memory slots in EEPROM has been cleared!"));
            dump_all_mem_slots_to_serial();
            S.println();
            break;
          }
          if (set_active_mem_slot(ch)) {    //If the active memory slot change was successfull...
            S.print(F("[SUCCESS] The currently active memory slot was changed to: "));
            S.println(activeMemSlot, HEX);
            break;
          } 
          else {                          //But if it wasn't...
            S.print(F("[ERROR] Invalid input! ")); S.print(F("( ")); 
            if ((ch == 13) || (ch == 10) || (ch == 8)) {  S.print(F("RETURN")); } else {  S.print(ch);  } //IF LF, CR or BackSpace...
            S.println(F(" )"));

            clear_serial();
            S.println();
            break;
          }
        }
        S.println();
        break;
      
      default:
        break;
  }
  serial_choice = -1; //It was previously at the end of every switch statement, so, let's try doing it always, anyways. Less code duplicity.
}

void setup() {
  #if USE_SERIAL == true
  Serial.begin(115200);
  #endif

  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(READ, INPUT_PULLUP);
  pinMode(WRITE, INPUT_PULLUP); //internal pullup

  for(int x = 0; x < 4; x++) {
    pinMode(SLOT[x], INPUT_PULLUP);
  }

  digitalWrite(RED, LOW);
  digitalWrite(GREEN, LOW); //off

  delay(150);   //Wait a bit, so we won't start printing menu too soon
  printMenu();  //print serial console welcome message
}

void loop(){
  if (Serial.available() > 0) {       //if there are data in the serial buffer...
    serial_parser();                  //Looks up, per 1 character, if the serial input is a command, otherwise prints input back

    // clear_serial();                   //Clear serial, but this will prevent batch execution of commands, so it's disabled
    function_caller();
    
    if (!Serial.available()) printMenu();
  }

  else {                                        //Serial buffer is empty (no incoming serial data present)
    read_pressed = !digitalRead(READ);
    write_pressed = !digitalRead(WRITE);
  
    if(read_pressed && write_pressed) {         //IF both buttons are pressed & held down, clear current mem position 
      for(int x = 0; x < 3; x++) {
        blinkPin(RED, 1, 500);
        read_pressed = !digitalRead(READ);
        write_pressed = !digitalRead(WRITE);
        if(!read_pressed || !write_pressed) {
          return;
        }
      }
      for(int i = 0; i < 16; i++) {
        EEPROM.write(i + (activeMemSlot << 5), 0x00); //Clear ALL 16 memory slots by writing zeroes to them
      }
      blinkPin(GREEN, 3, 150);
    }

    if(read_pressed) {                      //Read button was pressed
      read_iButton();
    }

    else if(write_pressed) {                //Write button was pressed
      write_iButton();
    }
    else {
      delay(1);
    }
  }
} 
