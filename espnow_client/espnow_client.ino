// ESP8266 Version
#include "ESP8266WiFi.h"
#include <espnow.h>

// Set the SERVER MAC Address
uint8_t slaveAddress[] = {0xC4, 0x5B, 0xBE, 0x6C, 0xAF, 0xA6};

// Structure to keep values
typedef struct struct_message {
  float temperature;
  float humidity;
};

// Create a struct_message called myData
struct_message myData;

// Callback to have a track of sent messages
void OnSent(uint8_t *mac_addr, uint8_t status) {
  Serial.print("\r\nSend message status:\t");
  Serial.println(status == 0 ? "Sent Successfully" : "Sent Failed");
}
 
void setup() {
  // Init Serial Monitor
  Serial.begin(115200);
 
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("There was an error initializing ESP-NOW");
    return;
  }
  
  // We will register the callback function to respond to the event
  esp_now_register_send_cb(OnSent);
  
  
  // Add server
  if (esp_now_add_peer(slaveAddress, ESP_NOW_ROLE_SLAVE, 0, NULL, 0) != 0){
    Serial.println("There was an error registering the slave");
    return;
  }
}

void loop() {
  // Set values to send
  // To simplify the code, we will just set two floats and I'll send it 
  myData.temperature = 12.5;
  myData.humidity = 58.9;
  
  // Is time to send the messsage via ESP-NOW
  uint8_t result = esp_now_send(slaveAddress, (uint8_t *) &myData, sizeof(myData));
   
  if (result == 0) {
    Serial.println("The message was sent sucessfully.");
  }
  else {
    Serial.println("There was an error sending the message.");
  }
  delay(2000);
}
