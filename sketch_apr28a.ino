#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Update.h>

// WiFi Credentials
const char* ssid = "Abdul 926";
const char* password = "abdul@786";

// Server URL (for sensor data and firmware updates)
const char* serverName = "https://mbsd-production.up.railway.app/addReading";  // For sending sensor data
const char* firmwareURL = "https://mbsd-production.up.railway.app/firmware";   // For firmware update (from tmp folder)

// DHT Sensor
#define DHTPIN 26
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Time intervals
const unsigned long dataInterval = 15000;  // Send data every 15 seconds
const unsigned long otaCheckInterval = 300000;  // Check for OTA update every 5 minutes

unsigned long lastDataTime = 0;
unsigned long lastOtaCheckTime = 0;

void setup() {
  Serial.begin(115200);
  dht.begin();

  connectToWiFi();
  checkForOTAUpdate();  // Check for OTA update on startup
}

void loop() {
  // Check if it's time to send sensor data
  if (millis() - lastDataTime >= dataInterval) {
    lastDataTime = millis();
    sendSensorData();
  }

  // Check if it's time to check for OTA update
  if (millis() - lastOtaCheckTime >= otaCheckInterval) {
    lastOtaCheckTime = millis();
    checkForOTAUpdate();
  }
}

// Function to connect to WiFi
void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected.");
  Serial.println(WiFi.localIP());
}

// Function to send sensor data to the server
void sendSensorData() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.printf("Temp: %.2f °C, Humidity: %.2f %%\n", temperature, humidity);

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);  // Specify the URL
    http.addHeader("Content-Type", "application/json");

    // Create JSON payload
    StaticJsonDocument<200> doc;
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;

    String json;
    serializeJson(doc, json);  // Convert JSON document to string

    int httpResponseCode = http.POST(json);  // Send HTTP POST request
    Serial.printf("HTTP Response code: %d\n", httpResponseCode);

    if (httpResponseCode > 0) {
      Serial.println("Data sent successfully.");
    } else {
      Serial.printf("Error sending data: %d\n", httpResponseCode);
    }

    http.end();  // Close connection
  } else {
    Serial.println("WiFi Disconnected");
  }
}

// Function to check and apply OTA update
void checkForOTAUpdate() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(firmwareURL);  // Set the OTA firmware URL

    int httpResponseCode = http.GET(); // Send GET request to check for firmware
    if (httpResponseCode == 200) {
      // If there's a valid response, try to update the firmware
      int contentLength = http.getSize();
      WiFiClient *stream = http.getStreamPtr();

      if (Update.begin(contentLength)) {
        Serial.println("Updating firmware...");

        // Write the firmware to flash memory
        size_t written = Update.writeStream(*stream);

        if (written == contentLength) {
          Serial.println("Firmware update successful");
        } else {
          Serial.println("Error writing firmware");
        }

        if (Update.end()) {
          Serial.println("Firmware update complete.");
          ESP.restart(); // Restart ESP32 after successful update
        } else {
          Serial.println("Error ending firmware update");
        }
      }
    } else {
      Serial.printf("Failed to connect to firmware server, HTTP response code: %d\n", httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected, unable to check for OTA update");
  }
}
