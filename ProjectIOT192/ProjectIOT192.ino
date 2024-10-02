// Group 3 - SE1805

/* pins:
 + RFID:
  -13: SCK for RFID      
  -12: MISO for RFID
  -11: MOSI for RFID      
  -10: SS(DSA) for RFID  
  -9: RST for RFID      
 + Servo:
  -5: Servo terminal
 */

//libraries

#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>
#include <IRremote.hpp>


//Define pins

#define RECV_PIN 7
#define SS_PIN 10
#define RST_PIN 9
#define SERVO_PIN 5
#define LED_PIN 8

//Create objects for servo, lcd, and card reader

MFRC522 mfrc522(SS_PIN, RST_PIN);    // Tạo đối tượng cho đọc thẻ RFID
Servo servo;                         // Tạo đối tượng cho Servo
LiquidCrystal_I2C lcd(0x27, 16, 2);  // I2C address 0x27, 16 column and 2 rows

int availableSlots = 1;   // Initial free slots
int maxSlot = 3;          // Maximum free slots
String parkedRFIDs[3];    // Store the ID of parked cars

// Store valid IDs
String validRFIDs[] = { "73c9de34", "53412914" };  // Add more IDs if you want 

void setup() {
  Serial.begin(9600);
  SPI.begin();         // Create SPI bus
  mfrc522.PCD_Init();  // Create RFID reader

  servo.attach(SERVO_PIN); // Add servo
  servo.write(0);  // Set initial state to close

  pinMode(LED_PIN, OUTPUT); // Add LED pins
  IrReceiver.begin(RECV_PIN); // Add IR receiver

  lcd.init();  // Initialize the lcd
  lcd.backlight();

  lcd.print("Smart Parking"); // Print to LCD
  Serial.println("Smart Parking");  // Print to Serial Monitor
  lcd.setCursor(0, 1); // Set cursor
  lcd.print("Available: " + String(availableSlots)); // Print available slots
  Serial.println("Available: " + String(availableSlots));  // Print to Serial Monitor
}

void loop() {
  // If receive signal from IR
  if (IrReceiver.decode()) {
    // Decode the command code
    int command = IrReceiver.decodedIRData.command;
    // Print to Serial for debugs
    Serial.println(command);
    // If remote button is 1 and there are still available slots
    if (command == 12 && availableSlots > 0) {
      // Let the cars go in and print some information to the LCD
      lcd.clear();
      goIn();
    } 
    // If the remote button is 2 and there are still cars in the parking
    else if (command == 24 && availableSlots < maxSlot) {
      // Let the cars go out and print some information to the LCD
      lcd.clear();
      goOut();
    }
    // Resume the remote
    IrReceiver.resume();
    // Reset LCD screen
    resetDisplay();
  }
  // If a RFID card is detected
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String rfid = readRFID();  // Read the ID

    if (isValidRFID(rfid))  // check validity
    { 
      // Print to LCD
      lcd.clear();
      lcd.print("RFID: " + rfid);
      Serial.println("RFID: " + rfid);  // Print to Serial Monitor

      // Check if this car is already parked
      bool isParkedCar = false;
      for (int i = 0; i < 3; i++) {
        if (rfid == parkedRFIDs[i]) {
          isParkedCar = true;
          break;
        }
      }

      // If it is parked (i.e. go out from inside)
      if (isParkedCar) {
        // Open the entrance and display good bye message
        goOut();
        // Remove ID from parked list
        removeParkedRFID(rfid);
      }
      // If it is from outside and there are still free slots 
      else if (availableSlots > 0) {
        // Open the entrance and display welcome message
        goIn();
        // Add ID to list of parked car
        addParkedRFID(rfid);
      } 
      // If all slots are filled
      else {
        // Print full slot message
        lcd.setCursor(3, 1);
        lcd.print("Full Slot");
        Serial.println("Full Slot");  // Print to Serial Monitor
        // Blink the LED
        for (int i = 0; i < 5; i++) {
          digitalWrite(LED_PIN, HIGH);
          delay(250);
          digitalWrite(LED_PIN, LOW);
          delay(250);
        }
      }
    }
    // If the ID is not valid 
    else {
      // Print invalid messgae
      lcd.clear();
      lcd.print("Invalid RFID!");
      Serial.println("Invalid RFID!");  // Print to Serial Monitor
      // Print ID to Serial for debug
      Serial.println(rfid);
      // Blink the LED
      for (int i = 0; i < 5; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(250);
        digitalWrite(LED_PIN, LOW);
        delay(250);
      }
    }
    // Reset the lcd
    resetDisplay();
  }
}

// Read the ID
String readRFID() {
  // Initialize empty string
  String rfid = "";
  // Read and concat each byte of the ID
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    rfid.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
    rfid.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  // Return the ID
  return rfid;
}

// Check ID validity
bool isValidRFID(String rfid)  // Hàm kiểm tra xem mã thẻ có hợp lệ không
{
  // Compare each string, if match return string
  for (int i = 0; i < sizeof(validRFIDs) / sizeof(validRFIDs[0]); i++) {
    if (rfid == validRFIDs[i]) {
      return true;
    }
  }
  // Else false
  return false;
}

// Add ID to parked array
void addParkedRFID(String rfid) {
  for (int i = 0; i < 3; i++) {
    // find a free slot, if availbale then add the ID
    if (parkedRFIDs[i] == "") {
      parkedRFIDs[i] = rfid;
      break;
    }
  }
}

// Remove ID from parked array
void removeParkedRFID(String rfid) {
  // If a match ID is found, turn it to blank string
  for (int i = 0; i < 3; i++) {
    if (parkedRFIDs[i] == rfid) {
      parkedRFIDs[i] = "";
      break;
    }
  }
}

// Open and close the entrace
void openServo() {
  servo.write(90);  // Turn the servo up
  digitalWrite(LED_PIN, HIGH); // Start the LED
  delay(5000); // Wait 5 seconds
  digitalWrite(LED_PIN, LOW); // Stop the LED
  servo.write(0);  // Turn the servo down
}

// When the user go out of the parking
void goOut() {
  // Display good bye message
  lcd.clear();
  lcd.print("See You Again!");
  Serial.println("See You Again!");  // Print to Serial Monitor
  lcd.setCursor(1, 1);
  // open the entrance
  openServo();
}

void goIn() {
  Serial.println("Welcome!");  // Print to Serial Monitor
  // Display welcome message
  lcd.setCursor(4, 1);
  lcd.print("Welcome!");
  // open the entrance
  openServo();
}

// Reset display
void resetDisplay() {
  // Print to lcd
  lcd.clear();
  lcd.print("Smart Parking");
  Serial.println("Smart Parking");  // Print to Serial Monitor
  lcd.setCursor(0, 1);
  // Print available slots
  lcd.print("Available: " + String(availableSlots));
  Serial.println("Available: " + String(availableSlots));  // Print to Serial Monitor
}
