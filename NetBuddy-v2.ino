#include "images.hpp"
#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define BUTTON_PREV_PIN 12
#define BUTTON_NEXT_PIN 13
#define BUTTON_SELECT_PIN 14

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define LAN_TEST 0
#define WIFI_TEST 1

float downloadSpeed = 0.0; // Declare and initialize downloadSpeed

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Menu items
const char* menuItems[] = {"LAN Test", "Wifi Test", "Speed Test"};

#define NUM_MENU_ITEMS 3
int selectedItem = 0;

EthernetClient client;
// Set the static IP address to use if the DHCP fails to assign
#define MYIPADDR 192,168,1,28
#define MYIPMASK 255,255,255,0
#define MYDNS 192,168,1,1
#define MYGW 192,168,1,1

unsigned long lastLANRefresh = 0;
const unsigned long LAN_REFRESH_INTERVAL = 15000;


bool ethernetInitialized = false;
int selectedOption = LAN_TEST;
int selectedSSIDIndex = 0;
int buttonBlock = 0;

bool wifiTestRequested = false; // Flag to indicate if Wifi Test is requested

void setup() {
  pinMode(BUTTON_PREV_PIN, INPUT_PULLUP);
  pinMode(BUTTON_NEXT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_SELECT_PIN, INPUT_PULLUP);

  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.display();

  Serial.begin(115200);

  while (!Serial);

  displayAnimation(); // Display opening logo animation
    display.clearDisplay();
  display.display();
  displayMenu();
  
}
void loop() {
  while (buttonBlock < 1) {
  if (digitalRead(BUTTON_PREV_PIN) == LOW) {
    selectedItem = (selectedItem - 1 + NUM_MENU_ITEMS) % NUM_MENU_ITEMS;
    displayMenu();
    delay(200); // debounce
  }

  if (digitalRead(BUTTON_NEXT_PIN) == LOW) {
    selectedItem = (selectedItem + 1) % NUM_MENU_ITEMS;
    displayMenu();
    delay(200); // debounce
  }

  if (digitalRead(BUTTON_SELECT_PIN) == LOW) {
    launchMenuItem(selectedItem);
    delay(200); // debounce
  }
  }
}

void displayMenu() {
  display.clearDisplay();
  switch(selectedItem) {
    case 0:
      display.drawBitmap(0, 0, imgwifitest, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
      break;
    case 1:
      display.drawBitmap(0, 0, imglantest, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
      break;
    case 2:
      display.drawBitmap(0, 0, imgspeedtest, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
      break;
    // Add more cases if you have more menu items
  }
  display.display();
}

void launchMenuItem(int index) {
  display.clearDisplay();
  display.setCursor(0, 10);
  display.println("Launching:");
  display.println(menuItems[index]);
  display.display();
   if (index == 0) {
    displayAniWifi();
    runWIFITest();
  } else if (index == 1) { 
    displayAniLan();
    runLANTest();
  } else if (index == 2) {
    displayAniSpeedtest();
    runSpeedTest();
  }
}

void initETH() {
  
   Serial.println("Starting Ethernet...");
  Ethernet.init(5);   // MKR ETH Shield
 
 
   if (Ethernet.begin(mac)) { // Dynamic IP setup
        Serial.println("DHCP OK!");
            display.clearDisplay();
            display.drawBitmap(0, 0, imgethconnected, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
        displayLANInfo();
    } else {
        Serial.println("Failed to configure Ethernet using DHCP");
        // Check for Ethernet hardware present
        if (Ethernet.hardwareStatus() == EthernetNoHardware) {
          Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
          while (true) {
            delay(1); // do nothing, no point running without Ethernet hardware
          }
        }
        if (Ethernet.linkStatus() == LinkOFF) {
          Serial.println("Ethernet cable is not connected.");
            display.clearDisplay();
            display.drawBitmap(0, 0, imgethdisco1, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
        }
 
          IPAddress ip(MYIPADDR);
          IPAddress dns(MYDNS);
          IPAddress gw(MYGW);
          IPAddress sn(MYIPMASK);
          Ethernet.begin(mac, ip, dns, gw, sn);
          Serial.println("STATIC OK!");
        }
}

void runLANTest() {
          initETH();         
          displayLANInfo();
}


void displayLANInfo() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
    display.print("IP: ");
    display.println(Ethernet.localIP());
    display.print("Subnet: ");
    display.println(Ethernet.subnetMask());
    display.print("Gateway: ");
    display.println(Ethernet.gatewayIP());
    display.print("DNS: ");
    display.println(Ethernet.dnsServerIP());
  display.display();
}


void runWIFITest() {
  
  int numNetworks = WiFi.scanNetworks();
  if (numNetworks == 0) return;
  buttonBlock = 1;

  while (true) {
    if (digitalRead(BUTTON_PREV_PIN) == LOW) {
      selectedSSIDIndex = (selectedSSIDIndex == 0) ? numNetworks - 1 : selectedSSIDIndex - 1;
      display.clearDisplay();
      delay(200);
      
    }

    if (digitalRead(BUTTON_NEXT_PIN) == LOW) {
      selectedSSIDIndex = (selectedSSIDIndex + 1) % numNetworks;
      display.clearDisplay();
      delay(200);
      
    }
   if (digitalRead(BUTTON_SELECT_PIN) == LOW) {
    buttonBlock = 0;
    delay(200); // debounce
  }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print(selectedSSIDIndex);
    display.println(" of ");
    display.println(numNetworks);

    display.print("SSID: ");
    display.println(WiFi.SSID(selectedSSIDIndex));
    display.print("RSSI: ");
    display.println(WiFi.RSSI(selectedSSIDIndex));
    display.print("MAC: ");
    display.println(WiFi.BSSIDstr(selectedSSIDIndex));
    display.display();
  }
}

void displayAnimation() {
  for (int i = 0; i < 2; i++) { // Loop 3 times
    display.clearDisplay();

    // Display each image
    display.drawBitmap(0, 0, image1, 128, 64, WHITE);
    display.display();
    delay(200); // Delay for 1 second

    display.clearDisplay();
    display.drawBitmap(0, 0, image2, 128, 64, WHITE);
    display.display();
    delay(200);

    display.clearDisplay();
    display.drawBitmap(0, 0, image3, 128, 64, WHITE);
    display.display();
    delay(200);

    display.clearDisplay();
    display.drawBitmap(0, 0, image4, 128, 64, WHITE);
    display.display();
    delay(200);
  }
}

void displayAniWifi() {
  for (int i = 0; i < 2; i++) { // Loop 3 times
    display.clearDisplay();

    // Display each image
    display.drawBitmap(0, 0, imagew1, 128, 64, WHITE);
    display.display();
    delay(200); // Delay for 1 second

    display.clearDisplay();
    display.drawBitmap(0, 0, imagew2, 128, 64, WHITE);
    display.display();
    delay(200);

    display.clearDisplay();
    display.drawBitmap(0, 0, imagew3, 128, 64, WHITE);
    display.display();
    delay(200);

    display.clearDisplay();
    display.drawBitmap(0, 0, imagew2, 128, 64, WHITE);
    display.display();
    delay(200);
  }
}

void displayAniLan() {
  for (int i = 0; i < 2; i++) { // Loop 3 times
    display.clearDisplay();

    // Display each image
    display.drawBitmap(0, 0, imlan0, 128, 64, WHITE);
    display.display();
    delay(200); // Delay for 1 second

    display.clearDisplay();
    display.drawBitmap(0, 0, imlan1, 128, 64, WHITE);
    display.display();
    delay(200);

    display.clearDisplay();
    display.drawBitmap(0, 0, imlan2, 128, 64, WHITE);
    display.display();
    delay(200);

    display.clearDisplay();
    display.drawBitmap(0, 0, imlan3, 128, 64, WHITE);
    display.display();
    delay(200);
  }
}

void displayAniSpeedtest() {
  for (int i = 0; i < 2; i++) { // Loop 3 times
    display.clearDisplay();

    // Display each image
    display.drawBitmap(0, 0, imgspeedtest1, 128, 64, WHITE);
    display.display();
    delay(200); // Delay for 1 second

    display.clearDisplay();
    display.drawBitmap(0, 0, imgspeedtest, 128, 64, WHITE);
    display.display();
    delay(200);

    display.clearDisplay();
    display.drawBitmap(0, 0, imgspeedtest3, 128, 64, WHITE);
    display.display();
    delay(200);

    display.clearDisplay();
    display.drawBitmap(0, 0, imgspeedtest, 128, 64, WHITE);
    display.display();
    delay(200);
  }
}

float runSpeedTest() {

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Download Speed:");
  downloadSpeed = getDownloadSpeed(); // Update download speed
  display.print(downloadSpeed);
  display.println(" Mbps");
  display.display();
  delay(5000);

}

float getDownloadSpeed() {
  initETH();
  HTTPClient http;
  http.begin("https://fast.com/download");
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    // Parse the response to extract the download speed
    int startPos = payload.indexOf("speed-value") + 13; // Position of the speed value
    int endPos = payload.indexOf("</span>", startPos);
    String speedString = payload.substring(startPos, endPos);
    return speedString.toFloat();
  } else {
    Serial.printf("Error getting download speed: %d\n", httpCode);
    return -1.0;
  }
}
