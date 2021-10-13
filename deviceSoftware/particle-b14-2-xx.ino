#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include <JsonParserGeneratorRK.h>
#include "HttpClient.h"

#define DHTPIN 7     // what pin we're connected to
#define DHTTYPE DHT22
#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)
#define FOUR_HOURS_MILLIS (4* 60 * 60 * 1000)
#define THIRTY_MINUTES_MILLIS (1 * 60 * 1000)
#define TOO_HOT 24
#define SW_VERSION 1

//Ergon Photon devices
//98:f4:ab:b9:4d:14 --> 192.168.108.112 (shelly-b14-1)
//dc:4f:22:76:ce:6c --> 192.168.108.113 (shelly-b14-2)
//3c:61:05:e5:14:69 ---> hostname:shelly-b14-3 192.168.108.114 --> B14-2.26
//8c:aa:b5:5d:7f:a1 ---> hostname:shelly-b14-4 192.168.108.115 -->  B14-2.25
//IMPORTANT - CHECK IP Part 192.168.xx below!

uint8_t shellyIPsDevicePart[] = { 112, 113 };

unsigned long lastTimeSync = millis();
unsigned long lastTempRead = millis();
bool windowOpen = false;
double roomTemperature = 0;
double outsideTemperature = 0;
int sunshine = 0;
double humidity = 0;
double outsideHumidity = 0;
double precipitation = 0;
double gustPeak = 0;
int led = D0;
int motionSensor = D1;
int sensorValue;
bool presence = false;
//http client freezes if too many (>3) request fail
uint8_t errors = 0;

http_header_t headers[] = {
    //  { "Content-Type", "application/json" },
    //  { "Accept" , "application/json" },
    { "Accept" , "*/*"},
    { NULL, NULL } // NOTE: Always terminate headers will NULL
};


HttpClient http;
http_request_t request;
http_response_t response;
DHT_Unified dht(DHTPIN, DHTTYPE);
JsonParser parser;

void setup() {
	dht.begin();
	Serial.begin(9600); 
	Particle.subscribe("meteodata", subscribeHandler, ALL_DEVICES);
	Particle.function("callShellysOpenClose", callShellysOpenClose);
	
    Particle.variable("windowOpen", windowOpen);
    Particle.variable("roomtemperature", roomTemperature);
    Particle.variable("humidity", humidity);
    Particle.variable("sunshine", sunshine);
    Particle.variable("outsidehumidity", outsideHumidity);
    Particle.variable("gustPeak", gustPeak);
    Particle.variable("precipitation", precipitation);
    Particle.variable("outsidetemperature", outsideTemperature);
    Particle.variable("sw_version", SW_VERSION);

    delay(2000);
    readMeasurements();
}

void loop() {
    if (millis() - lastTempRead > THIRTY_MINUTES_MILLIS) {
        readMeasurements();
    }
	
    //enable relais
    if (isTooHotAndSummer()) {
        callShellysOpenClose("open");
    }
    
    //close if getting cold
    if (windowOpen && roomTemperature < 21) {
        callShellysOpenClose("close");
    }
    
    //close if too hot outside
    if (windowOpen && outsideTemperature > roomTemperature) {
        callShellysOpenClose("close");
    }
    
    //close if wind gusts are too high
    //TODO
    
	syncTimeWithCloud();
	delay(THIRTY_MINUTES_MILLIS);
}

bool isTooHotAndSummer() {
    //auto cooling between May (5) and September (9)
    if (Time.month() >= 5 && Time.month() <= 9) {
        if (roomTemperature > TOO_HOT && outsideTemperature < roomTemperature && !windowOpen) {
            return true;
        }
    }
    return false;
}

void triggerRelais(String command) {
    presence = true; //simulate presence to not close it immediatly againif > 23 and > sunshine
    if (command == "close") {
        doRequest(command);
        windowOpen = false;
    } else if (command == "open") {
        doRequest(command);
        windowOpen = true;
    }
}

int callShellysOpenClose(String command) {
    //http client freezes if too many (>3) request fail
    errors = 0;
    for(uint8_t i=0; i<arraySize(shellyIPsDevicePart); i++)
    {
        if (errors < 2) {
            IPAddress();
            IPAddress shellyIP(192, 168, 108, shellyIPsDevicePart[i]);
            Serial.print("Calling shelly with IP: ");
            String ipStr = String(shellyIP[0])+"."+String(shellyIP[1])+"."+String(shellyIP[2])+"."+String(shellyIP[3]);
            Serial.println(ipStr);
            request.ip = shellyIP;
            request.port = 80;
            triggerRelais(command);
        } else {
            Serial.println("Stopping request, too many errors!");
            return -1;
        }
    }
    if (errors < 2) {
        return 1;
    }
    return 0;
}

bool doRequest(String command) {
    request.path = "/roller/0?go="+command;
    Particle.publish("roller status", command);
    http.get(request, response, headers);
    Serial.println(command);
    Serial.println(response.status);
    if (response.status != 200) {
        Particle.publish("Shelly http status", String(response.status));
        errors++;
        return false;
    }
    return true;
}

void readMeasurements() {
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old'
    
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
        Serial.println(F("Error reading temperature!"));
    }
    else {
        roomTemperature = event.temperature;
        Particle.publish("roomTemperature", String::format("%.1f", event.temperature));
        logger("Temp: %s*C", String::format("%.1f", event.temperature));
    }
    // Get humidity event and print its value.
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
        Serial.println(F("Error reading humidity!"));
    }
    else {
        humidity = event.relative_humidity;
    	Particle.publish("humidity", String::format("%.1f", event.relative_humidity));
    	logger("Humidity: %s%%", String::format("%.1f", event.relative_humidity));
    }
    
	lastTempRead = millis();
}

void syncTimeWithCloud() {
  if (millis() - lastTimeSync > ONE_DAY_MILLIS) {
    // Request time synchronization once a day from the Particle Device Cloud
    Particle.syncTime();
    lastTimeSync = millis();
  }
}

void logger(const char* message, const char* value) {
    Serial.printf(message, value);
    Serial.println();
}

void subscribeHandler(const char *event, const char *data) {
  if (data && String(event).equals("meteodata")) {
    Serial.println(data);
    parser.clear();
    parser.addString(data);
    parser.parse();
    parser.getOuterValueByKey("sunshine", sunshine);
    parser.getOuterValueByKey("outsidetemperature", outsideTemperature);
    parser.getOuterValueByKey("outsidehumidity", outsideHumidity);
    parser.getOuterValueByKey("gustPeak", gustPeak);
    parser.getOuterValueByKey("precipitation", precipitation);
  }
}

