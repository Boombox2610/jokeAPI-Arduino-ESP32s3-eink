#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

// E-paper library includes (customize as needed for your hardware)
#include "EPD_GUI.h"
#include "Pic.h"

// E-paper buffer
uint8_t ImageBW[2888]; // 152 x 152

// WiFi credentials
const char* ssid = "Maru_2g";
const char* password = "Jio123456";

// Joke API
String jsonBuffer;
JSONVar myObject;
String jokeSetup;
String jokePunchline;

void printWiFiStatus() {
  wl_status_t status = WiFi.status();
  Serial.print("WiFi.status(): ");
  Serial.println(status);
  switch (status) {
    case WL_IDLE_STATUS: Serial.println("Idle"); break;
    case WL_NO_SSID_AVAIL: Serial.println("No SSID available"); break;
    case WL_SCAN_COMPLETED: Serial.println("Scan completed"); break;
    case WL_CONNECTED: Serial.println("Connected"); break;
    case WL_CONNECT_FAILED: Serial.println("Connect failed"); break;
    case WL_CONNECTION_LOST: Serial.println("Connection lost"); break;
    case WL_DISCONNECTED: Serial.println("Disconnected"); break;
    default: Serial.println("Unknown status"); break;
  }
}

void EPD_ShowWordWrappedString(uint16_t x, uint16_t y, const String& text, uint16_t maxCharsPerLine, uint16_t lineSpacing, uint16_t fontSize, uint16_t color) {
    int start = 0;
    int len = text.length();
    int lineNum = 0;
    while (start < len) {
        int end = start + maxCharsPerLine;
        if (end > len) end = len;

        // Find last space within the range
        int actualEnd = end;
        if (end < len) {
            actualEnd = start;
            for (int i = start; i < end; i++) {
                if (text[i] == ' ') actualEnd = i;
            }
            // If no space found, just split at maxCharsPerLine
            if (actualEnd == start) actualEnd = end;
        }

        String line = text.substring(start, actualEnd);
        line.trim(); // Remove leading/trailing spaces
        EPD_ShowString(x, y + lineNum * lineSpacing, line.c_str(), fontSize, color);

        // Skip spaces at start of next line
        start = actualEnd;
        while (start < len && text[start] == ' ') start++;
        lineNum++;
    }
}

// Function to fetch joke JSON and print to Serial
void fetchJoke()
{
    Serial.println("Attempting to fetch joke from API...");
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("WiFi is connected, proceeding with HTTP request.");
        String serverPath = "https://official-joke-api.appspot.com/jokes/random";
        HTTPClient http;
        http.begin(serverPath);
        int httpResponseCode = http.GET();

        Serial.print("HTTP GET Response code: ");
        Serial.println(httpResponseCode);

        if (httpResponseCode > 0) {
            jsonBuffer = http.getString();
            Serial.println("Raw JSON from API:");
            Serial.println(jsonBuffer);

            myObject = JSON.parse(jsonBuffer);
            if (JSON.typeof(myObject) == "undefined") {
                Serial.println("Parsing input failed!");
                jokeSetup = "API Error";
                jokePunchline = "";
                return;
            }

            jokeSetup = JSON.stringify(myObject["setup"]);
            jokePunchline = JSON.stringify(myObject["punchline"]);
            // Remove quotes
            jokeSetup.replace("\"", "");
            jokePunchline.replace("\"", "");

            // Print jokes to Serial for debugging
            Serial.println("Setup: " + jokeSetup);
            Serial.println("Punchline: " + jokePunchline);
        } else {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
            Serial.println("If this is -1, check SSL certs, API URL, or try HTTP instead of HTTPS.");
        }
        http.end();
    }
    else
    {
        Serial.println("WiFi Disconnected - cannot fetch joke.");
        printWiFiStatus();
        jokeSetup = "WiFi Error";
        jokePunchline = "";
    }
}

// Display joke on E-paper (placeholder, adjust for your hardware)
void UI_joke()
{
    char buffer[100];
    Serial.println("Displaying joke on e-paper...");

    EPD_HW_RESET();
    Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);
    Paint_Clear(WHITE);
    EPD_FastMode1Init();
    EPD_FastUpdate();
    EPD_Clear_R26H();

    // Display Setup
    // memset(buffer, 0, sizeof(buffer));
    // snprintf(buffer, sizeof(buffer), "%s", jokeSetup.c_str());
    // EPD_ShowString(0, 10, buffer, 12, BLACK);

    // // Display Punchline
    // memset(buffer, 0, sizeof(buffer));
    // snprintf(buffer, sizeof(buffer), "%s", jokePunchline.c_str());
    // EPD_ShowString(10, 80, buffer, 12, BLACK);

    EPD_ShowWordWrappedString(0, 0, jokeSetup, 25, 16, 12, BLACK);
    EPD_ShowWordWrappedString(0, 60, jokePunchline, 25, 16, 16, BLACK);

    // EPD_ShowWordWrappedString (x, y, text, maxCharsPerLine, lineSpacing, fontSize, color)
    //x -> Horizontal position (pixels from left)
    //y -> Vertical position (pixels from top)
    //text -> The text you want to display (usually a String)
    //maxCharsPerLine -> Max number of characters per line before wrapping
    //lineSpacing -> Vertical space (in pixels) between lines
    // Font size -> 12, 16, 
    // color -> black white

    EPD_Display(ImageBW);
    EPD_FastUpdate();
    EPD_DeepSleep();
    Serial.println("Joke display done.");
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("==== ESP32 Joke E-paper Debug ====");
    Serial.print("WiFi SSID: ");
    Serial.println(ssid);
    Serial.print("WiFi Password: ");
    Serial.println(password);

    // Connect to WiFi with timeout
    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi...");
    int max_tries = 60; // 60 x 500ms = 30 seconds
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < max_tries) {
        delay(500);
        Serial.print(".");
        tries++;
        printWiFiStatus();
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi Connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("Signal strength (RSSI): ");
        Serial.println(WiFi.RSSI());
    } else {
        Serial.println("WiFi connection FAILED after 30 seconds.");
        Serial.println("Double-check your SSID and password.");
        printWiFiStatus();
        // Optionally restart ESP32
        // ESP.restart();
    }

    // Power pin for display
    pinMode(7, OUTPUT);
    digitalWrite(7, HIGH);

    // Initial display setup (optional)
    EPD_GPIOInit();
    Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);
    Paint_Clear(WHITE);
    EPD_FastMode1Init();
    EPD_Display_Clear();
    EPD_FastUpdate();
    EPD_Clear_R26H();
}

void loop() {
    fetchJoke();    // fetch joke and print to Serial
    UI_joke();      // push joke to e-paper
    delay(1000 * 60 * 2); // wait 2 minutes
}