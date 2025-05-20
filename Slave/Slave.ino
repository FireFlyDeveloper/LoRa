#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LoRa Pin Configuration for NodeMCU-ESP32 (38-pin)
#define LORA_NSS  5   // GPIO5  - Chip Select (CS)
#define LORA_RST  14  // GPIO14 - Reset
#define LORA_DIO0 26   // GPIO2  - Interrupt
#define LORA_MOSI 23  // GPIO23 - MOSI
#define LORA_MISO 19  // GPIO19 - MISO
#define LORA_SCK  18  // GPIO18 - SCK

// LCD Configuration (I2C 20x4)
LiquidCrystal_I2C lcd(0x27, 20, 4);  // Address 0x27, 20 columns, 4 rows

// Alarm & Lights Pin Configuration
#define HORN_PIN   4   // GPIO4 - Horn/Buzzer
#define GREEN_PIN  15  // GPIO15 - Green LED (Low water level)
#define YELLOW_PIN 13  // GPIO13 - Yellow LED (Medium water level)
#define RED_PIN    12  // GPIO12 - Red LED (High water level)

// LoRa Addresses
byte LocalAddress = 0x02;       // Slave address
byte Destination_Master = 0x01; // Master address

String Incoming = "";
unsigned long buzzerStartTime = 0; // Track when the buzzer was activated
const unsigned long buzzerDuration = 180000; // 3 minutes in milliseconds (3 * 60 * 1000)

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Initialize LCD
  Wire.begin(21, 22); // SDA=GPIO21, SCL=GPIO22 (standard I2C pins for ESP32)
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LoRa Slave Ready");
  lcd.setCursor(0, 1);
  lcd.print("Waiting for data...");

  // Configure SPI with custom pins
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);

  // Initialize LoRa with custom pins
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
  
  Serial.println("Initializing LoRa...");
  if (!LoRa.begin(433E6)) {  // Use 915E6 for 915MHz
    Serial.println("LoRa init failed!");
    lcd.setCursor(0, 2);
    lcd.print("LoRa Init FAILED!");
    while (true);
  }
  Serial.println("LoRa initialized");
  lcd.setCursor(0, 2);
  lcd.print("LoRa Initialized");

  // Set up Alarm & Lights as OUTPUT
  pinMode(HORN_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);

  // Ensure all outputs are OFF initially
  digitalWrite(HORN_PIN, HIGH);
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(YELLOW_PIN, HIGH);
  digitalWrite(RED_PIN, HIGH);
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    onReceive(packetSize);
  }

  // Check if buzzer has been on for more than 3 minutes
  if (digitalRead(HORN_PIN)) {
    if (millis() - buzzerStartTime >= buzzerDuration) {
      digitalWrite(HORN_PIN, HIGH); // Turn off buzzer
    }
  }
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;

  // Read packet header
  int recipient = LoRa.read();
  byte sender = LoRa.read();
  byte incomingLength = LoRa.read();

  Incoming = "";
  while (LoRa.available()) {
    Incoming += (char)LoRa.read();
  }

  if (incomingLength != Incoming.length()) {
    Serial.println("Error: Message length mismatch");
    return;
  }

  if (recipient != LocalAddress) {
    Serial.println("This message is not for me.");
    return;
  }

  // Parse received data
  int data = Incoming.toInt();
  String waterLevel = "??%";
  String warning = "invalid";

  // Reset all outputs first
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(YELLOW_PIN, HIGH);
  digitalWrite(RED_PIN, HIGH);

  switch(data) {
    case 1: // 33% - Low level (Green light)
      waterLevel = "33%";
      warning = "low";
      digitalWrite(GREEN_PIN, LOW);
      break;
    case 2: // 66% - Medium level (Yellow light)
      waterLevel = "66%";
      warning = "medium";
      digitalWrite(YELLOW_PIN, LOW);
      break;
    case 3: // 99% - High level (Red light + Horn alarm)
      waterLevel = "99%";
      warning = "dangerous";
      digitalWrite(RED_PIN, LOW);
      digitalWrite(HORN_PIN, LOW); // Activate horn
      buzzerStartTime = millis();   // Record activation time
      break;
  }

  // Update LCD display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Water Level: " + waterLevel);
  lcd.setCursor(0, 1);
  lcd.print("Warning: " + warning);

  // Optional: Print debug info to Serial
  Serial.print("\nReceived: ");
  Serial.println(data);
  Serial.print("Water Level: ");
  Serial.println(waterLevel);
  Serial.print("Warning: ");
  Serial.println(warning);
}

void sendMessage(String Outgoing, byte Destination) {
  LoRa.beginPacket();
  LoRa.write(Destination);
  LoRa.write(LocalAddress);
  LoRa.write(Outgoing.length());
  LoRa.print(Outgoing);
  LoRa.endPacket();
}