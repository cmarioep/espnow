#include <ESP8266WiFi.h>
#include <espnow.h>


// ***********************************************
#define digitalSwitch D0

bool currentStatusSwitch;
bool deviceState;
unsigned long previousMillis = 0;
const long interval = 500;
// ***********************************************



typedef struct esp_now_peer_info {
  u8 peer_addr[6];
  uint8_t channel;
  uint8_t encrypt;
}esp_now_peer_info_t;

// Global copy of slave
esp_now_peer_info_t slaveInfo;
bool isServerOnline;

#define CHANNEL 3
#define PRINTSCANRESULTS 0

// Init ESP Now with fallback
void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == 0) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
}

// Scan for slaves in AP mode
void ScanForSlave() {
  int8_t scanResults = WiFi.scanNetworks();
  //reset slaves
  memset(&slaveInfo, 0, sizeof(slaveInfo));
  isServerOnline = false;
  Serial.println("");
  
  if (scanResults == 0) {
    Serial.println("No WiFi devices in AP Mode found");
  } else {
    Serial.print("No WiFi devices found"); Serial.print(scanResults);
    
    for (int i = 0; i < scanResults; ++i) {
      String SSID = WiFi.SSID(i);
      int32_t RSSI = WiFi.RSSI(i);
      String BSSIDstr = WiFi.BSSIDstr(i);

      delay(10);
      
      // Check if the current device starts with `Slave`
      if (SSID.indexOf("Slave") == 0) {
        // SSID of interest
        Serial.print(i + 1); Serial.print(": "); Serial.print(SSID); Serial.print(" ["); Serial.print(BSSIDstr); Serial.print("]"); Serial.print(" ("); Serial.print(RSSI); Serial.print(")"); Serial.println("");
        // Get BSSID => Mac Address of the receiver
        int mac[6];

        if ( 6 == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x",  &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5] ) ) {
            memcpy(slaveInfo.peer_addr, &mac, sizeof(mac));            
          }
        }
        
        slaveInfo.channel = CHANNEL; // pick a channel
        slaveInfo.encrypt = 0; // no encryption
        isServerOnline = true;
        
      }
    }
    
    if (isServerOnline) {
    Serial.println("Receiver found, processing..");
  } else {
    Serial.println("No receiver Found, trying again.");
  }

  // clean up ram
  WiFi.scanDelete();
  }




// Check if the slave is already paired with the master.
// If not, pair the slave with master
void manageSlave() {
  if (isServerOnline) {

      const esp_now_peer_info_t *peer = &slaveInfo;
      u8 *peer_addr = slaveInfo.peer_addr;
      
      Serial.print("Processing: ");

      Serial.print(" Status: ");
      // check if the peer exists
      bool exists = esp_now_is_peer_exist((u8*)peer_addr);
      if (exists) {
        // Slave already paired.
        Serial.println("Already Paired");
      } else {
        // Slave not paired, attempt pair
        int addStatus = esp_now_add_peer((u8*)peer_addr, ESP_NOW_ROLE_CONTROLLER, CHANNEL, NULL, 0);
        if (addStatus == 0) {
          // Pair success
          Serial.println("Pair success");
        } else {
          Serial.println("Pair failed");
        }
        delay(100);
      }

  } else {
    // No slave found to process
    Serial.println("No receiver found to process");
  }
}

// ***********************************************
void sendData() {
    u8 *peer_addr = slaveInfo.peer_addr;

    Serial.print("Sending: ");
    Serial.println(deviceState);

    int result = esp_now_send(peer_addr, (u8*)&deviceState, sizeof(deviceState));
    Serial.print("Send Status: ");
    if (result == 0) {
      Serial.println("Success");
    } else {
      Serial.println("Failed");
    }
  }



// ***********************************************


// callback when data is sent from Master to Slave
esp_now_send_cb_t OnDataSent([](uint8_t *mac_addr, uint8_t status) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Last Packet Sent to: "); Serial.println(macStr);
  Serial.print("Last Packet Send Status: "); Serial.println(status == 0 ? "Delivery Success" : "Delivery Fail");
});

void setup() {
  Serial.begin(115200); 

  // ***********************************************
  pinMode(digitalSwitch, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  // ***********************************************
  
  //Set device in STA mode to begin with
  WiFi.mode(WIFI_STA);
  Serial.println("ESPNow/Single-Slave/Sender Example");
  // This is the mac address of the Master in Station Mode
  Serial.print("STA MAC: "); Serial.println(WiFi.macAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

    // In the loop we scan for slave
  ScanForSlave();
  // If Slave is found, it would be populate in `slave` variable
  // We will check if `slave` is defined and then we proceed further
  if (isServerOnline) { // check if slave channel is defined
    // `slave` is defined
    // Add slave as peer if it has not been added already
    manageSlave();
    // pair success or already paired
    // Send data to device
  } else {
    // No slave found to process
  }
}

void loop() {

  // ***********************************************
  unsigned long currentMillis = millis();
  currentStatusSwitch = digitalRead(digitalSwitch);

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    if (currentStatusSwitch == HIGH) {
      Serial.println("Sensor Touched");
      deviceState = !deviceState;
      digitalWrite(LED_BUILTIN, deviceState);

      sendData();
      
    }
  }
  // ***********************************************
 
}
