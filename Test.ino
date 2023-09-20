#include <WiFi.h>
#include <PubSubClient.h>
#include "HomeSpan.h"
#include "extras/Pixel.h"

#define NEOPIXEL_RGB_PIN 26
#define NUM_PIXELS 74

const char* mqttServerIP = "broker.hivemq.com"; 
unsigned long interval = 5000; 
unsigned long previousMillis = 0;
bool value1Enabled = false;

WiFiClient espClient;
PubSubClient client(espClient);

struct NeoPixel_RGB : Service::LightBulb {
  Characteristic::On power{0,true};
  Characteristic::Hue H{0,true};
  Characteristic::Saturation S{0,true};
  Characteristic::Brightness V{100,true};
  Pixel *pixel;
  uint8_t nPixels;
  int pre_value;
  int value;

  NeoPixel_RGB(uint8_t pin, uint8_t nPixels) : Service::LightBulb() {
    V.setRange(0, 100, 1);   
    V.setVal(20, true);                   
    pixel = new Pixel(pin);           
    this->nPixels = nPixels; 
    this->pre_value = 100;
    this->value = 0;
     client.setCallback([this](char* topic, byte* payload, unsigned int length) {
      this->callback(topic, payload, length);
    });
    update();
  }

  void callback(char* topic, byte* payload, unsigned int length) {
    String receivedPayload = "";
    for (int i = 0; i < length; i++) {
      receivedPayload += (char)payload[i];
    }
    Serial.println(receivedPayload);
    value = receivedPayload.toInt() * 6;
    if (value > 100) value = 100;
    if (value < 0) value = 0;
    this->value = value;
  }

  boolean update() override {
   
    int p = power.getNewVal();
    float v = V.getVal<float>();
    float h = H.getNewVal<float>();
    float s = S.getNewVal<float>();
    Pixel::Color color;
    
    if (v != V.getNewVal() || H.getVal() != H.getNewVal()|| S.getVal() != S.getNewVal()) {
      v = V.getNewVal();
      pre_value = v;
      value = v;
      pixel->set(color.HSV(h*p, s*p, v*p), nPixels);
      return true;
    }
    if (pre_value == value) 
      return true;
    if (pre_value != value) {
      if(value == 0) p = 0, power.setVal(0, true);
      else  p = 1, power.setVal(1, true);
      pre_value = value;
      V.setVal(value, true);
      v = (float)value;
    }
    
    pixel->set(color.HSV(h*p, s*p, v*p), nPixels);
    return true;
  }
  
  void loop() {
    update();
  }
};

void setup() {
  Serial.begin(115200);
  WiFi.begin("Haha", "vannhucu");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  homeSpan.begin(Category::Lighting, "HoangDieu Led");                                             
  SPAN_ACCESSORY("Neo RGB");
  new NeoPixel_RGB(NEOPIXEL_RGB_PIN, 74);  
  client.setServer(mqttServerIP, 1883);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32 Client")) {
      Serial.println("connected");
      client.subscribe("HoangDieu");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected() && WiFi.status() == WL_CONNECTED) {
    reconnect();
  }
  client.loop();
  homeSpan.poll();
}
