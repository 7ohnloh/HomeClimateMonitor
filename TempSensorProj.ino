// Required Libraries
#include <WiFi.h>                // ESP32 WiFi
#include <FirebaseESP32.h>      // Firebase client for ESP32
#include <Adafruit_Sensor.h>    // Base sensor library
#include <DHT.h>                // DHT11 library

// -----------------------------
// 🔐 Firebase Configuration
// -----------------------------
#define API_KEY "your_firebase_api_key"
#define USER_EMAIL "your@email.com"
#define USER_PASSWORD "your_password"
#define DATABASE_URL "https://your-project.firebaseio.com"

// Firebase Objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// -----------------------------
// 📶 WiFi Credentials
// -----------------------------
#define WIFI_SSID "your_wifi"
#define WIFI_PASSWORD "your_password"

// -----------------------------
// 🌡️ DHT11 Sensor Configuration
// -----------------------------
#define DHTPIN 4          // Data pin connected to DHT11
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE); // Create DHT object

// -----------------------------
// 🔢 5641AS 7-Segment Display
// -----------------------------
// Segment order: A, B, C, D, E, F, G, DP
const int segmentPins[] = {5, 7, 8, 9, 10, 0, 1, 2};

// Digit control pins: Common anode pins for each digit
const int digitPins[] = {3, 18, 19};  // D1 = Tens, D2 = Ones, D3 = Decimal

// Segment pattern for digits 0–9 (common anode logic)
const byte digitPatterns[10] = {
    0b11000000, // 0
    0b11111001, // 1
    0b10100100, // 2
    0b10110000, // 3
    0b10011001, // 4
    0b10010010, // 5
    0b10000010, // 6
    0b11111000, // 7
    0b10000000, // 8
    0b10011000  // 9
};

// Variables for display digits (d1 = tens, d2 = ones, d3 = tenths)
int d1 = 0, d2 = 0, d3 = 0;

// Timing control for Firebase update
unsigned long lastUpdateTime = 0;
const long updateInterval = 3000; // 3 seconds

// -----------------------------
// 🔧 Utility: Turn off all digits
// -----------------------------
void clearDigits() {
    for (int i = 0; i < 3; i++) {
        digitalWrite(digitPins[i], HIGH); // HIGH = OFF (common anode)
    }
}

// -----------------------------
// 🔧 Display one digit using multiplexing
// -----------------------------
void displayDigit(int digit, int num, bool showDecimal) {
    clearDigits(); // Turn off all digits first
    digitalWrite(digitPins[digit], LOW); // Enable current digit

    // Set segments for current number
    for (int i = 0; i < 8; i++) {
        bool segmentState = bitRead(digitPatterns[num], i);
        // If we want decimal point, turn it ON (LOW for common anode)
        if (showDecimal && i == 7) segmentState = LOW;
        digitalWrite(segmentPins[i], segmentState);
    }

    delay(2); // Small delay for persistence of vision
    digitalWrite(digitPins[digit], HIGH); // Turn digit off after display
}

// -----------------------------
// 🚀 Setup Function
// -----------------------------
void setup() {
    Serial.begin(115200);      // Start serial monitor
    dht.begin();               // Initialize DHT11

    // Initialize segment and digit pins
    for (int i = 0; i < 8; i++) pinMode(segmentPins[i], OUTPUT);
    for (int i = 0; i < 3; i++) pinMode(digitPins[i], OUTPUT);

    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi!");

    // Firebase config setup
    config.api_key = API_KEY;
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;
    config.database_url = DATABASE_URL;

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    Serial.println("Firebase Initialized!");
}

// -----------------------------
// 🔁 Loop Function
// -----------------------------
void loop() {
    unsigned long currentTime = millis();

    // Only read and send data every 3 seconds
    if (currentTime - lastUpdateTime >= updateInterval) {
        lastUpdateTime = currentTime;

        // Read temperature from DHT11
        float temperature = dht.readTemperature();
        // Optionally: float humidity = dht.readHumidity();

        float humidity = dht.readHumidity(); // <-- Add this line


        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("❌ Failed to read from DHT sensor!");
            return;
        }

        Serial.print("📡 Temperature: ");
        Serial.println(temperature, 2);



        // Convert temperature to integer for display (e.g., 23.4 → 234)
        int tempInt = temperature * 10;
        d1 = (tempInt / 100) % 10;  // Tens
        d2 = (tempInt / 10) % 10;   // Ones
        d3 = tempInt % 10;          // Tenths

        // Push to Firebase Realtime DB
        if (Firebase.setFloat(fbdo, "/temperature", temperature)) {
            Serial.println("✅ Temperature sent to Firebase!");
        } else {
            Serial.print("❌ Firebase Error: ");
            Serial.println(fbdo.errorReason());
        }

        // Read Humidity from DHT11

        Serial.print("📡 Humidity: ");
        Serial.println(humidity);
        
        if (Firebase.setFloat(fbdo, "/humidity", humidity)) {
            Serial.println("✅ Humidity sent to Firebase!");
        }
        
    }

    // Continuously refresh the 7-segment display using multiplexing
    for (int i = 0; i < 100; i++) {  // Repeat to keep display stable
        displayDigit(0, d1, false);  // Tens
        displayDigit(1, d2, true);   // Ones + Decimal point
        displayDigit(2, d3, false);  // Tenths
    }
}
