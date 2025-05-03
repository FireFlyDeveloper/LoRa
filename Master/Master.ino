#include <SPI.h>
#include <LoRa.h>

// LoRa Pin Configuration
#define LORA_NSS  21   // GPIO21 - Chip Select
#define LORA_RST  5    // GPIO5  - Reset
#define LORA_DIO0 6    // GPIO6  - Interrupt
#define LORA_MOSI 0    // GPIO0  - MOSI
#define LORA_MISO 1    // GPIO1  - MISO
#define LORA_SCK  20   // GPIO20 - SCK

// Switch Pins (Active-LOW: GND = OFF, OPEN = ON)
#define SWITCH_33  2   // GPIO2 - 33% Water Level
#define SWITCH_66  3   // GPIO3 - 66% Water Level
#define SWITCH_99  4   // GPIO4 - 99% Water Level

// LoRa addresses
byte LocalAddress = 0x01;       // Master address
byte Destination_Slave = 0x02;  // Slave address

// Timer for sending messages
unsigned long previousMillis = 0;
const long interval = 2000;  // Send every 2 seconds

void sendMessage(String Outgoing, byte Destination) {
  LoRa.beginPacket();
  LoRa.write(Destination);
  LoRa.write(LocalAddress);
  LoRa.write(Outgoing.length());
  LoRa.print(Outgoing);
  LoRa.endPacket();
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Configure SPI with custom pins
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  
  // Initialize LoRa with custom pins
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);

  Serial.println("Initializing LoRa Master...");
  if (!LoRa.begin(433E6)) {  // Use 915E6 for 915MHz
    Serial.println("LoRa init failed!");
    while (true);
  }
  Serial.println("LoRa Master initialized");

  // Set switch pins as INPUT_PULLUP (GND-triggered)
  pinMode(SWITCH_33, INPUT_PULLUP);
  pinMode(SWITCH_66, INPUT_PULLUP);
  pinMode(SWITCH_99, INPUT_PULLUP);
}

void loop() {
  // Read switches
  int switch33 = digitalRead(SWITCH_33);
  int switch66 = digitalRead(SWITCH_66);
  int switch99 = digitalRead(SWITCH_99);

  // Determine current state (sequential activation)
  int currentState = 0;  // Default: invalid
  if (switch33 == HIGH && switch66 == LOW && switch99 == LOW) {
    currentState = 1;  // Only 33% pressed
  } 
  else if (switch33 == HIGH && switch66 == HIGH && switch99 == LOW) {
    currentState = 2;  // 33% + 66% pressed
  }
  else if (switch33 == HIGH && switch66 == HIGH && switch99 == HIGH) {
    currentState = 3;  // All switches pressed
  }

  // Send current state every 2 seconds (regardless of changes)
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    // Only send valid states (1, 2, or 3)
    if (currentState >= 1 && currentState <= 3) {
      String message = String(currentState);
      Serial.println("Sending: " + message);
      sendMessage(message, Destination_Slave);
    }
  }
}