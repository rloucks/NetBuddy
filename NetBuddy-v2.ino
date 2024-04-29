#include "images.hpp"
#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Button Configuration
#define BUTTON_PREV_PIN 12
#define BUTTON_NEXT_PIN 13
#define BUTTON_SELECT_PIN 14

// OLED Size Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Menu Item Setup
#define NUM_MENU_ITEMS 3
int selectedItem = 0;
// Menu items
const char* menuItems[] = {"LAN Test", "Wifi Test", "Speed Test"};

// Ethernet Setup
float downloadSpeed = 0.0; // Declare and initialize downloadSpeed

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // Mac Address

EthernetClient client;
// Set the static IP address to use if the DHCP fails to assign
#define MYIPADDR 192,168,1,28
#define MYIPMASK 255,255,255,0
#define MYDNS 192,168,1,1
#define MYGW 192,168,1,1

unsigned long lastLANRefresh = 0;
const unsigned long LAN_REFRESH_INTERVAL = 15000;


//speedtest
// The file to get.
//const char* url      = "https://192.168.1.1/1MB_file.bin";
const char* url      = "http://raw.githubusercontent.com/rloucks/NetBuddy/main/1MB_file.bin";
// How many times to get it per loop.
int  num_downloads   = 10;
int start_time       = micros();
int end_time         = micros();
int response_size    = 0; // Holds the size of the current chunk.
int total_elapsed    = 0; // Holds the elapsed time counter for the loop.
int chunk_elapsed    = 0; // Holds the time to download the current chunk.  Don't count non-200 status.
float chunk_speed    = 0.0; // Holds the speed of the current chunk.
float total_speed    = 0.0; // Holds the speed of the total download.
int download_count   = 0; // This is the counter.  Preset to 0.
int total_bytes      = 0; // Holds the total bytes transferred.

char server[] = "httpbin.org"; // Domain name of the server


// dont touch
bool ethernetInitialized = false; // keeps the ethernet code from running until the LAN test or SpeedTest
//int selectedOption = LAN_TEST; // inital selected option - cant remember if I need it :)
int selectedSSIDIndex = 0; // starting index for the SSID scrollable list
int buttonBlock = 0; // Keep the buttons from getting in the way
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
  display.clearDisplay(); // clean up the display
  display.display();
  displayMenu(); // start the menu
  
}
void loop() {
  while (buttonBlock < 1) { // only perform these actions if button block is not 1

  // previous scroll  
  if (digitalRead(BUTTON_PREV_PIN) == LOW) {
    selectedItem = (selectedItem - 1 + NUM_MENU_ITEMS) % NUM_MENU_ITEMS;
    displayMenu();
    delay(200); // debounce
  }

  // next scroll
  if (digitalRead(BUTTON_NEXT_PIN) == LOW) {
    selectedItem = (selectedItem + 1) % NUM_MENU_ITEMS;
    displayMenu();
    delay(200); // debounce
  }

  // select and launch the next function
  if (digitalRead(BUTTON_SELECT_PIN) == LOW) {
    launchMenuItem(selectedItem);
    delay(200); // debounce
  }
  }
}

void displayMenu() { // code or the menu display - runs on images from image.hpp file
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

void launchMenuItem(int index) { // setup for each function to run on which menu item
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

void initETH() {  // init the ethernet shield
  Serial.println("Starting Ethernet...");
  Ethernet.init(5);   // MKR ETH Shield
 
   if (Ethernet.begin(mac)) { // Dynamic IP setup
        Serial.println("DHCP OK!");
            //display.clearDisplay();
            //display.drawBitmap(0, 0, imgethconnected, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
            delay(500);
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
            //display.clearDisplay();
            //display.drawBitmap(0, 0, imgethdisco1, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
            delay(5000);
        } 
          IPAddress ip(MYIPADDR);
          IPAddress dns(MYDNS);
          IPAddress gw(MYGW);
          IPAddress sn(MYIPMASK);
          Ethernet.begin(mac, ip, dns, gw, sn);
          Serial.println("STATIC OK!");
            //display.clearDisplay();
            //display.drawBitmap(0, 0, imgethconnected, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
            delay(500);
        }
}

void runLANTest() { // doesnt get any simpler than this
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
  unsigned long lastRefreshTime = 0; // Variable to store the last time the RSSI was refreshed
  int r = 0; // Initialize r outside of the loop to persist its value

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
    } else {

    // Check if 5 seconds have passed since the last RSSI refresh
    if (millis() - lastRefreshTime >= 5000) {
      WiFi.scanNetworks(); // Refresh the WiFi scan
      lastRefreshTime = millis(); // Update the last refresh time

      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.println(WiFi.SSID(selectedSSIDIndex));
      display.print(selectedSSIDIndex);
      display.print(" of ");
      display.println(numNetworks);
      
      // Print the countdown for RSSI update
      //display.setCursor(0, 8);
      //display.print("       R: ");
      //display.print(5 - (millis() - lastRefreshTime) / 1000); // Countdown from 5 seconds


      display.println("--------------------");
      display.print("RSSI: ");
      display.println(WiFi.RSSI(selectedSSIDIndex));
      display.println("MAC Address: ");
      display.print(WiFi.BSSIDstr(selectedSSIDIndex));
      display.display();
    }
    }
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

void runSpeedTest() {
  initETH(); 
  const int numTests = 5; // Number of tests to perform
  float downloadSpeeds[numTests]; // Array to store download speeds
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Running Test # 1...");
  display.display();
  
  for (int i = 0; i < numTests; i++) {
  
  

  if (client.connect(server, 80)) { // Connect to server
    client.println("GET /range/102400 HTTP/1.1");
    client.println("Host: httpbin.org");
    client.println("Connection: close");
    client.println();
    
       // Start time measurement
      unsigned long startTime = millis();
      
      // Wait for server response headers
      unsigned long timeout = millis();
      while (!client.available() && millis() - timeout < 5000) {
        delay(100);
      }
      
      if (!client.available()) {
        Serial.println("No response from server");
        return;
      }
      
      // Read response data byte by byte and calculate dataSize
      int dataSize = 0;
      while (client.connected()) {
        if (client.available()) {
          client.read(); // Read and discard the byte
          dataSize++;    // Increment dataSize
        }
      }
      
      // End time measurement
      unsigned long endTime = millis();
      
      // Calculate download time in milliseconds
      unsigned long downloadTime = endTime - startTime;
      
      // Convert dataSize to kilobits
      float dataSizeKbps = (dataSize * 8.0) / 1000.0; // Convert bytes to kilobits
      
      // Calculate download speed in kbps
      float downloadSpeedKbps = dataSizeKbps / (downloadTime / 1000.0); // Convert milliseconds to seconds
      
      // Store download speed in array
      downloadSpeeds[i] = downloadSpeedKbps;
      
      // Close the connection
      client.stop();
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("Results Test # ");
      display.println(i+1);
      display.println("----------------");
  
      display.print("Size: ");
      display.print(dataSize);
      display.println(" bytes");
      
      display.print("Speed: ");
      display.print(downloadSpeedKbps);
      display.println(" kbps");
      display.display();
      
      display.println("");
      display.println("");
      display.println("");
      if (i == 4) {
      display.print("Finlizing Results...");
      display.println("...");
      display.display();
      } else {        
      display.print("Running Test # ");
      display.print(i+2);
      display.println("...");
      display.display();

      }
      
      delay(5000); // Wait for 5 seconds before making the next request
    } else {
      Serial.println("Connection failed");
      delay(1000);
      return;
    }
  }
  
  // Calculate average, maximum, and minimum download speeds
  float totalSpeed = 0;
  float maxSpeed = downloadSpeeds[0];
  float minSpeed = downloadSpeeds[0];
  
  for (int i = 0; i < numTests; i++) {
    totalSpeed += downloadSpeeds[i];
    if (downloadSpeeds[i] > maxSpeed) {
      maxSpeed = downloadSpeeds[i];
    }
    if (downloadSpeeds[i] < minSpeed) {
      minSpeed = downloadSpeeds[i];
    }
  }
  
  float averageSpeed = totalSpeed / numTests;
  
  // Print statistics to serial monitor
  Serial.print("Average download speed: ");
  Serial.print(averageSpeed);
  Serial.println(" kbps");
  
  Serial.print("Maximum download speed: ");
  Serial.print(maxSpeed);
  Serial.println(" kbps");
  
  Serial.print("Minimum download speed: ");
  Serial.print(minSpeed);
  Serial.println(" kbps");
  
  // Display statistics on OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Speed test results");
  display.println("--------------------");
  display.println("");
  display.print("Average: ");
  display.print(averageSpeed);
  display.println(" kbps");
  
  display.print("Max: ");
  display.print(maxSpeed);
  display.println(" kbps");
  
  display.print("Min: ");
  display.print(minSpeed);
  display.println(" kbps");
  display.display();
  
  delay(60000);
}
 
