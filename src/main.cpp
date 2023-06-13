#include <WiFi.h>
#include <HTTPClient.h>
#include <RTClib.h>
#include <ThingSpeak.h>
#include <Adafruit_AHTX0.h>

#define PHOTORESISTOR_PIN 39
#define MOISTURE_PIN 38

int get_average_moisture();
void send_email(float temperature, float humidity, int moisture, int light, String warning);
int get_light();

char ssid[] = "UCInet Mobile Access"; // your network SSID (name)
const char *password = "\0";          // your network password
String httpsRequest = "/hooks/catch/15608594/3hubi9u/";

const char *host = "hooks.zapier.com";
// WiFiClient ZapierClient;
WiFiClient ThingSpeakClient;

unsigned long myChannelNumber = 2183874;
const char *myWriteAPIKey = "W0PFNAJWPWD2E6HG";

RTC_PCF8523 rtc; // create RTC object

Adafruit_AHTX0 aht;

int threshold = 800;
unsigned long triggerInterval = 12UL * 60UL * 60UL * 1000UL; // 12 hours in milliseconds
unsigned long lastTriggerTime = 0;

bool alert_already_sent = false;
bool already_sent_to_cloud = true;

void setup()
{
  Serial.begin(9600);

  pinMode(PHOTORESISTOR_PIN, INPUT); // set light sensor pin as an input

  pinMode(MOISTURE_PIN, INPUT); // set the moisture pin as an input

  while (!Serial)
    ;
  delay(2000);

  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");

  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  ThingSpeak.begin(ThingSpeakClient);

  if (!aht.begin())
  {
    Serial.println("Could not find AHT? Check wiring");
    while (1)
      delay(10);
  }
  Serial.println("AHT10 or AHT20 found");
}

void loop()
{
  String warning = "";

  if (get_average_moisture() < threshold && !alert_already_sent)
  {
    warning = "Warning your plant needs water !";
    warning.replace(" ", "%20");

    // get temp and humidity
    sensors_event_t humidityEvent, tempEvent;
    aht.getEvent(&humidityEvent, &tempEvent);

    float temperature = tempEvent.temperature;
    float humidity = humidityEvent.relative_humidity;

    Serial.println("Sending Email...");

    send_email(temperature, humidity, get_average_moisture(), get_light(), warning);
    alert_already_sent = true;
    already_sent_to_cloud = false;

    Serial.println("Email Sent!");
  }

  unsigned long currentTime = millis();
  if (currentTime - lastTriggerTime >= triggerInterval)
  {
    Serial.println("Triggered");

    // get temp and humidity
    sensors_event_t humidityEvent, tempEvent;
    aht.getEvent(&humidityEvent, &tempEvent);

    float temperature = tempEvent.temperature;
    float humidity = humidityEvent.relative_humidity;

    Serial.println("Sending Email...");

    send_email(temperature, humidity, get_average_moisture(), get_light(), warning);
    alert_already_sent = false;

    Serial.println("Email Sent!");

    lastTriggerTime = currentTime;
  }

  if (!already_sent_to_cloud)
  {
    // get temp and humidity
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);

    ThingSpeak.setField(1, get_light());
    ThingSpeak.setField(2, temp.temperature);
    ThingSpeak.setField(3, humidity.relative_humidity);
    ThingSpeak.setField(4, get_average_moisture());
    ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    already_sent_to_cloud = true;
    Serial.println("Message sent to cloud");
  }
}

int get_average_moisture()
{
  int tempValue = 0;
  for (int a = 0; a < 10; a++)
  {
    tempValue += analogRead(MOISTURE_PIN);
    delay(10);
  }
  return tempValue / 10;
}

int get_light()
{
  int light_value = analogRead(PHOTORESISTOR_PIN);
  return light_value;
}

void send_email(float temperature, float humidity, int moisture, int light, String warning)
{
  // convert values to String
  String _temperature = String(temperature);
  String _humidity = String(humidity);
  String _moisture = String(moisture);
  String _light = String(light);
  String _warning = warning;

  // Perform the HTTP request
  HTTPClient http;

  // Construct the URL with variable values
  String url = "https://hooks.zapier.com/hooks/catch/15608594/3hubi9u/?temperature=" + _temperature +
               "&humidity=" + _humidity +
               "&moisture=" + _moisture +
               "&light=" + _light +
               "&warning=" + _warning;

  http.begin(url);

  int httpCode = http.GET();
  String payload = http.getString();

  Serial.println("HTTP Response Code: " + String(httpCode));
  Serial.println("Response Body: " + payload);

  http.end();
}
