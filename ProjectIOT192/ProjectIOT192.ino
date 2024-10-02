// Group 3 - SE1805

/* Đấu nối:
 + RFID:
  -13: nối chân SCK của RFID      
  -12: nối chân MISO của RFID
  -11: nối chân MOSI của RFID      
  -10: nối chân SS(DSA)của RFID  
  -9: nối chân RST của RFID   
 + Loa BUZZER:
  -A0: nối chân Anot của loa   
 + Động cơ Servo:
  -5: nối chân tín hiệu của động cơ Servo
 */

//Khai báo thư viện

#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>
#include <IRremote.hpp>

const int RECV_PIN = 7;

//Khai báo các chân kết nối của Arduino với các linh kiện

#define SS_PIN 10
#define RST_PIN 9
// #define IN_BTN 7
// #define OUT_BTN 8
#define SERVO_PIN 5
#define BUZZER_PIN 8

//Khởi tạo các đối tượng (instances) cho các module

MFRC522 mfrc522(SS_PIN, RST_PIN);    // Tạo đối tượng cho đọc thẻ RFID
Servo servo;                         // Tạo đối tượng cho Servo
LiquidCrystal_I2C lcd(0x27, 16, 2);  // I2C address 0x27, 16 column and 2 rows

int availableSlots = 1;  // Số lượng chỗ đỗ trống ban đầu
int maxSlot = 3;
String parkedRFIDs[3];   // Mảng để lưu trữ thông tin về các thẻ RFID đã đỗ

// Mảng chứa các mã thẻ RFID hợp lệ
String validRFIDs[] = { "73c9de34", "53412914" };  // Thay đổi các mã thẻ tùy ý

// Thêm biến để theo dõi thời gian còi kêu
unsigned long buzzerStartTime = 0;
const unsigned long buzzerDuration = 5000;  // Thời gian kêu còi: 5 giây

void setup() {
  Serial.begin(9600);
  SPI.begin();         // Khởi tạo bus SPI
  mfrc522.PCD_Init();  // Khởi tạo đọc thẻ RFID

  servo.attach(SERVO_PIN);
  servo.write(0);  // Đóng Servo ban đầu

  pinMode(BUZZER_PIN, OUTPUT);
  IrReceiver.begin(RECV_PIN);

  lcd.init();  // initialize the lcd
  lcd.backlight();

  lcd.print("Smart Parking");
  Serial.println("Smart Parking");  // In ra Serial Monitor
  lcd.setCursor(0, 1);
  lcd.print("Available: " + String(availableSlots));
  Serial.println("Available: " + String(availableSlots));  // In ra Serial Monitor
}

void loop() {
  if (IrReceiver.decode()) {
    int command = IrReceiver.decodedIRData.command;
    Serial.println(command);
    if (command == 12 && availableSlots > 0) {
      lcd.clear();
      goIn();
    } else if (command == 24 && availableSlots < maxSlot) {
      lcd.clear();
      goOut();
    }
    IrReceiver.resume();
    resetDisplay();
  }
  // Kiểm tra nếu có thẻ RFID mới được đưa vào
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String rfid = readRFID();  // Đọc mã RFID từ thẻ

    if (isValidRFID(rfid))  // Kiểm tra xem mã thẻ có hợp lệ không
    {
      lcd.clear();
      lcd.print("RFID: " + rfid);
      Serial.println("RFID: " + rfid);  // In ra Serial Monitor

      bool isParkedCar = false;
      for (int i = 0; i < 3; i++) {
        if (rfid == parkedRFIDs[i]) {
          isParkedCar = true;
          break;
        }
      }

      if (isParkedCar) {
        goOut();
        removeParkedRFID(rfid);
      } else if (availableSlots > 0) {
        goIn();
        addParkedRFID(rfid);
      } else {
        lcd.setCursor(3, 1);
        lcd.print("Full Slot");
        Serial.println("Full Slot");  // In ra Serial Monitor
        for (int i = 0; i < 5; i++) {
          digitalWrite(BUZZER_PIN, HIGH);
          delay(250);
          digitalWrite(BUZZER_PIN, LOW);
          delay(250);
        }
      }
    } else {
      lcd.clear();
      lcd.print("Invalid RFID!");
      Serial.println("Invalid RFID!");  // In ra Serial Monitor
      Serial.println(rfid);
      for (int i = 0; i < 5; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(250);
        digitalWrite(BUZZER_PIN, LOW);
        delay(250);
      }
    }
    resetDisplay();

    // if (isBuzzerOn()) {
    //   if (millis() - buzzerStartTime >= buzzerDuration) {
    //     stopBuzzer();
    //   }
    // }
    mfrc522.PICC_HaltA();  // Dừng truyền thẻ RFID
  }
}

String readRFID() {
  String rfid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    rfid.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
    rfid.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  mfrc522.PICC_HaltA();
  return rfid;
}

bool isValidRFID(String rfid)  // Hàm kiểm tra xem mã thẻ có hợp lệ không
{
  for (int i = 0; i < sizeof(validRFIDs) / sizeof(validRFIDs[0]); i++) {
    if (rfid == validRFIDs[i]) {
      return true;
    }
  }
  return false;
}

void addParkedRFID(String rfid) {
  for (int i = 0; i < 3; i++) {
    if (parkedRFIDs[i] == "") {
      parkedRFIDs[i] = rfid;
      break;
    }
  }
}

void removeParkedRFID(String rfid) {
  for (int i = 0; i < 3; i++) {
    if (parkedRFIDs[i] == rfid) {
      parkedRFIDs[i] = "";
      break;
    }
  }
}

void buzzBuzzer() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(3000);
  digitalWrite(BUZZER_PIN, LOW);
}

void openServo() {
  servo.write(90);  // Open the servo
  digitalWrite(BUZZER_PIN, HIGH);
  delay(5000);
  digitalWrite(BUZZER_PIN, LOW);
  servo.write(0);  // Close the servo
}

void goOut() {
  lcd.print("See You Again!");
  Serial.println("See You Again!");  // In ra Serial Monitor
  lcd.setCursor(1, 1);
  openServo();
  lcd.clear();
  lcd.print("Available: " + String(++availableSlots));
  Serial.println("Available: " + String(availableSlots));  // In ra Serial Monitor
}

void goIn() {
  Serial.println("Welcome!");  // In ra Serial Monitor
  lcd.setCursor(4, 1);
  lcd.print("Welcome!");
  openServo();
  lcd.clear();
  lcd.print("Available: " + String(--availableSlots));
  Serial.println("Available: " + String(availableSlots));  // In ra Serial Monitor
}

void resetDisplay() {
  lcd.clear();
  lcd.print("Smart Parking");
  Serial.println("Smart Parking");  // In ra Serial Monitor
  lcd.setCursor(0, 1);
  lcd.print("Available: " + String(availableSlots));
  Serial.println("Available: " + String(availableSlots));  // In ra Serial Monitor
}

// void startBuzzer() {
//   digitalWrite(BUZZER_PIN, HIGH);
//   buzzerStartTime = millis();
// }

// void stopBuzzer() {
//   digitalWrite(BUZZER_PIN, LOW);
//   buzzerStartTime = 0;
// }

// bool isBuzzerOn() {
//   return buzzerStartTime > 0;
// }
