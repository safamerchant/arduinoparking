//The libraries I have used within the code:
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>  // for the lcd screen
#include <TimeLib.h>                //for the time display
#include <utility/Adafruit_MCP23017.h>
#include <MemoryFree.h>  // for the freeSRAM display
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
//defining the colours of the lcd backlight:
#define YELLOW 0x3  //when NPD
#define GREEN 0x2   //when PD
#define VIOLET 0x5  //in Sync state
#define WHITE 0x7
// the global variables use:
unsigned long lastDisplayTime;
int i = 0;
int count = 0;
int n = 0;       // array index
int scroll = 0;  // the scrolling index
//The 2 states defined:
#define Sync 0
#define Main 1
//Defining the makeup of my up and down arrows (custom character):
uint8_t upArrow[] = {
  0b00100,
  0b01110,
  0b10101,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00000
};
uint8_t downArrow[] = {
  0b00000,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b10101,
  0b01110,
  0b00100
};

//Defining Struct to store vehicle data below:
struct parkingData {
  String vehicleReg;
  String location;
  char vehicleType;
  String payStatus;
  String enterTime;
  String exitTime;
};
parkingData parking[5];  //Stating parking as an array with 5 variables

//function for the display of data on lcd screen:
void lcdDisplay() {
  if (n > 0) {
    lcd.createChar(0, upArrow);
    lcd.createChar(1, downArrow);
    for (i = 0; i < 5; i++) {
      if (n > 1 && scroll != 0) { 
        lcd.setCursor(0, 0);
        lcd.write((uint8_t)0);
      }// if 1+ elements in array and scroll not 0 then up arrow displayed in lcd screen
      if (n > 0 && scroll != n - 1) {
        lcd.setCursor(0, 1);
        lcd.write((uint8_t)1);
      }/*prevents the down arrow being displayed if only one element in array 
      or if the last element is being displayed on the lcd screen*/
      lcd.setCursor(1, 0);
      lcd.print(parking[scroll].vehicleReg);
      lcd.print(" ");
      lcd.print(parking[scroll].location);
      lcd.setCursor(1, 1);
      lcd.print(parking[scroll].vehicleType);
      lcd.print(" ");
      lcd.print(parking[scroll].payStatus);
      lcd.print(" ");
      lcd.print(parking[scroll].enterTime);
      lcd.print(" ");
      lcd.print(parking[scroll].exitTime);
      if (parking[scroll].payStatus == "NPD") {
        lcd.setBacklight(YELLOW);
      }
      if (parking[scroll].payStatus == "PD") {
        lcd.setBacklight(GREEN);
      }
    }
  }
}

String displayTime() {
  String newTime = printDigits(hour());
  newTime += printDigits(minute());
  return newTime;
}// function to return current time in HHMM format

String printDigits(int timeAsInt) {
  // prints leading 0 if number is less than 10
  String printedTime = (timeAsInt < 10) ? "0" + String(timeAsInt) : String(timeAsInt);
  return printedTime;
}

void setup() {
  setTime(0, 0, 0, 1, 1, 2023);//sets time as 000 when code begins
  Serial.begin(9600);
  lcd.begin(16, 2);
  lastDisplayTime = millis();
  lcd.setBacklight(VIOLET);
}

void loop() {
  static int state = Sync;
  switch (state) {
    case Sync:
      {
        Serial.print("Q");
        delay(1000);
        if (Serial.available() > 0) {
          String input = Serial.readString();
          input.trim();
          if (input == "X") {
            lcd.setBacklight(WHITE);
            Serial.println("UDCHARS,FREERAM");// based on extensions implemented
            state = Main;
          } else {
            Serial.println("ERROR: Invalid Input");
          }
        }
      }
      break;
    case Main:
      {
        uint8_t buttons = lcd.readButtons();
        static unsigned long selectStart = 0;
        static bool select_pressed = false;
        
        if (buttons & BUTTON_SELECT) {
          if (selectStart == 0) {
            selectStart = millis();
          }
          if (millis() - selectStart > 1000) {
            if (!select_pressed) {
              lcd.setBacklight(VIOLET);
              lcd.setCursor(1, 0);
              lcd.clear();
              lcd.print("F330446");
              lcd.setCursor(0, 1);
              lcd.print(getFreeMemory());//function to print free memory
              lcd.print("bytes");
              select_pressed = true;
              selectStart == 0;
            }
          }
        } else {
          if (select_pressed) { //when select released
            lcd.clear();
            lcd.setBacklight(WHITE);
            lcdDisplay();
            select_pressed = false;
            selectStart = 0;
          }
        }
        if (buttons & BUTTON_UP) {
          if (scroll > 0) {
            lcd.clear();
            scroll--;
          }// decrements scroll to show next element in array
        }
        if (buttons & BUTTON_DOWN) {
          if (scroll < n - 1) {
            lcd.clear();
            scroll++;
          }// increments scroll to show previous element in array
        }
        {
          lcdDisplay();
        }
        String input = Serial.readString();
        String reg = input.substring(2, 9);
        input.trim();
        if (input.length() > 0){// checks if first character is valid
          if(input[0]== 'A' or input[0]== 'S' or input[0]== 'L' or input[0]== 'T' or input[0]== 'R' ){
        // Protocol A:
        // FIX THE REPEATING 0 ISSUE IF COMMENT OUT SERIAL PRINT LN THEN ENTERTIME DISAPPEARS SOME DIGITS
        bool exists = false;
        if (input.charAt(0) == 'A') {
          if (input.charAt(1) == '-') {
            //Serial.print("...");
            if (input.substring(2, 9).length() == 7) {
              if (input[9] == '-' && input[11] == '-') {
                if (input[10] == 'C' || input[10] == 'M' || input[10] == 'V' || input[10] == 'L' || input[10] == 'B') {
                  //Serial.println("...");
                  if (input[11] == '-') {
                    if (input.substring(12, input.length()).length() >= 1 && (input.substring(12, input.length()).length() <= 11)) {
                      for (int i = 0; i < 5; i++) {
                        //Serial.println("DEBUG: Enters the for loop");
                        if (parking[i].vehicleReg.equals(reg) && parking[i].vehicleType == (input[10])) {
                          Serial.println("ERROR: This vehicle already exists");
                          exists = true;
                          break;
                        }
                      }
                      if (exists == false) {
                        Serial.println("DEBUG: Vehicle is different");
                        parking[n].vehicleReg = reg;
                        parking[n].vehicleType = input[10];
                        parking[n].location = input.substring(12, input.length());
                        parking[n].payStatus = "NPD";
                        parking[n].enterTime = displayTime();
                        parking[n].exitTime = "";
                        Serial.println("DONE!");
                        n++;
                      }
                    } else {
                      Serial.println("ERROR: Invalid input");
                    }
                  }
                }
              } else {
                Serial.print("ERROR: Invalid input");
              }
            }
          } else {
            Serial.print("ERROR: Invalid input!");
          }
        }
        // Protocol S:
        int count = -1;
        String enteredStatus = input.substring(10, input.length());
        if (input.charAt(0) == 'S' && input.charAt(1) == '-' && reg.length() == 7 && input.charAt(9) == '-') {
          if (enteredStatus == "NPD" || enteredStatus == "PD") {
            Serial.println("DEBUG: Valid input of S");
            // Initialize count outside the loop
            int count = -1;
            for (int i = 0; i < 5; i++) {
              if (parking[i].vehicleReg == reg) {
                count = i;  // Update count to the found index
                Serial.print("DEBUG: Vehicle found in database");
                if (parking[i].payStatus != enteredStatus) {
                  if (parking[i].payStatus == "NPD" && enteredStatus == "PD") {
                    parking[i].payStatus = "PD";
                    parking[i].exitTime = displayTime();
                    Serial.println("DONE!");
                  }
                  if (parking[i].payStatus == "PD" && enteredStatus == "NPD") {
                    parking[i].payStatus = "NPD";
                    parking[i].exitTime = "";
                    parking[i].enterTime = displayTime();
                    Serial.println("DONE!");
                  }
                } else {
                  Serial.println("Payment status is same");
                }
              }
            }
            // If count remains -1, the vehicle was not found
            if (count == -1) {
              Serial.println("Vehicle not found in database");
            }
          } else {
            Serial.print("ERROR: Invalid input");
          }
        }
        //Protocol T:
        if (input[0] == 'T') {
          if (input[1] == '-') {
            if (reg.length() == 7 && input[9] == '-') {
              Serial.print("DEBUG: Valid input for function T");
              for (int i = 0; i < 5; i++) {
                if (parking[i].vehicleReg == reg) {
                  char storedType = parking[i].vehicleType;
                  char enteredType = input[10];
                  if (parking[i].payStatus != "NPD") {
                    if (parking[i].vehicleType != input[10]) {
                      if (enteredType == 'C') {
                        parking[i].vehicleType = 'C';
                        Serial.print("DONE!");
                      }
                      if (enteredType == 'V') {
                        parking[i].vehicleType = 'V';
                        Serial.print("DONE!");
                      }
                      if (enteredType == 'B') {
                        parking[i].vehicleType = 'B';
                        Serial.print("DONE!");
                      }
                      if (enteredType == 'M') {
                        parking[i].vehicleType = 'M';
                        Serial.print("DONE!");
                      }
                      if (enteredType == 'L') {
                        parking[i].vehicleType = 'L';
                        Serial.print("DONE!");
                      }
                      break;
                    } else {
                      Serial.print("ERROR: Vehicle already stored as this type");
                      break;
                    }
                  } else {
                    Serial.print("ERROR: Vehicle not paid for so cannot modify");
                    break;
                  }
                } else {
                  Serial.print("ERROR: No such vehicle found");
                  break;
                }
              }
            }
          }
        }
        //Protocol L:
        if (input[0] == 'L') {
          if (input[1] == '-') {
            if (reg.length() == 7 && input[9] == '-') {
              if (input.substring(10, input.length()).length() >= 1 && (input.substring(10, input.length()).length() <= 11)) {
                Serial.print("DEBUG: Valid input for function L");
                bool vehicleFound = false;  // Add a flag to track if the vehicle is found
                for (int i = 0; i < 5; i++) {
                  //Serial.print(parking[i].vehicleReg);
                  //Serial.println(reg);
                  if (parking[i].vehicleReg == reg) {
                    vehicleFound = true;  // Set the flag to true
                    if (parking[i].location != input.substring(10, input.length())) {
                      if (parking[i].payStatus == "PD") {
                        parking[i].location = input.substring(10, input.length());
                        Serial.println("DONE!");
                      } else {
                        Serial.println("ERROR: Payment has not been made so cannot be modified");
                      }
                    } else {
                      Serial.println("ERROR: Location is the same.");
                    }
                    // No need to break here; let the loop continue in case there are more entries
                  }
                }
                // Check if the vehicle was not found after the loop
                if (!vehicleFound) {
                  Serial.println("ERROR: Vehicle doesn't exist");
                }
              }
            }
          }
        }
        //Protocol R:
        if (input[0] == 'R') {
          if (input[1] == '-') {
            if (reg.length() == 7) {
              int exists = 0;  // Initialize with a value that indicates non-existence
              for (int i = 0; i < 5; i++) {
                if (parking[i].vehicleReg == reg) {
                  exists = 1;
                  if (parking[i].payStatus == "PD") {
                    // Mark the entry for discard
                    int discard = i;
                    for (int k = 0; k < 5 - i; ++k) {
                      if (k == discard) {
                        for (int p = k; p < 4; ++p) {
                          parking[p] = parking[p + 1];
                          //entry Discarded
                        }
                        n--;
                        lcd.clear();
                        lcd.setBacklight(WHITE);
                        Serial.println("DONE!");
                        break;
                      }
                    }
                    break;
                  } else {
                    Serial.println("ERROR: Payment not been made");
                    break;
                  }
                }
              }
              if (exists == 0) {
                Serial.println("ERROR: This vehicle is not present in database");
              }
            }
          }
        }
        break;
          } else{
            Serial.println("ERROR: Invalid input");//if not A/S/T/L/R as first character in user input
          }
        } 
      }
  }
}
