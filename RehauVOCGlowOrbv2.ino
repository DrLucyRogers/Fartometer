/*
 
 Rehau VOC Glow Orb

 Gas visual alert using a Rehau VOC, WeMoS D1 ESP-8266 board and IBM Bluemix Quickstart.

 Expects to receive a number, in ASCII, over MQTT, which it compares to a threshold to decide whether
 to display red or green. e.g. "400"

 by Andy Stanford-Clark - with embellishments by Lucy Rogers
 Copyright (c) 2016 IBM Corporation

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 

 use board "WeMos D1 R2 & mini"
 CPU 160MHz
 4M (3M SPIFFS)
 upload speed 115200
 
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

// remember to change MQTT_KEEPALIVE to 60 in the header file ~/Arduino/libraries/PubSubClient/src

/////////////////////////////////////////////////////////////////////////////////////////////

// Update these with values suitable for your network.
const char* wifi_ssid = "*****";
const char* wifi_password = "*****";

// make up a unique identifier for your application to replace xxxxx here
#define CLIENTID "a:quickstart:xxxxx"

// replace yyyyy with a name that's unique to you
#define COMMAND_TOPIC "iot-2/type/VOCmeter/id/yyyyy/evt/reading/fmt/json"

// set this to an appropriate value for where you want the green/red transition to occur
#define THRESHOLD 600

/////////////////////////////////////////////////////////////////////////////////////////////


#define BROKER "quickstart.messaging.internetofthings.ibmcloud.com"
#define PASSWORD ""

//WiFiClientSecure espClient;
WiFiClient espClient;
PubSubClient client(espClient);

//Change this if using different number of neopixels or different pin
Adafruit_NeoPixel pixel = Adafruit_NeoPixel(1, 4); // one pixel, on pin 4
// pin 4 is D2 on the WeMoS D1 mini

// flashes this colour when connecting to wifi:
static uint32_t wifi_colour = pixel.Color(64, 0, 64); // magenta
// flashes this colour when connecting to MQTT:
static uint32_t mqtt_colour = pixel.Color(0, 64, 64); // cyan

static uint32_t current_colour = 0x000000; // black
static uint32_t current_LED = current_colour;



void setup() {
  
  Serial.begin(9600);

  pixel.begin();
  pixel.show(); // Initialize all pixels to 'off'
  
  setup_wifi();
  client.setServer(BROKER, 1883);
  client.setCallback(callback);
  
}


void setup_wifi() {
  // connecting to wifi
  set_colour(wifi_colour); 
  
  // We start by connecting to a WiFi network   
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  wait_for_wifi();
   
}


void callback(char* topic, byte* payload, unsigned int length) {
  char content[10];
  int count;
  char count_str[3];
  
 Serial.print("Message arrived: '");

  strncpy(content, (char *)payload, length);
  content[length] = '\0';
  
 Serial.print(content);
  Serial.println("'");

  uint32_t value = strtol(content, 0, 10);
  //Serial.println(value);

   // if it's above the threshold, turn red, otherwise green
  if (value > THRESHOLD) {
      set_pixels(0xff0000); // red
  }
  else {
    set_pixels(0x00ff00); // green
  }


}


void wait_for_wifi()
{
  Serial.println("waiting for Wifi");
  
  // connecting to wifi - magenta
  set_pixels(wifi_colour);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    toggle_pixel();
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  set_colour(0);
}


void reconnect() {
  boolean first = true;

  // Loop until we're reconnected to the broker
  while (!client.connected()) {
 
    if (WiFi.status() != WL_CONNECTED) {
      wait_for_wifi();
      first = true;
    }
    
 
    Serial.print("Attempting MQTT connection...");
      
    if (first) {
      // now we're on wifi, show connecting to MQTT colour
      set_colour(mqtt_colour);
      first = false;
    }

    // Attempt to connect
    if (client.connect(CLIENTID, "use-token-auth", PASSWORD)) {
      Serial.println("connected");
    } 
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      
      toggle_pixel();
      delay(5000);
    }
  }
  
  set_colour(0); // clear pixel when connected (black)
 
  // subscribe to the command topic
  client.subscribe(COMMAND_TOPIC);
      
}


void loop() {
  int reading;
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

}



void set_colour(uint32_t colour) {
  set_pixels(colour);
  // Update current_LED with what the user last requested,
  // so we can toggle it to black and back again.
  current_colour = colour;

}


void set_pixels(uint32_t colour) {
  for (int i = 0; i < pixel.numPixels(); i++) {
    pixel.setPixelColor(i, colour);
  }
  pixel.show();
  
  // Store current actual LED colour
  // (which may be black if toggling code turned it off.)
  current_LED = colour;

}

void toggle_pixel() {

  if (current_LED == 0) 
  {
    // if it's black, set it to the stored colour
    set_pixels(current_colour);
  } else {
    // otherwise set it to black
    set_pixels(0);
  }
}






