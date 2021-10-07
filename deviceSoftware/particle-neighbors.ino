#include <JsonParserGeneratorRK.h>
#include <Adafruit_DHT_Particle.h>
#include "HttpClient.h"

#define DHTPIN D2     // what pin we're connected to
#define DHTTYPE DHT22		// DHT 11 or 22
#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)
#define FOUR_HOURS_MILLIS (4* 60 * 60 * 1000)
#define THIRTY_MINUTES_MILLIS (30 * 60 * 1000)
#define TOO_HOT 23


uint8_t shellyIPsDevicePart[] = { 40, 41, 44, 49 };

unsigned long lastTimeSync = millis();
unsigned long lastTempRead = millis();
unsigned long lastPresence = millis();
bool rollersDown = false;
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
DHT dht(DHTPIN, DHTTYPE);
JsonParser parser;

void setup() {
	Serial.begin(9600); 
	pinMode(led,OUTPUT);
    pinMode(motionSensor,INPUT);
	dht.begin();
	
	Particle.subscribe("meteodata", subscribeHandler, ALL_DEVICES);
	Particle.function("callShellysOpenClose", callShellysOpenClose);
	
    Particle.variable("roomtemperature", roomTemperature);
    Particle.variable("humidity", humidity);
    Particle.variable("rollersDown", rollersDown);
    Particle.variable("sunshine", sunshine);
    Particle.variable("outsidehumidity", outsideHumidity);
    Particle.variable("gustPeak", gustPeak);
    Particle.variable("precipitation", precipitation);
    Particle.variable("outsidetemperature", outsideTemperature);
    
    delay(1000);
    readMeasurements();
}

void loop() {
    sensorValue = digitalRead(motionSensor);
    ledToggle(sensorValue);
    
    if (millis() - lastTempRead > THIRTY_MINUTES_MILLIS) {
        readMeasurements();
    }
	
    //enable relais if humidity higher than 55% and run at least for four hours
    if (isTooHotAndSunny() && !presence && !rollersDown) {
        callShellysOpenClose("close");
    }
    
    //should not get too cold in rooms and heat with sun
    if (rollersDown && roomTemperature < 22 && sunshine > 8) {
        callShellysOpenClose("open");
    }
    
    resetPresence();
	syncTimeWithCloud();
	delay(300);
}

bool isTooHotAndSunny() {
        if (roomTemperature > TOO_HOT && sunshine > 8 && !presence) {
            return true;
        }
    return false;
}

void triggerRelais(String command) {
    presence = true; //simulate presence to not close it immediatly againif > 23 and > sunshine
    if (command == "close") {
        doRequest(command);
        rollersDown = true;
    } else if (command == "open") {
        doRequest(command);
        rollersDown = false;
    }
}

int callShellysOpenClose(String command) {
    //http client freezes if too many (>3) request fail
    errors = 0;
    for(uint8_t i=0; i<arraySize(shellyIPsDevicePart); i++)
    {
        if (errors < 2){
            IPAddress();
            IPAddress shellyIP(192, 168, 1, shellyIPsDevicePart[i]);
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
}

bool doRequest(String command) {
    request.path = "/roller/0?go="+command;
    Particle.publish("roller status", command, PRIVATE);
    http.get(request, response, headers);
    Serial.println(command);
    Serial.println(response.status);
    if (response.status != 200) {
        Particle.publish("Shelly http status", String(response.status), PRIVATE);
        errors++;
        return false;
    }
    return true;
}

void readMeasurements() {
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old'
	humidity = dht.getHumidity();
	Particle.publish("humidity", String(humidity), PRIVATE);
	logger("Humidity: %s%%", String(humidity));

	roomTemperature = dht.getTempCelcius();
	Particle.publish("roomTemperature", String(roomTemperature), PRIVATE);
	logger("Temp: %s*C", String(roomTemperature));
  
    // Check if any reads failed and exit early (to try again).
	if (isnan(humidity) || isnan(roomTemperature) ) {
		Serial.println("Failed to read from DHT sensor!");
		return;
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

int ledToggle(int value) {
    if (value) {
        digitalWrite(led,HIGH);
        presence = true;
         Serial.println("presence detected");
        return 1;
    } else {
        digitalWrite(led,LOW);
        return 0;
    }
}

void resetPresence() {
    if (millis() - lastPresence > THIRTY_MINUTES_MILLIS) {
        presence = false;
        lastPresence = millis();
        Serial.println("Auto reset presence");
    }
}