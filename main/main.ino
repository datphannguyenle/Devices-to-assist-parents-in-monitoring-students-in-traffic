#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

#define SIM800_RX_PIN 19
#define SIM800_TX_PIN 18

SoftwareSerial sim800l(SIM800_TX_PIN, SIM800_RX_PIN);

const char* phoneNumber = "0973297567";

static const int RXPin = 17, TXPin = 16;
static const uint32_t GPSBaud = 9600;

TinyGPSPlus gps;

SoftwareSerial ss(RXPin, TXPin);

String oldlat = "";
String oldlng = "";
String newlat = "";
String newlng = "";
float oldspeed = 0;
float newspeed = 0;
int thresholdSpeed = 70;
bool nearSchool = false;
bool nearHome = false;
static const double SCHOOL_LAT = 11.08024095444668, SCHOOL_LON = 107.69359356483338;
static const double HOME_LAT = 11.081387088090779, HOME_LON = 107.69317036965914;

int sensorPin = 21;
int sensorValue;
int lastTiltState = HIGH;  // the previous reading from the tilt sensor

// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers

void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;

  sim800l.begin(9600);
  ss.begin(GPSBaud);

  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);
}

void loop() {
  sensorValue = digitalRead(sensorPin);
  if (sensorValue == lastTiltState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    lastTiltState = sensorValue;
  }

  Serial.println(sensorValue);
  if (sensorValue == 1) {
    String message = "Canh bao xay ra tai nan, hoc sinh can ho tro. Vui long den ngay | Vi tri hien tai: https://www.google.com/maps?q=" + newlat + "," + newlng + " .";
    sendSMS(phoneNumber, message);
    smartDelay(0);
  }
  smartDelay(500);

  oldlat = newlat;
  oldlng = newlng;
  newlat = String(gps.location.lat(), 15);
  Serial.println(newlat);
  smartDelay(0);
  newlng = String(gps.location.lng(), 15);
  Serial.println(newlng);
  smartDelay(0);

  oldspeed = newspeed;
  newspeed = gps.speed.kmph();
  Serial.println(newspeed);
  smartDelay(0);

  if (gps.speed.kmph() > thresholdSpeed) {
    String message = "Canh bao toc do da vuot qua 70 km/h | Vi tri hien tai: https://www.google.com/maps?q=" + newlat + "," + newlng + " .";
    sendSMS(phoneNumber, message);
    smartDelay(0);
  }

  if (sim800l.available() && sim800l.find("CALL")) {
    String message = "Vi tri hien tai: https://www.google.com/maps?q=" + newlat + "," + newlng + " | Toc do hien tai: " + String(newspeed) + " km/h.";
    sendSMS(phoneNumber, message);
    smartDelay(0);
  }

  if (gps.location.isValid()) {
    double distanceToSchool = TinyGPSPlus::distanceBetween(gps.location.lat(), gps.location.lng(), SCHOOL_LAT, SCHOOL_LON);

    // Check if you are 500m from the school
    if (distanceToSchool / 1000 <= 0.5) {
      // If not already near the school, announce that you have arrived at the school
      if (!nearSchool) {
        sendSMS(phoneNumber, "Hoc sinh da den truong an toan.");
        Serial.println("Hoc sinh da den truong an toan.");
        nearSchool = true;  // Set the flag to true to indicate nearness to the school
      }
    } else {
      // If already near the school, announce that you are leaving the school
      if (nearSchool) {
        sendSMS(phoneNumber, "Hoc sinh da roi khoi truong.");
        Serial.println("Hoc sinh da roi khoi truong.");
        nearSchool = false;  // Set the flag to false to indicate leaving the school
      }
    }

    double distanceToHome = TinyGPSPlus::distanceBetween(gps.location.lat(), gps.location.lng(), HOME_LAT, HOME_LON);

    // Check if you are 500m from the home
    if (distanceToHome / 1000 <= 0.5) {
      // If not already near the home, announce that you have arrived at the home
      if (!nearHome) {
        sendSMS(phoneNumber, "Hoc sinh da ve nha an toan.");
        Serial.println("Hoc sinh da ve nha an toan.");
        nearHome = true;  // Set the flag to true to indicate nearness to the home
      }
    } else {
      // If already near the home, announce that you are leaving the home
      if (nearHome) {
        sendSMS(phoneNumber, "Hoc sinh da roi khoi nha.");
        Serial.println("Hoc sinh da roi khoi nha.");
        nearHome = false;  // Set the flag to false to indicate leaving the home
      }
    }
  }

  smartDelay(1000);

  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("No GPS data received: check wiring"));
}

static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

void sendSMS(String phoneNumber, String message) {
  sim800l.println("AT+CMGF=1");
  smartDelay(1000);

  sim800l.print("AT+CMGS=\"");
  sim800l.print(phoneNumber);
  sim800l.println("\"");
  smartDelay(1000);

  sim800l.print(message);
  smartDelay(1000);

  sim800l.println((char)26);
  smartDelay(1000);
}