#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include<WiFi.h>
#include<ThingSpeak.h>

//Pin Definitions

//DHT22
#define DHT_PIN 4
#define DHT_TYPE DHT22

//LDR
#define LDR_PIN 34

//Soil-Moisture sensor
#define SOIL_PIN 32

//Servo
#define SERVO_PIN 18

//OLED
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RESET 23

//Grow LEDs
#define LED1_PIN 25
#define LED2_PIN 26

//Push button
#define OVERRIDE_PIN 27

//Relay
#define RELAY_PIN 33

//Configure WiFi and ThingSpeak
const char* WIFI_SSID = "WIFI_NAME";
const char* WIFI_PASSWORD = "WIFI_PASSWORD!";

unsigned long THINGSPEAK_CHANNEL_ID = 3508213;
const char* THINGSPEAK_WRITE_API_KEY = "TA1LNIUN9PSWKHEA";

WiFiClient thingSpeakClient;

const unsigned long THINGSPEAK_INTERVAL = 20000;
unsigned long lastThingSpeakUpload = 0;

//OLED Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

//Sensors and Actuators
DHT dht(DHT_PIN, DHT_TYPE);
Servo ventServo;

//Temperature settings
const float TEMP_OPEN = 30.0; //vent opens at 30 degrees C or more
const float TEMP_CLOSE = 28.0; //vent closes at 28 degrees C or less

//Light settings
const int LIGHT_THRESHOLD = 1800;

//Servo positions
const int VENT_CLOSED = 60;
const int VENT_OPEN = 120;

//Soil moisture settings
const int SOIL_DRY = 3020;
const int SOIL_WET = 2600;

//Irrigation control
const int IRRIGATION_ON = 30; //Start irrigation at or below 30%
const int IRRIGATION_OFF = 40; //stop irrigation at or above 40%
const unsigned long MAX_PUMP_RUNTIME = 1000; //max continuous pump operation is 1 sec

//System state variables
bool ventIsOpen = false;
bool growLightsOn = false;
bool pumpIsOn = false;
bool pumpTimeoutLockout = false;
unsigned long pumpStartTime = 0;

//Manual Override Toggles
//false = autonomous mode, true = manual override mode
bool manualOverrideActive = false;

bool buttonWasPressed = false;

//Vent functions
/**
 * @brief Opens the greenhouse ventilation panel
 * Moves servo to the open position (120 degrees)
 * @return void
 */

 void openVent(){
  ventServo.write(VENT_OPEN);
  ventIsOpen = true;
 }

 /**
 * @brief Close the greenhouse ventilaton panel
 * Moves servo to closed position (60 degrees)
 *@return void
*/

void closeVent(){
  ventServo.write(VENT_CLOSED);
  ventIsOpen = false;
}

//Grow light function
/**
* @brief Controls both grow light LEDs
* @param state true switches LEDs on, false switches LEDs off
*@return void
*/

void setGrowLights(bool state){
  digitalWrite(
    LED1_PIN,
    state ? HIGH : LOW
  );

  digitalWrite(
    LED2_PIN,
    state ? HIGH : LOW
  );

  growLightsOn = state;
}

//Irrigation functions
/**
* @brief starts the irrigation pump
* the relay was experimentally found to have an active-HIGH. The
* function also records the pump start time for runtime safety monitoring.
* @return void
*/

void startPump(){
  digitalWrite(RELAY_PIN, HIGH);

  pumpIsOn = true;
  pumpStartTime = millis();
}

/**
* @brief Stops the irrigation pump
* @return void
*/

void stopPump(){
  digitalWrite(RELAY_PIN, LOW);
  pumpIsOn = false;
}

/**
* @brief Attempts to connect the ESP32 to the configured WiFi network
*Connection attempt is time limited, so failure to connect does not
*prevent the nursery from operating.
* @return void
*/

void connectToWiFi(){
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long connectionStart = millis();

  while(
    WiFi.status() != WL_CONNECTED &&
    millis() - connectionStart < 10000
  ){
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED){
    Serial.println("WiFi connected.");

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    Serial.print("Signal strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  }

  else{
    Serial.println(
      "WiFi unavailable - "
      "local control continuing."
    );
  }
}

/**
* @brief uploads environmental readings and actuator states to ThingSpeak
* Field allocation: 
* Field 1 = Temperature (C)
* Field 2 = Humidity (%)
* Field 3 = Light level (ADC)
* Field 4 = Soil moisture (%)
* Field 5 = Vent state
* Field 6 = Grow light state
* Field 7 = Pump state 
* Field 8 = Operating mode
*
* Binary states use: 
* 0 = inactive / closed
* 1 = active / open
* 
* @param temperature Current DHT22 temperature reading
* @param humidity Current DHT22 humidity reading
* @param lightLevel Current LDR ADC reading
* @param soilMoisture Calibrated soil moisture percentage
*
*@return void
*/

void uploadToThingSpeak(
  float temperature,
  float humidity,
  int lightLevel,
  int soilMoisture
){
  if(
    millis() - lastThingSpeakUpload < THINGSPEAK_INTERVAL
  ){
    return;
  }

  lastThingSpeakUpload = millis();

  if (WiFi.status() != WL_CONNECTED){
    Serial.println(
      "ThingSpeak skipped - WiFi unavailable."
    );
    return;
  }

  ThingSpeak.setField(1, temperature);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, lightLevel);
  ThingSpeak.setField(4, soilMoisture);

  ThingSpeak.setField(
    5,
    ventIsOpen ? 1:0
  );

  ThingSpeak.setField(
    6,
    growLightsOn ? 1:0
  );

  ThingSpeak.setField(
    7,
    pumpIsOn ? 1:0
  );

  ThingSpeak.setField(
    8,
    manualOverrideActive ? 1:0
  );

  int response = ThingSpeak.writeFields(
    THINGSPEAK_CHANNEL_ID,
    THINGSPEAK_WRITE_API_KEY
  );

  if(response == 200){
    Serial.println("ThingSpeak upload successful.");
  }
  else{
    Serial.print("ThingSpeak upload failed. Error: ");
    Serial.println(response);
  }
}

//Setup
/**
* @brief Initializes all sensors, actuators and interfaces
* Establishes safe startup states before autonomous environmental control begings
* @return void*/

void setup(){
  //Serial communication

  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("================================");
  Serial.println("MICRO-CLIMATE NURSERY");
  Serial.println("System starting....");
  Serial.println("================================");

  //DHT22
  dht.begin();

  //Grow LEDs
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);

  //Safe startup state
  setGrowLights(false);

  //Relay
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  pumpIsOn = false;

  //Manual override button (push button)
  pinMode(OVERRIDE_PIN, INPUT_PULLUP);

  //OLED I2C
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println("OLED INITIALIZATION FAILED");

    while(true){

    }
  }

  //Startup msg
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);

  display.println("MICRO-CLIMATE");
  display.println("NURSERY");
  display.println();
  display.println("System starting...");

  display.display();
  delay(2000);

  //Servo
  ventServo.setPeriodHertz(50);

  ventServo.attach(
    SERVO_PIN,
    500,
    2400
  );

  closeVent();
  delay(500);

  Serial.println("Servo initialized on GPIO18.");
  Serial.println("Initial vent position: CLOSED");
  Serial.println("System initialized successfully");

  //WiFi and ThingSpeak
  connectToWiFi();
  ThingSpeak.begin(thingSpeakClient);

  Serial.println("ThingSpeak cloud monitoring initialized.");


}

//main loop
/**
* @brief executes continuous environmental monitoring and automatic climate control logic.
* Control priority: 
* 1. Critical DHT22 fault
* 2. Manual override
* 3. Autonomous environmental control
* @return void
*/

void loop(){
  //Read all sensor values
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  int lightLevel = analogRead(LDR_PIN);
  int soilRaw = analogRead(SOIL_PIN);

  //convert soil ADC values into percentage
  int soilMoisture = map(
    soilRaw,
    SOIL_DRY,
    SOIL_WET,
    0,
    100
  );

  soilMoisture = constrain(
    soilMoisture,
    0,
    100
  );

  //PRIORITY 1: CRITICAL DHT22 SENSOR FAULT

  bool dhtFault = 
    isnan(temperature) ||
    isnan(humidity) ||
    temperature < -40 ||
    temperature > 80 ||
    humidity < 0 ||
    humidity > 100;

  if(dhtFault){
    openVent();
    setGrowLights(false);
    stopPump();

    display.clearDisplay();
    display.setCursor(0,0);
    display.println("!! SENSOR FAULT !!");
    display.println();
    display.println("DHT22 ERROR");
    display.println();
    display.println("Vent: OPEN");
    display.println("LEDs: OFF");
    display.println("Pump: OFF");

    display.display();

    Serial.println("DHT22 FAULT - SAFE STATE ACTIVE");
    delay(1000);
    return;
  }

  //PRIORITY 2: MANUAL OVERRIDE BUTTON
  bool buttonPressed = digitalRead(OVERRIDE_PIN) == LOW;
  if(buttonPressed && !buttonWasPressed){
    manualOverrideActive = !manualOverrideActive;
    buttonWasPressed = true;

    if(manualOverrideActive){
      Serial.println("==============================");
      Serial.println("MANUAL OVERRIDE ENABLED");
      Serial.println("==============================");
    }
    else{
      Serial.println("==============================");
      Serial.println("MANUAL OVERRIDE DISABLED");
      Serial.println("AUTONOMOUS MODE RESUMED");
      Serial.println("==============================");
    }
    delay(50);
  }

  if(!buttonPressed){
    buttonWasPressed = false;
  }

  if(manualOverrideActive){
    openVent();
    setGrowLights(false);
    stopPump();

    display.clearDisplay();
    display.setCursor(0,0);

    display.println("MANUAL OVERRIDE");
    display.println();
    display.println("AUTOMATION");
    display.println("SUSPENDED");
    display.println();
    display.println("Vent: OPEN");
    display.println("LEDs: OFF");
    display.println("Pump: OFF");

    display.display();

    Serial.println("MANUAL OVERRIDE ACTIVE - "
    "VENT OPEN / LEDs OFF / PUMP OFF");

      uploadToThingSpeak(
    temperature,
    humidity,
    lightLevel,
    soilMoisture
  );

  delay(100);
  return;

  delay(100);
  return;
}

//PRIORITY 3: AUTONOMOUS CONTROL

if(temperature >= TEMP_OPEN){
  openVent();
}
else if(temperature <= TEMP_CLOSE){
  closeVent();
}

if(lightLevel < LIGHT_THRESHOLD){
  setGrowLights(true);
}
else{
  setGrowLights(false);
}

if(pumpIsOn){
  if(millis() - pumpStartTime >= MAX_PUMP_RUNTIME){
    stopPump();

    pumpTimeoutLockout = true;

    Serial.println(
    "PUMP SAFETY TIMEOUT - "
    "IRRIGATION LOCKED"
    );
  }
}

//SOIL MOISTURE IRRIGATION CONTROL
if(soilMoisture >= IRRIGATION_OFF){
  if(pumpIsOn){
    stopPump();
  }

  pumpTimeoutLockout = false;
}
else if(
  soilMoisture <= IRRIGATION_ON && !pumpIsOn && !pumpTimeoutLockout
){
  startPump();
}

//OLED NORMAL OPERATION DISPLAY
display.clearDisplay();
display.setCursor(0,0);

display.print("Temp: ");
display.print(temperature, 1);
display.println(" C");

display.print("Hum: ");
display.print(humidity, 1);
display.println(" %");

display.print("Light: ");
display.println(lightLevel);

display.print("Soil: ");
display.print(soilMoisture);
display.print(" %");

display.print("Vent: ");
display.println(
  ventIsOpen 
  ? "OPEN" 
  : "CLOSED"
);

display.print("LEDs: ");
display.println(
  growLightsOn
  ? "ON"
  : "OFF"
);

display.print("Pump: ");
if(pumpTimeoutLockout){
  display.println("LOCKED");
}
else{
  display.println(
    pumpIsOn
    ? "ON"
    : "OFF"
  );
}

display.display();

//SERIAL MONITOR OUTPUT
Serial.print("Temp: ");
Serial.print(temperature, 1);
Serial.println(" C");

Serial.print(" | Humidity: ");
Serial.print(humidity, 1);
Serial.print("%");

Serial.print(" | Light: ");
Serial.println(lightLevel);

Serial.print(" | Soil Raw: ");
Serial.print(soilRaw);

Serial.print(" | Soil: ");
Serial.print(soilMoisture);
Serial.print("%");

Serial.print(" | Vent: ");
Serial.print(
  ventIsOpen 
  ? "OPEN" 
  : "CLOSED"
);

Serial.print(" | LEDs: ");
Serial.print(
  growLightsOn
  ? "ON"
  : "OFF"
);

Serial.print(" | Pump: ");
if(pumpTimeoutLockout){
  Serial.println("TIMEOUT LOCKOUT");
}
else{
  Serial.println(
    pumpIsOn
    ? "ON"
    : "OFF"
  );
}

//ThingSpeak cloud monitoring
uploadToThingSpeak(
  temperature,
  humidity,
  lightLevel,
  soilMoisture
);

delay(500);

}
