#include <LiquidCrystal.h>      
#include <ESP8266WiFi.h>         
#include <PubSubClient.h>        


const char* ssid = "your wifi name";  
const char* password = "your wifi pass";   
const char* mqtt_server = "your server";  

#define LED_PIN_2 2  
#define BUZZER_PIN 16  


#define LCD_RS  4    
#define LCD_E   5    // GPIO 14
#define LCD_D4  12   // GPIO 6
#define LCD_D5  13   // GPIO 7
#define LCD_D6  14   // GPIO 5
#define LCD_D7  15   // GPIO 4

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int messageCount = 0;  

LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

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

  // Check if the message is "emergency"
  if (message == "Emergency") {
    Serial.println("Emergency received");
    

    // Display message on the LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Emergency");
    lcd.setCursor(0, 1);
    lcd.print("ambulance is");
    lcd.setCursor(0, 2);
    lcd.print("in this way");

    // Activate the buzzer
    digitalWrite(BUZZER_PIN, HIGH);  // Turn buzzer ON
    delay(1000);                     // Buzzer ON for 1 second
    digitalWrite(BUZZER_PIN, LOW);   // Turn buzzer OFF
  }

  // Check if the message is to control the LED or increment the count
  if (String(topic) == "device/led") {
    if (message == "INCREMENT") {
      messageCount++;
      snprintf(msg, MSG_BUFFER_SIZE, "Message count: %d", messageCount);
      Serial.print("Publish message: ");
      Serial.println(msg);
      client.publish("device/messageCount", msg);
    } else if (message == "ON") {
      digitalWrite(LED_PIN_2, LOW);  // Turn the LED on
      Serial.println("LED on GPIO 2 is turned ON");
    } else if (message == "OFF") {
      digitalWrite(LED_PIN_2, HIGH); // Turn the LED off
      Serial.println("LED on GPIO 2 is turned OFF");
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.publish("device/status", "MQTT Server is Connected");
      client.subscribe("device/emergency");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(LED_PIN_2, OUTPUT);  // Initialize the LED_PIN_2 (GPIO 2) as an output
  digitalWrite(LED_PIN_2, HIGH);  // Ensure LED on GPIO 2 is off initially

  pinMode(BUZZER_PIN, OUTPUT);  // Initialize the BUZZER_PIN (GPIO 16) as an output
  digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer is off initially

  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Initialize LCD
  lcd.begin(16, 2);  // 16 columns and 2 rows
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
