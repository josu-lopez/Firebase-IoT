#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>

// Firebase helpers
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// Timestamp
String getLocalTimeISO();
String getLocalTimeUNIX();

// NTP
#define NTP_SERVER "YOUR_NTP_SERVER"
#define NTP_GMT_OFFSET_SEC 0
#define NTP_DAYLIGHT_OFFSET_SEC 0

// WiFi credentials
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASSWORD "YOUR_PASSWORD"

// Firebase API key
#define API_KEY "YOUR_API_KEY"

// Firebase RTDB URL
#define DATABASE_URL "YOUR_RTDB_URL"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Global variables
unsigned long t1_sendDataPrevMillis = 0;
unsigned long t2_sendDataPrevMillis = 0;
bool signupOK = false;

// GPIO
#define LED_OUTPUT 32
#define LED_RED 33
#define LED_GREEN 25
#define LED_BLUE 26
#define LED_CONFORT 23

// DHT
#define DHTPIN 22
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

void setup()
{
  // Initialize Serial
  Serial.begin(115200);

  // Initialize GPIO
  pinMode(LED_OUTPUT, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_CONFORT, OUTPUT);
  digitalWrite(LED_OUTPUT, HIGH);
  analogWrite(LED_RED, 0);
  analogWrite(LED_GREEN, 0);
  analogWrite(LED_BLUE, 0);
  digitalWrite(LED_CONFORT, LOW);

  // Initialize DHT
  dht.begin();

  // Initialize WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(333);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Initialize NTP
  configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC, NTP_SERVER);

  // Configure Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Sign up
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("Firebase signup ok!");
    signupOK = true;
  }
  else
  {
    Serial.println("Firebase signup failed!");
  }

  // Assign callback function for token generation task
  config.token_status_callback = tokenStatusCallback;

  // Initialize Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop ()
{
  if (Firebase.ready() && signupOK)
  {
    // Task 1: read every 3 seconds
    if (millis() - t1_sendDataPrevMillis > 3000 || t1_sendDataPrevMillis == 0)
  {
      // Update execution time
      t1_sendDataPrevMillis = millis();

      // Read output LED
      bool ledOutput = false;
      Serial.print("actuador/led: ");
      if (Firebase.RTDB.getBool(&fbdo, "actuador/led", &ledOutput))
    {
        Serial.println(ledOutput);
        digitalWrite(LED_OUTPUT, !ledOutput);
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("  REASON: " + fbdo.errorReason());
    }

      // Read red LED
      int ledRed = 0;
      Serial.print("actuador/rgb/red: ");
      if (Firebase.RTDB.getInt(&fbdo, "actuador/rgb/red", &ledRed))
    {
        Serial.println(ledRed);
        analogWrite(LED_RED, ledRed);
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("  REASON: " + fbdo.errorReason());
    }

      // Read green LED
      int ledGreen = 0;
      Serial.print("actuador/rgb/green: ");
      if (Firebase.RTDB.getInt(&fbdo, "actuador/rgb/green", &ledGreen))
    {
        Serial.println(ledGreen);
        analogWrite(LED_GREEN, ledGreen);
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("  REASON: " + fbdo.errorReason());
    }

      // Read blue LED
      int ledBlue = 0;
      Serial.print("actuador/rgb/blue: ");
      if (Firebase.RTDB.getInt(&fbdo, "actuador/rgb/blue", &ledBlue))
    {
        Serial.println(ledBlue);
        analogWrite(LED_BLUE, ledBlue);
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("  REASON: " + fbdo.errorReason());
    }
    }

    // Task 2: write every minute
    if (millis() - t2_sendDataPrevMillis > 60000 || t2_sendDataPrevMillis == 0)
    {
      // Update execution time
      t2_sendDataPrevMillis = millis();

      // Obtain current timestamp
      String timestampUNIX = getLocalTimeUNIX();
      String timestampISO = getLocalTimeISO();

      float humidity = dht.readHumidity();
      float temperature = dht.readTemperature();

      // Calculate confort
      String confort = "";
      if (temperature >= 21.0f && temperature <= 25.0f && humidity >= 40.0f && humidity <= 60.0f)
      {
        confort = "bueno";
        digitalWrite(LED_CONFORT, HIGH);
      }
      else
      {
        confort = "malo";
        digitalWrite(LED_CONFORT, LOW);
      }
      
      // Write confort
      Serial.print("Confort write ");
      if (Firebase.RTDB.setString(&fbdo, "sensor/" + timestampUNIX + "/confort", confort))
      {
        Serial.println("OK");
        Serial.println("  Path: " + fbdo.dataPath());
        Serial.println("  Type: " + fbdo.dataType());
        Serial.print("  Value: ");
        Serial.println(fbdo.stringData());
    }
    else
    {
      Serial.println("FAILED");
        Serial.println("  Reason: " + fbdo.errorReason());
    }

      // Write timestamp
      Serial.print("Timestamp write ");
      if (Firebase.RTDB.setString(&fbdo, "sensor/" + timestampUNIX + "/timestamp", timestampISO))
    {
        Serial.println("OK");
        Serial.println("  Path: " + fbdo.dataPath());
        Serial.println("  Type: " + fbdo.dataType());
        Serial.print("  Value: ");
        Serial.println(fbdo.stringData());
    }
    else
    {
      Serial.println("FAILED");
        Serial.println("  Reason: " + fbdo.errorReason());
    }

      // Write humidity
      Serial.print("Humidity write ");
      if (Firebase.RTDB.setFloat(&fbdo, "sensor/" + timestampUNIX + "/humedad", humidity))
    {
        Serial.println("OK");
        Serial.println("  Path: " + fbdo.dataPath());
        Serial.println("  Type: " + fbdo.dataType());
        Serial.print("  Value: ");
        Serial.println(fbdo.floatData());
    }
    else
    {
      Serial.println("FAILED");
        Serial.println("  Reason: " + fbdo.errorReason());
    }

      // Write temperature
      Serial.print("Temperature write ");
      if (Firebase.RTDB.setFloat(&fbdo, "sensor/" + timestampUNIX + "/temperatura", temperature))
    {
        Serial.println("OK");
        Serial.println("  Path: " + fbdo.dataPath());
        Serial.println("  Type: " + fbdo.dataType());
        Serial.print("  Value: ");
        Serial.println(fbdo.floatData());
    }
    else
    {
      Serial.println("FAILED");
        Serial.println("  Reason: " + fbdo.errorReason());
    }
    }
  }
}

String getLocalTimeISO()
{
  struct tm timeinfo;
  char buffer[20];

  // Get local time
  if(!getLocalTime(&timeinfo))
  {
    return "NTP Error!";
  }

  // Obtain ISO 8601 timestamp
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &timeinfo);
  return String(buffer);
}

String getLocalTimeUNIX()
{
  struct tm timeinfo;

  // Get local time
  if(!getLocalTime(&timeinfo))
  {
    return "NTP Error!";
  }

  // Obtain UNIX timestamp
  return String(mktime(&timeinfo));
}
