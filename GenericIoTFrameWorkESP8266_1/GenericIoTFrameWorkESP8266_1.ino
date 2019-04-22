// Copyright 2019 Steve Quinn
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//
//
//
//
// Requires MQTT PubSubClient Library found here:                  https://github.com/knolleary/pubsubclient
// Requires ESP8266WiFi Library found here:                        https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
// Requires Arduino IDE support for ESP8266 found here:            https://github.com/esp8266/Arduino#installing-with-boards-manager
// Requires FS file system support found in ESP8266 Core or here : https://github.com/esp8266/Arduino/blob/master/cores/esp8266/FS.h
// Requires ESP8266WebServer Library found here:                   https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer
// Requires ESP8266mDNS Library found here:                        https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266mDNS
// 
// ESP8266 generic IoT framework for connecting to WiFi and Broker has one led output (if fitted to a given dev board)
//
// Compiled on Arduino IDE v1.8.5, Boards Manager : ESP8266 by ESP8266 Community v2.4.2
//
// Also reference the following : https://github.com/esp8266/arduino-esp8266fs-plugin/releases v0.4.0
//
// Files -> Preferences -> Additional Boards Manager URLs: http://arduino.esp8266.com/stable/package_esp8266com_index.json
// 
//
// Maintenance History
// 25/08/18 : 1   First go at creating a generic IoT frame work for connecting to WiFi and Broker. Uses SPIFFs.
// 25/08/18 : 2   Changed all references to 'eGENIOTSTATE_' from 'eSENSORSTATE_'. Corrected fileRead BOOL issue
// 14/04/19 : 2_1 Added in debug to check for missing SPIFFs files. If you get the wrong 'ESP8266 by ESP8266 Community', 'arduino-esp8266fs-plugin', 'Arduino IDE' combo you can get erroneous SPIFFS upload results.
// 22/04/19 : 2_2 Tidied up the code and comments
// 22/04/19 : 1 Rename to upload to GitHub
//
//
// Start up sequence
// -----------------
// Unit starts up in STA_AP mode and looks for SPIFFs file SECURITY_PARAMETERS_FILE. If the file doesn't exist the IoT device switches to AP mode, 
// starts a web server and initialises an mDNS service. The device then broadcasts the SSID AP_NETWORK_SSID_DEFAULT + DeviceMACAddress. To connect 
// to this APnetwork use the following password AP_NETWORK_PASSWORD_DEFAULT once connected enter the following URL into your browser nDNSHostName + '.local'
// The IoT device will then serve up a configuration web page allowing new sensor network security parameters to be submitted.
// If the SPIFFs file is present the information in this file is used by the IoT device to connect to the sensor network. The device then stays in STA_AP 
// mode until it has connected. Once connected to the sensor network the IoT device switches to STA mode.
// If the information in the SECURITY_PARAMETERS_FILE does not allow connection to the sensor network it is possible to connect to the sensors APnetwork as above,
// only now you must enter the IP address 192.168.4.1. This is due to a flaw in the mDNS library of the ESP8266. When in STA_AP mode mDNS service
// will not work.
// Once the device has connected to the sensor network it attempts to connect to the MQTT broker which it expects at the following address MQTT_BROKER_IP_DEFAULT
// and port MQTT_BROKER_PORT_DEFAULT. If the IoT device exceeds mqtt_broker_connection_attempts it will re-initialise as if no SECURITY_PARAMETERS_FILE were present.
// Once connected to the MQTT broker, if the connection to the broker is lost the system re-initialises.
// If mqtt_broker_connection_attempts=0 the IoT device will continue to attempt an MQTT Broker connection.
//
// To give a visual indication of the above connection states, the IoT device will flash the local led as follows.
// 1. When no onboard configuration file is present SECURITY_PARAMETERS_FILE 1 quick flash.
// 2. When attempting to connect to a given WiFi network 2 quick flashes in succession.
// 3. Once a WiFi n/w connection has been achieved. Whilst attempting to connect to an MQTT Broker the led will be on continuously.
// 4. Once WiFi n/w and MQTT Broker connections are in place the led will extinguish.
//
// Notes
// Create a subdirectory named 'data' within the directory holding this sketch.
// Any files residing here will be uploaded to the SPIFFS area on the ESP8266 when Tools->ESP8266 Sketch Data Upload
// Is selected. Though you still need to follow the Flash/Reset button process on the programmer. The file system is uploaded
// Using a different offset to the code which starts from 'flash at 0x00000000'
// Here there are three text files named 'secvals.txt' and 'index.htm' + some other crap such that a little 'IoT' icon appears in the browser address bar.
//
// 'secvals.txt' contains five entries. These values are write only via MQTT topics.
// - 1st MQTT Broker IP Address. In dotted decimal form AAA.BBB.CCC.DDD
// - 2nd MQTT Broker Port. In Integer form.
// - 3rd MQTT Broker connection attempts to make before switching from STA mode to AP mode. In Integer form. 
// - 4th WiFi Network SSID. In free form text.
// - 5th WiFi Network Password. In free form text.
// - 6th WiFi Network connection attempts to make before switching from STA mode to AP mode. In Integer form. // NOTE this is not implemented
//
// 'index.htm'
// Contains web page served up when the device can't connect to the Network using the password held in the 'secvals.txt' file
//
// Arduino IDE programming parameters.
// 
// From Tools Menu
// Board: 'Generic ESP8266 Module'
// Flash Mode: 'DIO' (See : https://hackaday.com/2017/10/01/trouble-flashing-your-esp8266-meet-dio-and-qio/)
// Debug Level: 'None'
// Reset Method: 'ck'
// Flash Frequency: '40MHz'
// CPU Frequency: '240MHz (WiFi/BT)' => '80 MHz' shows 240MHz but actually is 80MHz
// Erase Flash: 'Only Sketch'
// Flash Size: '4M (1MSPIFFS)'  // I used 1M(512K SPIFFS) for my ESP8266-01
// Debug Port: 'Disabled'
// IwIP Variant: 'v2 Lower Memory'
// Crystal Frequency: '26 MHz'
// VTables: 'Flash'
// Flash Frequency: '40MHz'
// Builtin Led: '2'
// Upload Speed: '115200'
// 
// Programmer: 'AVRISP mkII'
//

//#define BOARD_HAS_A_SYSTEM_LED // Define this if the board has a system led fitted. This will enable debug flashing.
//#define DEBUG_GENERAL        // Undefine this for general debug information via the serial port. Note, you must deploy with this undefined as your network security parameters will be sent to serial port
//#define DEBUG_WEB            // Undefine this for comprehensive debug information on web interface via the serial port. Requires DEBUG_GENERAL.
//#define DEBUG_MDNS           // Undefine this for comprehensive debug information on mDNS support via the serial port. Requires DEBUG_GENERAL.
//#define DEBUG_TIMER          // Undefine this for timer debug information via the serial port. Requires DEBUG_GENERAL.
//#define DEBUG_SPIFFS         // Undefine this for SPIFFS debug information via the serial port. Requires DEBUG_GENERAL.
//#define DEBUG_VALIDATION     // Undefine this for validation debug information via the serial port. Requires DEBUG_GENERAL.
//#define DEBUG_STATE_CHANGE   // Undefine this for 'eGENIOTSTATE' State Change debug information via the serial port. Requires DEBUG_GENERAL.
//#define DEBUG_LEDFLASH       // Undefine this for 'eLEDFLASHSTATE' State Change debug information via the serial port. Requires DEBUG_GENERAL.
//#define DEBUG_SECVALS        // Undefine this for MQTT SecVals Topic debug information via the serial port. Requires DEBUG_GENERAL.
//#define DEBUG_PARMGRAB       // Undefine this for parmGrab function debug information via the serial port. Requires DEBUG_GENERAL.

#include "FS.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>



#define TO_UPPER 0xDF
#define MAKE_UPPER_CASE(c)     ((c) & TO_UPPER)
#define SET_BIT(p,whichBit)    ((p) |=  (1    << (whichBit)))
#define CLEAR_BIT(p,whichBit)  ((p) &= ~((1)  << (whichBit)))
#define TOGGLE_BIT(p,whichBit) ((p) ^=  (1    << (whichBit)))
#define BIT_IS_SET(p,whichBit) ((p) &   (1    << (whichBit)))

#define SPIFFS_FILE_READ_MODE  "r"
#define SPIFFS_FILE_WRITE_MODE "w"



#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

#define SECURITY_PARAMETERS_FILE               ((char *)("/secvals.txt"))

boolean bBrokerPresent  = false;

#define MQTT_VERSION MQTT_VERSION_3_1

#define MQTT_BROKER_IP_STRING_MAX_LEN           30
#define MQTT_BROKER_IP_DEFAULT                  "192.168.1.44"
#define MQTT_BROKER_PORT_DEFAULT                ((int)1883)
#define STA_NETWORK_SSID_DEFAULT                "GENIOT"
#define STA_NETWORK_PASSWORD_DEFAULT            "PASSWORD"
#define AP_NETWORK_SSID_DEFAULT                 "GENIOT"
#define AP_NETWORK_PASSWORD_DEFAULT             "PASSWORD"
#define NETWORK_CONNECTION_ATTEMPTS_DEFAULT     ((int)10)
#define MQTT_BROKER_CONNECTION_ATTEMPTS_DEFAULT ((int)10)
#define NETWORK_SSID_STRING_MAX_LEN             32  
#define NETWORK_PASSWORD_STRING_MAX_LEN         40
#define CONNECTION_ATTEMPTS_MAX                 100
#define CONNECTION_ATTEMPTS_MIN                 0

char   mqtt_broker_ip[MQTT_BROKER_IP_STRING_MAX_LEN];
int    mqtt_broker_port;
String macStrForAPSSID;
char   sta_network_ssid[NETWORK_SSID_STRING_MAX_LEN];
char   sta_network_password[NETWORK_PASSWORD_STRING_MAX_LEN];
char   ap_network_ssid[NETWORK_SSID_STRING_MAX_LEN];
char   ap_network_password[NETWORK_PASSWORD_STRING_MAX_LEN];
int    network_connection_attempts = NETWORK_CONNECTION_ATTEMPTS_DEFAULT;
int    mqtt_broker_connection_attempts = MQTT_BROKER_CONNECTION_ATTEMPTS_DEFAULT;
#ifdef DEBUG_GENERAL
int    conDotCountNW   = 0; // Used to print a NW connecting dot each 500ms
int    conDotCountMQTT = 0; // Used to print a MQTT connecting dot each 500ms
#endif

// don't forget to #define 'BOARD_HAS_A_SYSTEM_LED'
//LED on ESP8266 GPIO0
const int lightPin = 0;

// Topic to publish to, to request this device publish the status of its local software version (Generic Device, MAC Addr, Filename.ino). Caution : This a THSen Broadcast message.
const char* swVerTopic = "WFD/GenIoT/SwVer/Command";

char swVerThisDeviceTopic[50];

// Topic to subscribe to, to receive publication of this device's software version. In form (Generic Device, MAC Addr, Filename.ino)
const char* swVerConfirmTopic = "WFD/GenIoT/SwVer/Confirm";

// Topic to publish to, to control the status of this device's local led state
const char* lightTopic = "WFD/GenIoT/1/Led/Command/1";

// Topic to subscribe to, to receive confirmation that this device has recieved a Led control command
const char* lightConfirmTopic = "WFD/GenIoT/1/Led/Confirm/1";

// Topic to subscribe to, to request this device publish the status of its local RSSI for SSID
const char* rssiTopic = "WFD/GenIoT/1/RSSILev";

// Topic to subscribe to, to receive publication of the status of this devices  local RSSI in dBm
const char* rssiConfirmTopic = "WFD/GenIoT/1/RSSILev/Confirm";

// Topic to publish to, to request this device re-format it's local SPIFFS filing system. Response; 0 = Done, Error = 1
// Response is sent via 'spiffsConfirmTopic'
const char* spiffsInitTopic = "WFD/GenIoT/1/SPIFFS/Init/Command";


// Topic to publish to, to request this device store new Security Values in it's local SPIFFS filing system. 
// Responses; 
// 0  = Done, 
// 1  = Failed to open SECURITY_PARAMETERS_FILE for write, 
// 6  = MQTT Broker IP address malformed,
// 7  = MQTT Broker Port number invalid,
// 8  = Network SSID or Network Password Wrong length,
// 9  = MQTT Broker Connection Attempts number invalid,
// 10 = MQTT Broker Connection Attempts out of range,
// 11 = Network Connection Attempts number invalid,
// 12 = Network Connection Attempts out of range,
// 13 = One or more items in the parameter string is missing
// Parameter is in the following form 'BrokerIPAddress,BrokerPort,MQTTBrokerConnectionAttempts,NetworkSSID,NetworkPassword,NetworkConnectionAttempts'
// Where;
// BrokerIPAddress : AAA.BBB.CCC.DDD dotted decimal form
// BrokerPort : Integer form. Typically 1883 for Mosquitto MQTT Broker
// MQTTBrokerConnectionAttempts : CONNECTION_ATTEMPTS_MIN ... CONNECTION_ATTEMPTS_MAX. 0 is a special case meaning keep retrying
// NetworkSSID : Free form text
// NetworkPassword : Free form text
// NetworkConnectionAttempts : Integer form. can be any value as this field is not implemented.
// 
// Response is sent via 'spiffsConfirmTopic'
const char* spiffsNewSecValsTopic  = "WFD/GenIoT/1/SPIFFS/SecVals";

// Topic to subscribe to, to receive publication of response that a given SPIFFS command has received and executed 
const char* spiffsConfirmTopic = "WFD/GenIoT/1/SPIFFS/Conf";



const char* subscriptionsArray[] = {swVerThisDeviceTopic, swVerTopic, lightTopic, rssiTopic, spiffsInitTopic, spiffsNewSecValsTopic};
int maxSubscriptions = 0;

#define WiFiConnected (WiFi.status() == WL_CONNECTED)
WiFiClient wifiClient;
PubSubClient MQTTclient(wifiClient);

String clientName;
const char* THIS_GENERIC_DEVICE = "esp8266";
String swVersion;

// Struct to hold a single timer instance
typedef struct tsTimerInstance {
  void (*tmrcallback)(void);   // Function called when timing period exceeded
  boolean bRunning;            // Flag, set with timer running
  unsigned long ulTimerPeriod; // Timing period in milliseconds
  unsigned long ulStartValue;  // Grab of value from millis() when timer was started, used to calculate elapsed time
} TimerInstance;

#define TOTAL_TIME_IN_MILLISECONDS(H, M, S) ((unsigned long)((((unsigned long)H)*60UL*60UL*1000UL)+(((unsigned long)M)*60UL*1000UL)+(((unsigned long)S)*1000UL)))
#define MAX_TIMERS             1
#define LED_FLASH_TIMER        0

TimerInstance stiTimerArray[MAX_TIMERS];  // Array for holding all the active timer instances


#define FILE_VAR_INSTANCE_TYPE_STRING 0
#define FILE_VAR_INSTANCE_TYPE_FLOAT  1
#define FILE_VAR_INSTANCE_TYPE_INT    2
#define FILE_VAR_INSTANCE_TYPE_BOOL   3

typedef struct tsFileVarInstance {
  int iVarType;  
  void *ptrVar;
} FileVarInstance;


FileVarInstance SecurityVarArray[] = 
{
  {FILE_VAR_INSTANCE_TYPE_STRING, (void *)mqtt_broker_ip                  },
  {FILE_VAR_INSTANCE_TYPE_INT,    (void *)&mqtt_broker_port               },
  {FILE_VAR_INSTANCE_TYPE_INT,    (void *)&mqtt_broker_connection_attempts},
  {FILE_VAR_INSTANCE_TYPE_STRING, (void *)sta_network_ssid                },
  {FILE_VAR_INSTANCE_TYPE_STRING, (void *)sta_network_password            },
  {FILE_VAR_INSTANCE_TYPE_INT,    (void *)&network_connection_attempts    }
};



typedef enum {
   eGENIOTSTATE_INIT         = 0,
   eGENIOTSTATE_NO_CONFIG    = 1,
   eGENIOTSTATE_PENDING_NW   = 2,
   eGENIOTSTATE_PENDING_MQTT = 3,
   eGENIOTSTATE_ACTIVE       = 4
} eGENIOTSTATE;

eGENIOTSTATE eGENIOTSTATE_STATE = eGENIOTSTATE_INIT;

const char* nDNSHostName = "GENIOTSVR";

ESP8266WebServer server(80);
static bool hasSPIFFS = false;
//IPAddress APIPAddress (192,168,1,1);
//IPAddress APNWMask (255,255,255,0);
IPAddress tmpAPIPAddress;


#define LED_FLASH_PERIOD   500 // Time of flash period in mS
#define FLASH_SEQUENCE_MAX 11  // The maximum number of definable states a flash sequence can have

typedef enum {
   eLEDFLASH_NO_CONFIG    = 0,
   eLEDFLASH_PENDING_NW   = 1,
   eLEDFLASH_PENDING_MQTT = 2,
   eLEDFLASH_OFF          = 3,
   eLEDFLASH_SEQUENCE_END = 4
} eLEDFLASHSTATE;

eLEDFLASHSTATE eLEDFLASHSTATE_STATE = eLEDFLASH_OFF;
int iFlashSequenceIndex = 0;

char cFlashProfiles[][FLASH_SEQUENCE_MAX] = {
  "1000000000",  // No Config
  "1010000000",  // Pending NW
  "1111111111",  // Pending MQTT
  "0000000000",  // Off
};

#ifdef DEBUG_STATE_CHANGE
char printStateChangeBuf[200];
const char *SensorStates[]= {
  "eGENIOTSTATE_INIT",
  "eGENIOTSTATE_NO_CONFIG",
  "eGENIOTSTATE_PENDING_NW",
  "eGENIOTSTATE_PENDING_MQTT",
  "eGENIOTSTATE_ACTIVE"
};

char *printStateChange(eGENIOTSTATE ThisState, eGENIOTSTATE NextState, const char *InThisFunction)
{
    printStateChangeBuf[0] = 0x00;
    sprintf(printStateChangeBuf,"State Change %s => %s : Within '%s'",SensorStates[ThisState],SensorStates[NextState],InThisFunction);
    return printStateChangeBuf;
}

#define SHOW_UPDATED_STATE(t,n,f) Serial.println(printStateChange((t),(n),(f)))
#endif

#ifdef DEBUG_LEDFLASH
char printLedStateChangeBuf[200];
const char *LedStates[]= {
  "eLEDFLASH_NO_CONFIG",
  "eLEDFLASH_PENDING_NW",
  "eLEDFLASH_PENDING_MQTT",
  "eLEDFLASH_OFF",
  "eLEDFLASH_SEQUENCE_END"
};

char *printLedStateChange(eLEDFLASHSTATE ThisState, eLEDFLASHSTATE NextState, const char *InThisFunction)
{
    printLedStateChangeBuf[0] = 0x00;
    sprintf(printLedStateChangeBuf,"LED State Change %s => %s : Within '%s'",LedStates[ThisState],LedStates[NextState],InThisFunction);
    return printLedStateChangeBuf;
}

#define SHOW_UPDATED_LED_STATE(t,n,f) Serial.println(printLedStateChange((t),(n),(f)))
#endif


void callback(char* topic, byte* payload, unsigned int length);
void grabParm(char **ptrToParmString, String *recipientString);
int fileWrite(File f, FileVarInstance *fviArray, int iTotalParametersToWrite);
int fileRead(File f, FileVarInstance *fviArray, int iTotalParametersToRead);
void readNetworkSecurityParameters(void);
void connectMQTT(void);
void makeSubscriptions(void);
String macToStr(const uint8_t* mac, boolean addColons);
void timer_create(int iTimerNumber, unsigned long ulTimerPeriod, void (*callbackfn)(void));
void timer_update(void);
void timer_start(int iTimerNumber);
void timer_stop(int iTimerNumber);
void timer_reset(int iTimerNumber);
boolean timer_isRunning(int iTimerNumber);
void timer_change_period(int iTimerNumber, unsigned long ulTimerPeriod);
void ledFlashTimerCallback(void);
void returnOK(String mess);
void returnFail(String msg);
bool loadFromSPIFFS(String path);
void handleNetworkConfig(void);
void handleNotFound(void);
boolean isFloat(String tString);
boolean isValidNumber(String str);
bool isValidIpv4Address(char *st);

void setup() {
  // Start the serial line for debugging
  // This is enabled or the TX/RX port will require 10K pull ups to stop oscillations of the I/Ps which makes the ESP8266-01 pull more current
  //#ifdef DEBUG_GENERAL
  Serial.begin(115200);
  delay(100);
  //#endif

  #ifdef BOARD_HAS_A_SYSTEM_LED
  // Initialize the light as an output and set to HIGH (off)
  pinMode(lightPin, OUTPUT);
  digitalWrite(lightPin, HIGH);
  #endif
  
  // Generate client name based on MAC address
  clientName = THIS_GENERIC_DEVICE;
  clientName += '-';
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac,true);
  macStrForAPSSID = macToStr(mac,false);
  macStrForAPSSID.trim();
  sprintf(swVerThisDeviceTopic,"WFD/%s/SwVer/Command",macToStr(mac, true).c_str());

  swVersion = THIS_GENERIC_DEVICE;
  swVersion += ',';
  swVersion += macToStr(mac,true);
  swVersion += ',';
  swVersion += __FILENAME__;
  #ifdef DEBUG_GENERAL  
  Serial.print("Client Name : ");
  Serial.println(clientName);
  Serial.print("SW Version : ");
  Serial.println(swVersion);
  #endif

  // Set up default security parameters. If all else fails so this device can become an AP.
  strcpy(ap_network_ssid,AP_NETWORK_SSID_DEFAULT);
  strcat(ap_network_ssid,macStrForAPSSID.c_str());
  strcpy(ap_network_password,AP_NETWORK_PASSWORD_DEFAULT);
  strcpy(mqtt_broker_ip, MQTT_BROKER_IP_DEFAULT);
  mqtt_broker_port = MQTT_BROKER_PORT_DEFAULT;
  mqtt_broker_connection_attempts = MQTT_BROKER_CONNECTION_ATTEMPTS_DEFAULT;
  strcpy(sta_network_ssid, STA_NETWORK_SSID_DEFAULT);
  strcpy(sta_network_password, STA_NETWORK_PASSWORD_DEFAULT);
  network_connection_attempts = NETWORK_CONNECTION_ATTEMPTS_DEFAULT;
  bBrokerPresent = true;

  // Set up MQTT auto topic subscription
  maxSubscriptions = sizeof(subscriptionsArray)/sizeof(const char*);  

  // Start filing subsystem
  SPIFFS.begin();
  delay(2000);
 
  // Try to read the security paramaters file. If missing this will set the ssid and p/w for the AP
  readNetworkSecurityParameters();

  // Set up initial conditions
  eGENIOTSTATE_STATE = eGENIOTSTATE_INIT;
  WiFi.mode(WIFI_OFF);

  // Set up timers
  iFlashSequenceIndex = 0;
  eLEDFLASHSTATE_STATE = eLEDFLASH_OFF;
  timer_create(LED_FLASH_TIMER, LED_FLASH_PERIOD, ledFlashTimerCallback);   
  //timer_create(LED_FLASH_TIMER, TOTAL_TIME_IN_MILLISECONDS(0, LED_FLASH_PERIOD, 0), ledFlashTimerCallback);   
  timer_start(LED_FLASH_TIMER);
    
  #ifdef DEBUG_SPIFFS
  String str = "";
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
      str += dir.fileName();
      str += " / ";
      str += dir.fileSize();
      str += "\r\n";
  }
  Serial.print(str);
  #endif

  //////////////////////////////
  // YOUR INIT CODE GOES HERE //
  //////////////////////////////

  delay(2000);
}


void loop(){
  timer_update(); // Update timers
  //MDNS.update();   // Check for any mDNS queries and send responses

  switch (eGENIOTSTATE_STATE) {
    case eGENIOTSTATE_INIT : //
           WiFi.mode(WIFI_OFF);
           delay(1000);
           if ((SPIFFS.exists(SECURITY_PARAMETERS_FILE)) && (bBrokerPresent)) {
              eGENIOTSTATE_STATE = eGENIOTSTATE_PENDING_NW;
              eLEDFLASHSTATE_STATE = eLEDFLASH_PENDING_NW;
              #ifdef DEBUG_STATE_CHANGE
              SHOW_UPDATED_STATE(eGENIOTSTATE_INIT,eGENIOTSTATE_PENDING_NW,"loop");
              #endif
              WiFi.mode(WIFI_AP_STA);
              delay(1000);
              // Read the security paramaters file. 
              readNetworkSecurityParameters();
              // Start STA wifi subsystem
              WiFi.begin((const char *)sta_network_ssid, (const char *)sta_network_password);
              #ifdef DEBUG_GENERAL
              Serial.println("Switching to AP_STA Mode. SecVals Found");
              Serial.print("Connecting to "); Serial.println(sta_network_ssid);
              conDotCountNW = 0;  
              #endif
           } else {
              #ifdef DEBUG_STATE_CHANGE
              SHOW_UPDATED_STATE(eGENIOTSTATE_INIT,eGENIOTSTATE_NO_CONFIG,"loop");
              #endif
              eGENIOTSTATE_STATE = eGENIOTSTATE_NO_CONFIG;
              eLEDFLASHSTATE_STATE = eLEDFLASH_NO_CONFIG;
              WiFi.mode(WIFI_AP);
              delay(1000);
              #ifdef DEBUG_GENERAL
              if (bBrokerPresent)
                Serial.println("Switching to AP Mode. No SecVals found");
              else
                Serial.println("Switching to AP Mode. No MQTT Broker found");
              #endif
           }

           // Start AP wifi subsystem
           WiFi.encryptionType(ENC_TYPE_WEP);
           //WiFi.softAPConfig(APIPAddress,APIPAddress,APNWMask);
           WiFi.softAP((const char *)ap_network_ssid, (const char *)ap_network_password);
          
           // Late binding for MQTT client
           MQTTclient.setServer((const char *)mqtt_broker_ip, mqtt_broker_port); 
           MQTTclient.setCallback(callback);
           hasSPIFFS = true;

           tmpAPIPAddress = WiFi.softAPIP();
           #ifdef DEBUG_GENERAL
           Serial.print("AP IP address: "); Serial.println(tmpAPIPAddress);
           #endif    
          
           //if (MDNS.begin(nDNSHostName, APIPAddress)) {
           if (MDNS.begin(nDNSHostName)) {
             MDNS.addService("http", "tcp", 80);
             #ifdef DEBUG_MDNS
             Serial.println("MDNS responder started");
             Serial.print("You can now connect to http://");
             Serial.print(nDNSHostName);
             Serial.println(".local");
             #endif
           } else {
             #ifdef DEBUG_MDNS
             Serial.println("MDNS responder failed to start");
             #endif
           }

           // Set up HTTP server
           server.on("/0", HTTP_GET, handleNetworkConfig);
           server.onNotFound(handleNotFound);
           server.begin();
           #ifdef DEBUG_WEB
           Serial.println("HTTP server started");
           #endif
           break;

    case eGENIOTSTATE_NO_CONFIG  : // Run only as an access point to allow the user to reconfigure to new network
           server.handleClient();
           delay(10); 
           break;

    case eGENIOTSTATE_PENDING_NW : // Run as an access point to allow the user to reconfigure to new network and as a station trying to connnect to NW
           server.handleClient();
           delay(10); 
           if (WiFiConnected) {
              // Start wifi subsystem
              //WiFi.mode(WIFI_STA);  // Switch off access point
              //#ifdef DEBUG_GENERAL
              //Serial.println();
              //Serial.println("Switching to STA Mode. Now WiFi is connected.");
              //#endif
              //WiFi.begin((const char *)sta_network_ssid, (const char *)sta_network_password);
              eGENIOTSTATE_STATE = eGENIOTSTATE_PENDING_MQTT;
              eLEDFLASHSTATE_STATE = eLEDFLASH_PENDING_MQTT;
              #ifdef DEBUG_STATE_CHANGE
              SHOW_UPDATED_STATE(eGENIOTSTATE_PENDING_NW,eGENIOTSTATE_PENDING_MQTT,"loop");
              #endif
              
              //print out some more debug once connected
              #ifdef DEBUG_GENERAL
              Serial.println("WiFi connected");  
              Serial.print("IP address: ");
              Serial.println(WiFi.localIP());
              #endif
           } else {
              #ifdef DEBUG_GENERAL
              if (conDotCountNW > 50)
                  conDotCountNW = 0;
              if (conDotCountNW == 0)
                Serial.print(".");
              conDotCountNW++;  
              #endif
           }
           break;
    
    case eGENIOTSTATE_PENDING_MQTT : // Try tp connect to MQTT Broker
           connectMQTT();
           break;
    
    case eGENIOTSTATE_ACTIVE : // Run as a WiFi client in active mode
           // Reconnect if connection is lost
           if (!MQTTclient.connected()) {
            #ifdef DEBUG_STATE_CHANGE
            Serial.println();
            Serial.println("Switching to AP_STA Mode. As MQTT has disconnected.");
            #endif
            WiFi.mode(WIFI_AP_STA);
            delay(1000);
            WiFi.encryptionType(ENC_TYPE_WEP);
            //WiFi.softAPConfig(APIPAddress,APIPAddress,APNWMask);
            WiFi.softAP((const char *)ap_network_ssid, (const char *)ap_network_password);
            
            tmpAPIPAddress = WiFi.softAPIP();
            #ifdef DEBUG_GENERAL
            Serial.print("AP IP address: "); Serial.println(tmpAPIPAddress);
            #endif    
            connectMQTT();
           } else //maintain MQTT connection
            MQTTclient.loop();

            /////////////////////////
            // YOUR CODE GOES HERE //
            /////////////////////////
        
           // Delay to allow ESP8266 WIFI functions to run
           //yield();
           delay(10); 
           break;
  }
}



void callback(char* topic, byte* payload, unsigned int length) {

  //convert topic to string to make it easier to work with
  String topicStr = topic; 
  char tmpCharBuf[length+1];
  int tmpInt = 0;

  for (int i=0;i<length;i++) 
    tmpCharBuf[i] = ((char)payload[i]);
  tmpCharBuf[length] = 0x00;
  
  //Print out some debugging info
  #ifdef DEBUG_GENERAL  
  Serial.println("Callback update.");
  Serial.print("Topic: ");
  Serial.println(topicStr);
  Serial.print("Payload: ");
  Serial.println(tmpCharBuf);
  #endif

  
  //turn the light off if the payload is '0' and publish to the MQTT server a confirmation message
  if (strcmp(lightTopic,topic)== 0) {
    #ifdef BOARD_HAS_A_SYSTEM_LED
    if(payload[0] == '1'){ //turn the light on if the payload is '1' and publish the confirmation 
      digitalWrite(lightPin, LOW);
      MQTTclient.publish(lightConfirmTopic, "On");
    } else if (payload[0] == '0'){ //turn the light off if the payload is '0' and publish the confirmation
      digitalWrite(lightPin, HIGH);
      MQTTclient.publish(lightConfirmTopic, "Off");
    } else {
      MQTTclient.publish(lightConfirmTopic, "Err");
    }
    #endif
    return;
  }

  if (strcmp(swVerTopic,topic)== 0) {
    MQTTclient.publish(swVerConfirmTopic, swVersion.c_str());
    return;
  }  

  if (strcmp(swVerThisDeviceTopic,topic)== 0) {
    MQTTclient.publish(swVerConfirmTopic, swVersion.c_str());
    return;
  }  

  // handle RSSI topic, send MQTT confirmation via rssiConfirmTopic  
  if (strcmp(rssiTopic,topic)== 0) {
    int32_t rssi = WiFi.RSSI();
    sprintf(tmpCharBuf,"%ld",rssi);
    MQTTclient.publish(rssiConfirmTopic, tmpCharBuf);
    return;
  }    


  // SPIFFS Handlers
  // Re-initialise the filing system
  if (strcmp(spiffsInitTopic,topic)== 0) {
    if (SPIFFS.format())
      MQTTclient.publish(spiffsConfirmTopic, "0");
    else
      MQTTclient.publish(spiffsConfirmTopic, "1");
    return;
  }  


  // Write new network security values to file and restart IoT device
  if (strcmp(spiffsNewSecValsTopic,topic)== 0) {
    char  *StrPtr = tmpCharBuf;
    char   tmp_mqtt_broker_ip[MQTT_BROKER_IP_STRING_MAX_LEN];
    int    tmp_mqtt_broker_port;
    int    tmp_mqtt_broker_connection_attempts = MQTT_BROKER_CONNECTION_ATTEMPTS_DEFAULT;
    char   tmp_sta_network_ssid[NETWORK_SSID_STRING_MAX_LEN];
    char   tmp_sta_network_password[NETWORK_PASSWORD_STRING_MAX_LEN];
    int    tmp_network_connection_attempts = NETWORK_CONNECTION_ATTEMPTS_DEFAULT;
    String strMQTTBrokerIPAddress;
    String strMQTTBrokerPort;
    String strMQTTBrokerConnectionAttempts;
    String strNetworkSSID;
    String strNetworkPassword;
    String strNetworkConnectionAttempts;

    grabParm(&StrPtr,&strMQTTBrokerIPAddress);
    grabParm(&StrPtr,&strMQTTBrokerPort);
    grabParm(&StrPtr,&strMQTTBrokerConnectionAttempts);
    grabParm(&StrPtr,&strNetworkSSID);
    grabParm(&StrPtr,&strNetworkPassword);
    grabParm(&StrPtr,&strNetworkConnectionAttempts);

/*    
    //sscanf(tmpCharBuf,"%s,%d,%d,%s,%s,%d",tmp_mqtt_broker_ip,&tmp_mqtt_broker_port,&tmp_mqtt_broker_connection_attempts,tmp_sta_network_ssid,tmp_sta_network_password,&tmp_network_connection_attempts);
    os_sprintf(tmpCharBuf,"%s,%d,%d,%s,%s,%d",tmp_mqtt_broker_ip,&tmp_mqtt_broker_port,&tmp_mqtt_broker_connection_attempts,tmp_sta_network_ssid,tmp_sta_network_password,&tmp_network_connection_attempts);
    strMQTTBrokerIPAddress = tmp_mqtt_broker_ip;
    strMQTTBrokerPort = tmp_mqtt_broker_port;
    strMQTTBrokerConnectionAttempts = tmp_mqtt_broker_connection_attempts;
    strNetworkSSID = tmp_sta_network_ssid;
    strNetworkPassword = tmp_sta_network_password;
    strNetworkConnectionAttempts = tmp_network_connection_attempts;
*/    
    #ifdef DEBUG_SECVALS
    Serial.print("SecValsMQTTBrokerIPAddress : "); Serial.println(strMQTTBrokerIPAddress);
    Serial.print("SecValsMQTTBrokerPort : "); Serial.println(strMQTTBrokerPort);
    Serial.print("SecValsMQTTBrokerConnectionAttempts : "); Serial.println(strMQTTBrokerConnectionAttempts);
    Serial.print("SecValsSTANetworkSSID : "); Serial.println(strNetworkSSID);
    Serial.print("SecValsSTANetworkPassword : "); Serial.println(strNetworkPassword);
    Serial.print("SecValsNetworkConnectionAttempts : "); Serial.println(strNetworkConnectionAttempts);
    #endif

    strMQTTBrokerIPAddress.trim();
    strMQTTBrokerPort.trim();
    strMQTTBrokerConnectionAttempts.trim();
    strNetworkSSID.trim();
    strNetworkPassword.trim();
    strNetworkConnectionAttempts.trim();

    if ((strMQTTBrokerIPAddress.length()          == 0) || 
        (strMQTTBrokerPort.length()               == 0) || 
        (strMQTTBrokerConnectionAttempts.length() == 0) || 
        (strNetworkSSID.length()                  == 0) || 
        (strNetworkPassword.length()              == 0) || 
        (strNetworkConnectionAttempts.length()    == 0)) {
      MQTTclient.publish(spiffsConfirmTopic, "13");
      return;
    }
    
    strcpy(tmp_mqtt_broker_ip,strMQTTBrokerIPAddress.c_str());
    if (! isValidIpv4Address((char *)strMQTTBrokerIPAddress.c_str())) {
        MQTTclient.publish(spiffsConfirmTopic, "6");
        return;
    } else {
      //strcpy(tmp_mqtt_broker_ip,strMQTTBrokerIPAddress.c_str());
      if (! isValidNumber(strMQTTBrokerPort)) {
        MQTTclient.publish(spiffsConfirmTopic, "7");
        return;
      } else {
        tmp_mqtt_broker_port = strMQTTBrokerPort.toInt();
        if (((strNetworkSSID.length() == 0)     || (strNetworkSSID.length() >= NETWORK_SSID_STRING_MAX_LEN)) || 
            ((strNetworkPassword.length() == 0) || (strNetworkPassword.length() >= NETWORK_PASSWORD_STRING_MAX_LEN))) {
            MQTTclient.publish(spiffsConfirmTopic, "8");
            return;
        } else {
          strcpy(tmp_sta_network_ssid,strNetworkSSID.c_str());
          strcpy(tmp_sta_network_password,strNetworkPassword.c_str());
  
          if (! isValidNumber(strMQTTBrokerConnectionAttempts)) {
            MQTTclient.publish(spiffsConfirmTopic, "9");
            return;
          } else {
            tmp_mqtt_broker_connection_attempts = strMQTTBrokerConnectionAttempts.toInt();
            if ((tmp_mqtt_broker_connection_attempts < CONNECTION_ATTEMPTS_MIN) || (tmp_mqtt_broker_connection_attempts > CONNECTION_ATTEMPTS_MAX)) {
              MQTTclient.publish(spiffsConfirmTopic, "10");
              return;
            } else {
              if (! isValidNumber(strNetworkConnectionAttempts)) {
                MQTTclient.publish(spiffsConfirmTopic, "11");
                return;
              } else {
                tmp_network_connection_attempts = strNetworkConnectionAttempts.toInt();
                if ((tmp_network_connection_attempts < CONNECTION_ATTEMPTS_MIN) || (tmp_network_connection_attempts > CONNECTION_ATTEMPTS_MAX)) {
                  MQTTclient.publish(spiffsConfirmTopic, "12");
                  return;
                } else {
                  strcpy(mqtt_broker_ip,tmp_mqtt_broker_ip);
                  mqtt_broker_port = tmp_mqtt_broker_port;
                  mqtt_broker_connection_attempts = tmp_mqtt_broker_connection_attempts;
                  strcpy(sta_network_ssid,tmp_sta_network_ssid);
                  strcpy(sta_network_password,tmp_sta_network_password);
                  network_connection_attempts = tmp_network_connection_attempts;
                  // Save new network parameters
                  File f = SPIFFS.open(SECURITY_PARAMETERS_FILE, SPIFFS_FILE_WRITE_MODE);
                  if (!f) {
                    MQTTclient.publish(spiffsConfirmTopic, "1");
                    return;
                  } else {
                    fileWrite(f, SecurityVarArray,(int)(sizeof(SecurityVarArray)/sizeof(tsFileVarInstance)));
                    f.close();
                    MQTTclient.publish(spiffsConfirmTopic, "0");
                    bBrokerPresent = true;
                    #ifdef DEBUG_STATE_CHANGE
                    SHOW_UPDATED_STATE(eGENIOTSTATE_STATE,eGENIOTSTATE_INIT,"callback, spiffsNewSecValsTopic");
                    #endif
                    eGENIOTSTATE_STATE = eGENIOTSTATE_INIT;
                    #ifdef DEBUG_SECVALS
                    Serial.print("SecValsMQTTBrokerIPAddress : "); Serial.println(mqtt_broker_ip);
                    Serial.print("SecValsMQTTBrokerPort : "); Serial.println(mqtt_broker_port);
                    Serial.print("SecValsMQTTBrokerConnectionAttempts : "); Serial.println(mqtt_broker_connection_attempts);
                    Serial.print("SecValsSTANetworkSSID : "); Serial.println(sta_network_ssid);
                    Serial.print("SecValsSTANetworkPassword : "); Serial.println(sta_network_password);
                    Serial.print("SecValsNetworkConnectionAttempts : "); Serial.println(network_connection_attempts);
                    #endif
                    return;
                  }      
                }
              }
            }
          }
        }
      }
    }
  }  
}



void grabParm(char **ptrToParmString, String *recipientString){
  #ifdef DEBUG_PARMGRAB
  Serial.print("**ptrToParmString : "); 
  #endif
  while (**ptrToParmString)
  {
    #ifdef DEBUG_PARMGRAB
    Serial.print(**ptrToParmString);
    #endif
    *recipientString += **ptrToParmString;
    (*ptrToParmString)++;
    if ((**ptrToParmString=='\0') || (**ptrToParmString==','))
    {
      if (**ptrToParmString==',')
        (*ptrToParmString)++;
      #ifdef DEBUG_PARMGRAB
      Serial.println();
      #endif
      return;
    }
  }
}


int fileWrite(File f, FileVarInstance *fviArray, int iTotalParametersToWrite) {
    for (int i = 0; i < iTotalParametersToWrite; i++){
      switch (fviArray[i].iVarType){
        case FILE_VAR_INSTANCE_TYPE_STRING :
                f.println((char *)(fviArray[i].ptrVar));
                break;
        case FILE_VAR_INSTANCE_TYPE_FLOAT :
                char tmpStr[10];
                dtostrf(*((float *)(fviArray[i].ptrVar)),5,2,tmpStr);
                f.println(tmpStr);
                break;
        case FILE_VAR_INSTANCE_TYPE_INT :
                f.println(*((int *)(fviArray[i].ptrVar)));
                break;
        case FILE_VAR_INSTANCE_TYPE_BOOL :
                f.println( ((*((int *)(fviArray[i].ptrVar)))?"1":"0") );
                break;
        default :
                return 1;
      }
  }
  return 0;
}



int fileRead(File f, FileVarInstance *fviArray, int iTotalParametersToRead) {
    String s;
    for (int i = 0; i < iTotalParametersToRead; i++){
      s=f.readStringUntil('\n');
      s.trim();
      switch (fviArray[i].iVarType){
        case FILE_VAR_INSTANCE_TYPE_STRING :
                strcpy((char *)(fviArray[i].ptrVar),s.c_str());
                break;
        case FILE_VAR_INSTANCE_TYPE_FLOAT :
                *((float *)(fviArray[i].ptrVar)) = s.toFloat();
                break;
        case FILE_VAR_INSTANCE_TYPE_INT :
                *((int *)(fviArray[i].ptrVar)) = s.toInt();
                break;
        case FILE_VAR_INSTANCE_TYPE_BOOL :
                *((bool *)(fviArray[i].ptrVar)) = (s.toInt()==0?false:true);
                break;
        default : // Unknown data type
                return 1;
      }
  }
  return 0; // Successful completion
}


void readNetworkSecurityParameters(){
  // open file for reading
  String s;
  File f = SPIFFS.open(SECURITY_PARAMETERS_FILE, SPIFFS_FILE_READ_MODE);
  if (!f) {
    strcpy(mqtt_broker_ip, MQTT_BROKER_IP_DEFAULT);
    mqtt_broker_port = MQTT_BROKER_PORT_DEFAULT;
    mqtt_broker_connection_attempts = MQTT_BROKER_CONNECTION_ATTEMPTS_DEFAULT;
    strcpy(sta_network_ssid, STA_NETWORK_SSID_DEFAULT);
    strcpy(sta_network_password, STA_NETWORK_PASSWORD_DEFAULT);
    network_connection_attempts = NETWORK_CONNECTION_ATTEMPTS_DEFAULT;
    #ifdef DEBUG_GENERAL
    Serial.println("Failed to read SPIFFS Sec Vals. Using defaults");
    #endif
  } else {
    fileRead(f, SecurityVarArray,(int)(sizeof(SecurityVarArray)/sizeof(tsFileVarInstance)));
    f.close();
  }
  strcpy(ap_network_ssid,AP_NETWORK_SSID_DEFAULT);
  strcat(ap_network_ssid,macStrForAPSSID.c_str());
  strcpy(ap_network_password,AP_NETWORK_PASSWORD_DEFAULT);
  #ifdef DEBUG_GENERAL
  Serial.println("readNetworkSecurityParameters");
  Serial.print("Broker IP : ");            Serial.println(mqtt_broker_ip);
  Serial.print("Broker Port : ");          Serial.println(mqtt_broker_port);
  Serial.print("Max MQTT Conn Atmpts : "); Serial.println(mqtt_broker_connection_attempts);
  Serial.print("STA SSID : ");             Serial.println(sta_network_ssid);
  Serial.print("STA PW : ");               Serial.println(sta_network_password);
  Serial.print("Max NW Conn Atmpts : ");   Serial.println(network_connection_attempts);
  Serial.print("AP SSID : ");              Serial.println(ap_network_ssid);
  Serial.print("AP PW : ");                Serial.println(ap_network_password);
  #endif
}




void connectMQTT() {
  int connection_counts = 0;
  eGENIOTSTATE tmpeGENIOTSTATE_STATE;
  bBrokerPresent = true;
  #ifdef DEBUG_GENERAL
  conDotCountMQTT = 0;
  #endif
  tmpeGENIOTSTATE_STATE = eGENIOTSTATE_STATE; // Record the state connectMQTT was entered from. 
  // Make sure we are connected to WIFI before attemping to reconnect to MQTT
  eLEDFLASHSTATE_STATE = eLEDFLASH_PENDING_MQTT;
  timer_update(); // Update timers
  if(WiFi.status() == WL_CONNECTED){
    // Loop until we're reconnected to the MQTT server
    #ifdef DEBUG_STATE_CHANGE
    SHOW_UPDATED_STATE(eGENIOTSTATE_STATE,eGENIOTSTATE_PENDING_MQTT,"connectMQTT");
    #endif
    eGENIOTSTATE_STATE = eGENIOTSTATE_PENDING_MQTT;
    #ifdef DEBUG_GENERAL
    Serial.print("Attempting MQTT connection");
    #endif
    while (!MQTTclient.connected()) {
      #ifdef DEBUG_GENERAL
      if (conDotCountMQTT > 50)
          conDotCountMQTT = 0;
      if (conDotCountMQTT == 0)
        Serial.print(".");
      conDotCountMQTT++;  
      #endif
    
      timer_update(); // Update timers
      server.handleClient();      
      //if connected, subscribe to the topic(s) we want to be notified about
      if (MQTTclient.connect((char*) clientName.c_str())) {
        // Start wifi subsystem
        WiFi.mode(WIFI_STA);  // Switch off access point and turn into station only
        //WiFi.begin((const char *)sta_network_ssid, (const char *)sta_network_password);
        #ifdef DEBUG_GENERAL
        Serial.println();
        Serial.println("Switching to STA Mode. Now MQTT is connected.");
        #endif
        MQTTclient.publish(swVerConfirmTopic, swVersion.c_str());        
        makeSubscriptions();
        #ifdef DEBUG_STATE_CHANGE
        SHOW_UPDATED_STATE(eGENIOTSTATE_STATE,eGENIOTSTATE_ACTIVE,"connectMQTT");
        #endif
        eGENIOTSTATE_STATE = eGENIOTSTATE_ACTIVE;
        eLEDFLASHSTATE_STATE = eLEDFLASH_SEQUENCE_END;
      } else { //otherwise print failed for debugging
        #ifdef DEBUG_GENERAL
        Serial.println("\tFailed."); 
        #endif
        //abort();
      }
      
      if(WiFi.status() != WL_CONNECTED) { // Catches a lost NW whilst looking for MQTT broker
        #ifdef DEBUG_STATE_CHANGE
        SHOW_UPDATED_STATE(eGENIOTSTATE_STATE,eGENIOTSTATE_INIT,"connectMQTT WiFi lost");
        #endif
        eGENIOTSTATE_STATE = eGENIOTSTATE_INIT;
        eLEDFLASHSTATE_STATE = eLEDFLASH_SEQUENCE_END;
        return;
      }
      
      if (eGENIOTSTATE_STATE == eGENIOTSTATE_INIT) // Catches the state where device is hung in MQTT pending mode with mqtt_broker_connection_attempts==0 and user sets new config via handleNetworkConfig
      {
        eLEDFLASHSTATE_STATE = eLEDFLASH_SEQUENCE_END;
        timer_update();
        return;
      }  
      
      if ((connection_counts >= mqtt_broker_connection_attempts) && (mqtt_broker_connection_attempts > 0))
      {
        #ifdef DEBUG_STATE_CHANGE
        SHOW_UPDATED_STATE(eGENIOTSTATE_STATE,eGENIOTSTATE_INIT,"connectMQTT con count");
        #endif
        eGENIOTSTATE_STATE = eGENIOTSTATE_INIT;
        eLEDFLASHSTATE_STATE = eLEDFLASH_SEQUENCE_END;
        timer_update();
        if (tmpeGENIOTSTATE_STATE == eGENIOTSTATE_ACTIVE)
          bBrokerPresent = true; // Force programme to go back to eGENIOTSTATE_INIT state if MQTT Broker conn lost after having connected to the nw and broker at least once
        else
          bBrokerPresent = false; // Force programme to go to eGENIOTSTATE_NO_CONFIG State if after MQTT connection attempts made and never having made an MQTT on this nw before
        return;
      }
      
      if (mqtt_broker_connection_attempts > 0)
        connection_counts++;
      delay(10);
    }
  } else { // catches a lost NW as the cause for an MQTT broker connection failure
    #ifdef DEBUG_STATE_CHANGE
    SHOW_UPDATED_STATE(eGENIOTSTATE_STATE,eGENIOTSTATE_INIT,"connectMQTT no WiFi at start");
    #endif
    eGENIOTSTATE_STATE = eGENIOTSTATE_INIT;
    eLEDFLASHSTATE_STATE = eLEDFLASH_SEQUENCE_END;
  }
}


void makeSubscriptions(void)
{
// Fixes these : https://github.com/knolleary/pubsubclient/issues/141
//             : https://github.com/knolleary/pubsubclient/issues/98
  for (int index = 0; index < maxSubscriptions; index++)
  {
    MQTTclient.subscribe(subscriptionsArray[index]);
    for (int i=0;i<10;i++) {
      MQTTclient.loop();
      delay(10);
    }
  }
}


//generate unique name from MAC addr
String macToStr(const uint8_t* mac, boolean addColons){

  String result;

  for (int i = 0; i < 6; ++i) {
    if ((mac[i] & 0xF0) == 0)
      result += String(0, HEX); // stop suppression of leading zero
    result += String(mac[i], HEX);

    if (addColons && (i < 5)){
      result += ':';
    }
  }
  
  return result;
}


void timer_create(int iTimerNumber, unsigned long ulTimerPeriod, void (*callbackfn)(void))
{
  if (iTimerNumber <= MAX_TIMERS)
  {
    stiTimerArray[iTimerNumber].tmrcallback = callbackfn;
    stiTimerArray[iTimerNumber].bRunning = false;
    stiTimerArray[iTimerNumber].ulTimerPeriod = ulTimerPeriod;
    stiTimerArray[iTimerNumber].ulStartValue = 0;
    #ifdef DEBUG_TIMER
    Serial.print(F("T Create, TNum : "));
    Serial.print(iTimerNumber);
    Serial.print(F(", TPeriod : "));
    Serial.println(ulTimerPeriod);
    #endif
  }
}



void timer_update(void)
{
  unsigned long ulCurrentTime = millis();
  unsigned long ulElapsedTime = 0;
  
  for (int iIndex = 0; iIndex < MAX_TIMERS; iIndex++)
  {
    if (stiTimerArray[iIndex].bRunning)
    {
      ulElapsedTime = ulCurrentTime - stiTimerArray[iIndex].ulStartValue;
      /* // Argh! twos complement arithmetic, I hate it...
      if (ulCurrentTime < stiTimerArray[iIndex].ulStartValue) // Cater for UL counter wrap ~every day
        ulElapsedTime = ulCurrentTime - stiTimerArray[iIndex].ulStartValue;
      else  
        ulElapsedTime = ulCurrentTime + (ULONG_MAX - stiTimerArray[iIndex].ulStartValue);
      */
      #ifdef DEBUG_TIMER
      Serial.print(F("T Up, TNum : "));
      Serial.print(iIndex);
      Serial.print(F(", T Elapsed : "));
      Serial.println(ulElapsedTime);
      #endif
        
      if (ulElapsedTime >= stiTimerArray[iIndex].ulTimerPeriod)
      {
        stiTimerArray[iIndex].bRunning = false;
        stiTimerArray[iIndex].tmrcallback();
      }
    }
  }
}


void timer_start(int iTimerNumber)
{
  if (iTimerNumber <= MAX_TIMERS)
  {
    stiTimerArray[iTimerNumber].ulStartValue = millis();
    stiTimerArray[iTimerNumber].bRunning = true;
    #ifdef DEBUG_TIMER
    Serial.print(F("T Start , TNum : "));
    Serial.print(iTimerNumber);
    Serial.print(F(", TStart : "));
    Serial.println(stiTimerArray[iTimerNumber].ulStartValue);
    #endif
  }
}


void timer_stop(int iTimerNumber)
{
  if (iTimerNumber <= MAX_TIMERS)
    stiTimerArray[iTimerNumber].bRunning = false;
  #ifdef DEBUG_TIMER
  Serial.print(F("T Stop : "));
  Serial.println(iTimerNumber);
  #endif
}


void timer_reset(int iTimerNumber)
{
  if (iTimerNumber <= MAX_TIMERS)
    stiTimerArray[iTimerNumber].ulStartValue = millis();
  #ifdef DEBUG_TIMER
  Serial.print(F("T Reset : "));
  Serial.println(iTimerNumber);
  #endif
}


boolean timer_isRunning(int iTimerNumber)
{
  return stiTimerArray[iTimerNumber].bRunning;
}


void timer_change_period(int iTimerNumber, unsigned long ulTimerPeriod)
{
  boolean bTmpRunning;
  if (iTimerNumber <= MAX_TIMERS)
  {
    bTmpRunning = stiTimerArray[iTimerNumber].bRunning;
    stiTimerArray[iTimerNumber].bRunning = false;
    stiTimerArray[iTimerNumber].ulTimerPeriod = ulTimerPeriod;
    stiTimerArray[iTimerNumber].bRunning = bTmpRunning;
    #ifdef DEBUG_TIMER
    Serial.print(F("T Change Period, TNum : "));
    Serial.print(iTimerNumber);
    Serial.print(F(", TPeriod : "));
    Serial.println(ulTimerPeriod);
    #endif
  }
}


  
void ledFlashTimerCallback(void)
{
  // This is called if the led flash timer has timed out. 
  #ifdef DEBUG_TIMER
  Serial.println("In ledFlashTimerCallback()");
  #endif


  #ifdef DEBUG_LEDFLASH
  SHOW_UPDATED_LED_STATE(eLEDFLASHSTATE_STATE,eLEDFLASHSTATE_STATE,"ledFlashTimerCallback");
  Serial.print("Led Flash : ");
  Serial.print(cFlashProfiles[eLEDFLASHSTATE_STATE][iFlashSequenceIndex]);
  Serial.print(", Led Flash Ind : ");
  Serial.print(iFlashSequenceIndex);
  Serial.print(", Led Flash State : ");
  Serial.println(eLEDFLASHSTATE_STATE);
  #endif


  switch (eLEDFLASHSTATE_STATE){
    case eLEDFLASH_NO_CONFIG    :
    case eLEDFLASH_PENDING_NW   :
    case eLEDFLASH_PENDING_MQTT :
        #ifdef BOARD_HAS_A_SYSTEM_LED
        if (cFlashProfiles[eLEDFLASHSTATE_STATE][iFlashSequenceIndex] == '1')
          digitalWrite(lightPin, LOW); // Led on
        else  
          digitalWrite(lightPin, HIGH); // Led off
        #endif
        break;
        
    case eLEDFLASH_SEQUENCE_END : 
        #ifdef BOARD_HAS_A_SYSTEM_LED
        digitalWrite(lightPin, HIGH); // Led off
        #endif
        eLEDFLASHSTATE_STATE = eLEDFLASH_OFF;
        break;
        
    case eLEDFLASH_OFF : 
        iFlashSequenceIndex = 0;
        break;
        
    default : 
        break;
  }

  iFlashSequenceIndex++;
  if (iFlashSequenceIndex >= (FLASH_SEQUENCE_MAX-1))
    iFlashSequenceIndex = 0;
  
  timer_start(LED_FLASH_TIMER);
  #ifdef DEBUG_SPIFFS
  Serial.println("In ledFlashTimerCallback");
  #endif
}



void returnOK(String mess) {
  #ifdef DEBUG_WEB
  Serial.println("returnOK");  
  #endif
  if (mess.length() > 0)
    server.send(200, "text/html", mess);
  else  
    server.send(200, "text/plain", "");
}

void returnFail(String msg) {
  #ifdef DEBUG_WEB
  Serial.println("returnFail");  
  #endif
  server.send(500, "text/plain", msg + "\r\n");
}

bool loadFromSPIFFS(String path){
  String dataType = "text/plain";
  #ifdef DEBUG_WEB
  Serial.println("loadFromSPIFFS");  
  #endif
  if(path.endsWith("/")) path += "index.htm";

  if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if(path.endsWith(".htm")) dataType = "text/html";
  else if(path.endsWith(".css")) dataType = "text/css";
  else if(path.endsWith(".js")) dataType = "application/javascript";
  else if(path.endsWith(".json")) dataType = "application/json";
  else if(path.endsWith(".png")) dataType = "image/png";
  else if(path.endsWith(".gif")) dataType = "image/gif";
  else if(path.endsWith(".jpg")) dataType = "image/jpeg";
  else if(path.endsWith(".ico")) dataType = "image/x-icon";
  else if(path.endsWith(".xml")) dataType = "text/xml";
  else if(path.endsWith(".pdf")) dataType = "application/pdf";
  else if(path.endsWith(".zip")) dataType = "application/zip";
  else if(path.endsWith(".png")) dataType = "image/png";

  File dataFile = SPIFFS.open(path.c_str(),SPIFFS_FILE_READ_MODE);

  if (!dataFile)
    return false;

  if (server.hasArg("download")) dataType = "application/octet-stream";

  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
    #ifdef DEBUG_WEB
    Serial.println("Sent less data than expected!");
    #endif
  }

  dataFile.close();
  return true;
}




void handleNetworkConfig()
{
  String pass_response;
  String fail_response;
  char tmp_mqtt_broker_ip[MQTT_BROKER_IP_STRING_MAX_LEN];
  int  tmp_mqtt_broker_port;
  int  tmp_mqtt_broker_connection_attempts = MQTT_BROKER_CONNECTION_ATTEMPTS_DEFAULT;
  char tmp_sta_network_ssid[NETWORK_SSID_STRING_MAX_LEN];
  char tmp_sta_network_password[NETWORK_PASSWORD_STRING_MAX_LEN];
  int  tmp_network_connection_attempts = NETWORK_CONNECTION_ATTEMPTS_DEFAULT;
  //char tmp_ap_network_ssid[NETWORK_SSID_STRING_MAX_LEN];
  //char tmp_ap_network_password[NETWORK_PASSWORD_STRING_MAX_LEN];

  pass_response  = "<html>";
  pass_response += "  <head>";
  pass_response += "   <title>Form submitted</title>";
  pass_response += " </head>";
  pass_response += " <body>";
  pass_response += "   <p><font face='Helvetica, Arial, sans-serif' size='5' color='#3366ff'> <b> Generic IoT Device Configuration Home Page </b> </font></p>";
  pass_response += "   <p><font face='Helvetica, Arial, sans-serif'>New configuration details now submitted</font></p>";
  pass_response += "   <p><font face='Helvetica, Arial, sans-serif'><a href='index.htm'>Return to main page</a></font></p>";
  pass_response += " </body>";
  pass_response += "</html>";  

  fail_response  = "<html>";
  fail_response += "  <head>";
  fail_response += "   <title>Form not submitted</title>";
  fail_response += " </head>";
  fail_response += " <body>";
  fail_response += "   <p><font face='Helvetica, Arial, sans-serif' size='5' color='#3366ff'> <b> Generic IoT Device Configuration Home Page </b> </font></p>";
  fail_response += "   <p><font face='Helvetica, Arial, sans-serif'>Return to main page and re-submit details</font></p>";
  fail_response += "   <p><font face='Helvetica, Arial, sans-serif'><a href='index.htm'>Return to main page</a></font></p>";
  fail_response += " </body>";
  fail_response += "</html>";  

  #ifdef DEBUG_WEB
  Serial.println("handleNetworkConfig");
  #endif
  String strMQTTBrokerIPAddress=server.arg("MQTTBrokerIPAddress");
  String strMQTTBrokerPort=server.arg("MQTTBrokerPort");
  String strMQTTBrokerConnectionAttempts=server.arg("MQTTBrokerConnectionAttempts");
  String strNetworkSSID=server.arg("NetworkSSID");
  String strNetworkPassword=server.arg("NetworkPassword");
  String strNetworkConnectionAttempts=server.arg("NetworkConnectionAttempts");

  strMQTTBrokerIPAddress.trim();
  strMQTTBrokerPort.trim();
  strMQTTBrokerConnectionAttempts.trim();
  strNetworkSSID.trim();
  strNetworkPassword.trim();
  strNetworkConnectionAttempts.trim();

  strcpy(tmp_mqtt_broker_ip,strMQTTBrokerIPAddress.c_str());
  if (! isValidIpv4Address((char *)strMQTTBrokerIPAddress.c_str())) {
    returnOK(fail_response);
  } else {
    //strcpy(tmp_mqtt_broker_ip,strMQTTBrokerIPAddress.c_str());
    if (! isValidNumber(strMQTTBrokerPort)) {
      returnOK(fail_response);
    } else {
      tmp_mqtt_broker_port = strMQTTBrokerPort.toInt();
      if (((strNetworkSSID.length() == 0)     || (strNetworkSSID.length() >= NETWORK_SSID_STRING_MAX_LEN)) || 
          ((strNetworkPassword.length() == 0) || (strNetworkPassword.length() >= NETWORK_PASSWORD_STRING_MAX_LEN))) {
        returnOK(fail_response);
      } else {
        strcpy(tmp_sta_network_ssid,strNetworkSSID.c_str());
        strcpy(tmp_sta_network_password,strNetworkPassword.c_str());

        if (! isValidNumber(strMQTTBrokerConnectionAttempts)) {
          returnOK(fail_response);
        } else {
          tmp_mqtt_broker_connection_attempts = strMQTTBrokerConnectionAttempts.toInt();
          if ((tmp_mqtt_broker_connection_attempts < CONNECTION_ATTEMPTS_MIN) || (tmp_mqtt_broker_connection_attempts > CONNECTION_ATTEMPTS_MAX)) {
            returnOK(fail_response);
          } else {
            if (! isValidNumber(strNetworkConnectionAttempts)) {
              returnOK(fail_response);
            } else {
              tmp_network_connection_attempts = strNetworkConnectionAttempts.toInt();
              if ((tmp_network_connection_attempts < CONNECTION_ATTEMPTS_MIN) || (tmp_network_connection_attempts > CONNECTION_ATTEMPTS_MAX)) {
                returnOK(fail_response);
              } else {
                strcpy(mqtt_broker_ip,tmp_mqtt_broker_ip);
                mqtt_broker_port = tmp_mqtt_broker_port;
                mqtt_broker_connection_attempts = tmp_mqtt_broker_connection_attempts;
                strcpy(sta_network_ssid,tmp_sta_network_ssid);
                strcpy(sta_network_password,tmp_sta_network_password);
                network_connection_attempts = tmp_network_connection_attempts;
                bBrokerPresent = true;
                // Save new network parameters
                File f = SPIFFS.open(SECURITY_PARAMETERS_FILE, SPIFFS_FILE_WRITE_MODE);
                if (f) {
                  fileWrite(f, SecurityVarArray,(int)(sizeof(SecurityVarArray)/sizeof(tsFileVarInstance)));
                  f.close();
                }      
                returnOK(pass_response);
                #ifdef DEBUG_STATE_CHANGE
                SHOW_UPDATED_STATE(eGENIOTSTATE_STATE,eGENIOTSTATE_INIT,"handleNetworkConfig");
                #endif
                eGENIOTSTATE_STATE = eGENIOTSTATE_INIT;
              }
            }
          }
        }
      }
    }
  }
 
  #ifdef DEBUG_WEB
  Serial.print("MQTTBrokerIPAddress : "); Serial.println(mqtt_broker_ip);
  Serial.print("MQTTBrokerPort : "); Serial.println(mqtt_broker_port);
  Serial.print("MQTTBrokerConnectionAttempts : "); Serial.println(mqtt_broker_connection_attempts);
  Serial.print("STANetworkSSID : "); Serial.println(sta_network_ssid);
  Serial.print("STANetworkPassword : "); Serial.println(sta_network_password);
  Serial.print("NetworkConnectionAttempts : "); Serial.println(network_connection_attempts);
  #endif
  return;
}



/*
 * http://www.esp8266.com/viewtopic.php?f=29&t=2153
 * 
Processing arguments of GET and POST requests is also easy enough. Let's make our sketch turn a led on or off depending on the value of a request argument.
http://<ip address>/led?state=on will turn the led ON
http://<ip address>/led?state=off will turn the led OFF
CODE: SELECT ALL
server.on("/led", []() {
  String state=server.arg("state");
  if (state == "on") digitalWrite(13, LOW);
  else if (state == "off") digitalWrite(13, HIGH);
  server.send(200, "text/plain", "Led is now " + state);
});
- See more at: http://www.esp8266.com/viewtopic.php?f=29&t=2153#sthash.7O0kU5VW.dpuf
 */

void handleNotFound(){
  #ifdef DEBUG_WEB
  Serial.println("handleNotFound");
  #endif
  if(hasSPIFFS && loadFromSPIFFS(server.uri())) return;
  String message = " Not Detected\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " NAME:"+server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  #ifdef DEBUG_WEB
  Serial.print(message);
  #endif
}



boolean isFloat(String tString) {
  String tBuf;
  boolean decPt = false;
  
  if(tString.charAt(0) == '+' || tString.charAt(0) == '-') tBuf = &tString[1];
  else tBuf = tString;  

  for(int x=0;x<tBuf.length();x++)
  {
    if(tBuf.charAt(x) == '.') {
      if(decPt) return false;
      else decPt = true;  
    }    
    else if(tBuf.charAt(x) < '0' || tBuf.charAt(x) > '9') return false;
  }
  return true;
}



boolean isValidNumber(String str){
   for(byte i=0;i<str.length();i++)
   {
      if(!isDigit(str.charAt(i))) return false;
   }
   return true;
} 

/*
boolean isValidNumber(String str){
   for(byte i=0;i<str.length();i++)
   {
      if(isDigit(str.charAt(i))) return true;
   }
   return false;
} 
*/



bool isValidIpv4Address(char *st)
{
    int num, i, len;
    char *ch;

    //counting number of quads present in a given IP address
    int quadsCnt=0;

    #ifdef DEBUG_VALIDATION
    Serial.print("Split IP: ");
    Serial.println(st);
    #endif
    len = strlen(st);

    //  Check if the string is valid
    if(len<7 || len>15)
        return false;

    ch = strtok(st, ".");

    while (ch != NULL) 
    {
        quadsCnt++;
        #ifdef DEBUG_VALIDATION
        Serial.print("Quald ");
        Serial.print(quadsCnt);
        Serial.print(" is ");
        Serial.println(ch);
        #endif

        num = 0;
        i = 0;

        //  Get the current token and convert to an integer value
        while(ch[i]!='\0')
        {
            num = num*10;
            num = num+(ch[i]-'0');
            i++;
        }

        if(num<0 || num>255)
        {
            #ifdef DEBUG_VALIDATION
            Serial.println("Not a valid ip");
            #endif
            return false;
        }

        if( (quadsCnt == 1 && num == 0) || (quadsCnt == 4 && num == 0))
        {
            #ifdef DEBUG_VALIDATION
            Serial.print("Not a valid ip, quad: ");
            Serial.print(quadsCnt);
            Serial.print(" AND/OR quad: ");
            Serial.print(quadsCnt);
            Serial.println(" is zero");
            #endif
            return false;
        }

        ch = strtok(NULL, ".");
    }

    //  Check the address string, should be n.n.n.n format
    if(quadsCnt!=4)
    {
        return false;
    }

    //  Looks like a valid IP address
    return true;
}




