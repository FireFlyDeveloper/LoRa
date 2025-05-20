#include <SPI.h>
#include <LoRa.h>

// LoRa Pin Configuration
#define LORA_NSS  5   // GPIO5  - Chip Select (CS)
#define LORA_RST  14  // GPIO14 - Reset
#define LORA_DIO0 2   // GPIO2  - Interrupt
#define LORA_MOSI 23  // GPIO23 - MOSI
#define LORA_MISO 19  // GPIO19 - MISO
#define LORA_SCK  18  // GPIO18 - SCK

// Switch Pins (Active-LOW: GND = OFF, OPEN = ON)
#define SWITCH_33  27   // GPIO27 - 33% Water Level
#define SWITCH_66  33   // GPIO33 - 66% Water Level
#define SWITCH_99  34   // GPIO34 - 99% Water Level

// LoRa addresses
byte LocalAddress = 0x01;       // Master address
byte Destination_Slave = 0x02;  // Slave address

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
  pinMode(SWITCH_33, INPUT);  // No pullup
  pinMode(SWITCH_66, INPUT);
  pinMode(SWITCH_99, INPUT);
}

void loop() {
  bool is33 = (digitalRead(SWITCH_33) == HIGH);
  bool is66 = (digitalRead(SWITCH_66) == HIGH);
  bool is99 = (digitalRead(SWITCH_99) == HIGH);

  Serial.println(is33);
  Serial.println(is66);
  Serial.println(is99);

  // Determine current state
  int currentState = 1;  // Default: invalid

  // **No connection (all switches open) → 99%**
  if (is33 && is66 && is99) {
    currentState = 1;  // 99% water level
  }
  // **Only 33% connected → 33%**
  else if (!is33 && is66 && is99) {
    currentState = 1;  // 33% water level
  }
  // **33% + 66% connected → 66%**
  else if (!is33 && !is66 && is99) {
    currentState = 2;  // 66% water level
  }
  // **All switches connected → 99%**
  else if (!is33 && !is66 && !is99) {
    currentState = 3;  // 99% water level
  }

  String message = String(currentState);
  Serial.println("Sending: " + message);
  sendMessage(message, Destination_Slave);

  delay(10000);
}