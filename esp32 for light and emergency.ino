#include <WiFi.h>           
#include <PubSubClient.h>     
#include <math.h>

const char* ssid = "your wifi name";
const char* password = "your wifi pass";
const char* mqtt_server = "your server";  

#define PIN_4 4    // GPIO 4
#define PIN_5 5    // GPIO 5
#define PIN_18 18  // GPIO 18
#define PIN_19 19  // GPIO 19
#define PIN_22 22  // GPIO 22
#define PIN_23 23  // GPIO 23
#define PIN_13 13  // GPIO 13
#define PIN_12 12  // GPIO 12

WiFiClient espClient;
PubSubClient client(espClient);

// Default Latitude and Longitude
float default_latitude = 11.2746893;  
float default_longitude = 77.6078799;

// Earth's radius in kilometers
const float EARTH_RADIUS_KM = 6371.0;
// Emergency distance threshold in kilometers
const float EMERGENCY_THRESHOLD = 3.0; // Example: 3 km

// Function to calculate the distance between two coordinates using the Haversine formula
float haversine(float lat1, float lon1, float lat2, float lon2) {
  float dLat = radians(lat2 - lat1);
  float dLon = radians(lon2 - lon1);

  lat1 = radians(lat1);
  lat2 = radians(lat2);

  float a = sin(dLat / 2) * sin(dLat / 2) +
            cos(lat1) * cos(lat2) *
            sin(dLon / 2) * sin(dLon / 2);
  float c = 2 * atan2(sqrt(a), sqrt(1 - a));
  float distance = EARTH_RADIUS_KM * c;
  
  return distance;
}

// Function to calculate bearing (direction) from one point to another
float calculate_bearing(float lat1, float lon1, float lat2, float lon2) {
  float dLon = radians(lon2 - lon1);

  lat1 = radians(lat1);
  lat2 = radians(lat2);

  float y = sin(dLon) * cos(lat2);
  float x = cos(lat1) * sin(lat2) -
            sin(lat1) * cos(lat2) * cos(dLon);
  
  float bearing = atan2(y, x);
  bearing = degrees(bearing);

  // Normalize to 0 - 360 degrees
  if (bearing < 0) {
    bearing += 360.0;
  }
  
  return bearing;
}

// Function to get the cardinal direction based on the bearing angle
String get_cardinal_direction(float bearing) {
  if (bearing >= 337.5 || bearing < 22.5) {
    return "North";
  } else if (bearing >= 22.5 && bearing < 67.5) {
    return "Northeast";
  } else if (bearing >= 67.5 && bearing < 112.5) {
    return "East";
  } else if (bearing >= 112.5 && bearing < 157.5) {
    return "Southeast";
  } else if (bearing >= 157.5 && bearing < 202.5) {
    return "South";
  } else if (bearing >= 202.5 && bearing < 247.5) {
    return "Southwest";
  } else if (bearing >= 247.5 && bearing < 292.5) {
    return "West";
  } else if (bearing >= 292.5 && bearing < 337.5) {
    return "Northwest";
  }
  return "Unknown";
}

// Function to check if coordinates are within the emergency range
void check_emergency(float lat, float lon) {
  float distance = haversine(default_latitude, default_longitude, lat, lon);

  if (distance <= EMERGENCY_THRESHOLD) {
    // Send emergency message to MQTT
    client.publish("device/emergency", "Emergency");
  }
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  if (String(topic) == "device/coordinates") {
    float received_latitude;
    float received_longitude;
    
    int commaIndex = message.indexOf(',');
    
    if (commaIndex > 0) {
      String lat_str = message.substring(0, commaIndex);
      String lon_str = message.substring(commaIndex + 1);
      
      received_latitude = lat_str.toFloat();
      received_longitude = lon_str.toFloat();
      
      if (received_latitude == 0 || received_longitude == 0) {
        Serial.println("Invalid coordinates received. Using default values.");
        received_latitude = default_latitude;
        received_longitude = default_longitude;
      }
    } else {
      Serial.println("Invalid message format. Using default coordinates.");
      received_latitude = default_latitude;
      received_longitude = default_longitude;
    }

    float distance = haversine(default_latitude, default_longitude, received_latitude, received_longitude);
    
    Serial.print("Distance to received coordinates: ");
    Serial.print(distance);
    Serial.println(" km");

    check_emergency(received_latitude, received_longitude);  // Check if emergency condition is met
    
    String lat_lon_message = String(received_latitude) + "," + String(received_longitude);
    client.publish("device/coordinates_response", lat_lon_message.c_str());

    if (distance <= 3.0) {
      float bearing = calculate_bearing(default_latitude, default_longitude, received_latitude, received_longitude);
      Serial.print("Direction (Bearing): ");
      Serial.print(bearing);
      Serial.println(" degrees");

      String direction = get_cardinal_direction(bearing);
      Serial.print("Cardinal Direction: ");
      Serial.println(direction);

      String bearing_message = String(bearing) + " degrees, " + direction + ", Distance: " + String(distance) + " km";
      client.publish("device/traffic1", bearing_message.c_str());

      if (direction == "North") {
        client.publish("device/traffic1", "A");
      } else if (direction == "East") {
        client.publish("device/traffic1", "B");
      } else if (direction == "South") {
        client.publish("device/traffic1", "C");
      } else if (direction == "West") {
        client.publish("device/traffic1", "D");
      } else {
        Serial.println("Direction is unknown. No message sent.");
      }
    } else {
      Serial.println("out of range");
    }
  }
  // Toggle LEDs based on the message
  if (message == "A") {
    digitalWrite(PIN_5, HIGH);
    digitalWrite(PIN_18, HIGH);
    digitalWrite(PIN_22, HIGH);
    digitalWrite(PIN_13, HIGH);
    digitalWrite(PIN_4, LOW);
    digitalWrite(PIN_23, LOW);
    digitalWrite(PIN_12, LOW);
    digitalWrite(PIN_19, LOW);
  }
  else if (message == "C") {
    digitalWrite(PIN_19, HIGH);
    digitalWrite(PIN_4, HIGH);
    digitalWrite(PIN_22, HIGH);
    digitalWrite(PIN_13, HIGH);
    digitalWrite(PIN_5, LOW);
    digitalWrite(PIN_18, LOW);
    digitalWrite(PIN_23, LOW);
    digitalWrite(PIN_12, LOW);
  }
  else if (message == "D") {
    digitalWrite(PIN_23, HIGH);
    digitalWrite(PIN_4, HIGH);
    digitalWrite(PIN_18, HIGH);
    digitalWrite(PIN_13, HIGH);
    digitalWrite(PIN_5, LOW);
    digitalWrite(PIN_19, LOW);
    digitalWrite(PIN_22, LOW);
    digitalWrite(PIN_12, LOW);
  } else if (message == "B") {
    digitalWrite(PIN_12, HIGH);
    digitalWrite(PIN_4, HIGH);
    digitalWrite(PIN_18, HIGH);
    digitalWrite(PIN_22, HIGH);
    digitalWrite(PIN_5, LOW);
    digitalWrite(PIN_19, LOW);
    digitalWrite(PIN_23, LOW);
    digitalWrite(PIN_13, LOW);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.publish("device/status", "MQTT Server is Connected");
      client.subscribe("device/coordinates");
      client.subscribe("device/traffic1");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Initialize GPIO pins
  pinMode(PIN_4, OUTPUT);
  pinMode(PIN_5, OUTPUT);
  pinMode(PIN_18, OUTPUT);
  pinMode(PIN_19, OUTPUT);
  pinMode(PIN_22, OUTPUT);
  pinMode(PIN_23, OUTPUT);
  pinMode(PIN_13, OUTPUT);
  pinMode(PIN_12, OUTPUT);

  // Ensure all pins are LOW initially
  digitalWrite(PIN_4, LOW);
  digitalWrite(PIN_5, LOW);
  digitalWrite(PIN_18, LOW);
  digitalWrite(PIN_19, LOW);
  digitalWrite(PIN_22, LOW);
  digitalWrite(PIN_23, LOW);
  digitalWrite(PIN_13, LOW);
  digitalWrite(PIN_12, LOW);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
