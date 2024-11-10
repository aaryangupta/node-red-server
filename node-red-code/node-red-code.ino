#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include <ESP32Servo.h>

#define LedPin 2 // ESP32 in-board led pin
#define ServoPin 18 // GPIO pin where the servo is connected

const char* ssid = "The"; // WiFi SSID
const char* password = "12345678"; // WiFi Password
const char* mqtt_server = "test.mosquitto.org"; // Mosquitto Server URL

const int DHT_PIN = 15; // GPIO pin where the DHT sensor is connected
DHTesp dht;
Servo servo;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  char msg[10] = {0}; // Make sure to properly terminate the string
  memcpy(msg, payload, length);
  msg[length] = '\0'; // Null-terminate the string
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(msg);

  // Check if the message is a command to control the LED
  if (strcmp(msg, "1") == 0) {
    digitalWrite(LedPin, HIGH);
  } else if (strcmp(msg, "2") == 0) {
    digitalWrite(LedPin, LOW);
  } else if (strncmp(topic, "/ThinkIOT/servo", 15) == 0) { // Check for servo control topic
    int pos = atoi(msg);
    servo.write(pos); // Set servo position
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("Connected");
      client.subscribe("/ThinkIOT/Subscribe");
      client.subscribe("/LedControl");
      client.subscribe("/ThinkIOT/servo"); // Subscribe to servo control topic
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(LedPin, OUTPUT);
  servo.attach(ServoPin); // Attach servo
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  dht.setup(DHT_PIN, DHTesp::DHT11); // Initialize DHT sensor
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    TempAndHumidity data = dht.getTempAndHumidity();

    // Publish temperature and humidity
    String temp = String(data.temperature, 2);
    client.publish("/ThinkIOT/temp", temp.c_str());
    String hum = String(data.humidity, 1);
    client.publish("/ThinkIOT/hum", hum.c_str());

    Serial.print("Temperature: ");
    Serial.println(temp);
    Serial.print("Humidity: ");
    Serial.println(hum);
  }
}
