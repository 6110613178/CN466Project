/* Edge Impulse Arduino examples
 * Copyright (c) 2021 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* Includes ---------------------------------------------------------------- */
#include <Drawer_inferencing.h>

#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_HTS221.h>
#include <Adafruit_NeoPixel.h>
#include <ESPAsync_WiFiManager.h>
#include <PubSubClient.h>

// constants
#define I2C_SDA     41
#define I2C_SCL     40
#define LED_PIN     2
#define RGBLED_PIN  18

const char SENSOR_TOPIC[] = "cn466/drawer/cucumber_tar";
const char SENSOR_TOPIC2[] = "cn466/drawer_humid/cucumber_tar";

// persistence variables
Adafruit_BMP280 bmp;
Adafruit_HTS221 hts;
Adafruit_MPU6050 mpu;
Adafruit_NeoPixel pixels(1, RGBLED_PIN, NEO_GRB + NEO_KHZ800);

AsyncWebServer webServer(80);
WiFiClient esp32Client;
DNSServer dnsServer;
PubSubClient mqttClient(esp32Client);
//IPAddress netpieBroker(35, 186, 155, 39);

// setup sensors and LED
void setupHardware() {
  Wire.begin(I2C_SDA, I2C_SCL, 100000);
  if (bmp.begin(0x76)) {    // prepare BMP280 sensor
    Serial.println("BMP280 sensor ready");
  }
  if (hts.begin_I2C()) {    // prepare HTS221 sensor
    Serial.println("HTS221 sensor ready");
  }
  if (mpu.begin()) {       // prepare MPU6050 sensor
    Serial.println("MPU6050 sensor ready");
  }
  pinMode(LED_PIN, OUTPUT);      // prepare LED
  digitalWrite(LED_PIN, HIGH);
  pixels.begin();                // prepare NeoPixel LED
  pixels.clear();
}

void setupNetwork() {
  // WiFi
 WiFi.begin("TP-Link_8BFD","79723404");
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  // HiveMQ
  mqttClient.setServer("broker.hivemq.com", 1883);
}

// connect MQTT broker
void connectBroker() {
  char client_id[20];

  sprintf(client_id, "cucumber-%d", esp_random()%10000);
  mqttClient.connect(client_id);
}

static float features[] = {
  10.4100, -0.6800, -1.8400,
  10.4400, -0.8900, -1.7900,
  12.1500, -1.5600, -1.8800,
  7.7200, -1.0700, -2.0900,
  7.2200, 1.1000, -0.8100,
  8.8700, 1.0200, -0.9000,
  15.3100, -1.3100, -2.2200,
  9.7700, 1.1700, -3.8600,
  10.6900, 3.2700, -4.7400,
  10.2400, -0.0900, -1.6300,
  10.3500, -0.4000, -1.8600,
  10.3100, -0.5700, -1.7100,
  10.3600, -0.4000, -1.7100
    // copy raw features here (for example from the 'Live classification' page)
    // see https://docs.edgeimpulse.com/docs/running-your-impulse-arduino
};

/**
 * @brief      Copy raw feature data in out_ptr
 *             Function called by inference library
 *
 * @param[in]  offset   The offset
 * @param[in]  length   The length
 * @param      out_ptr  The out pointer
 *
 * @return     0
 */
int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}


/**
 * @brief      Arduino setup function
 */
void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);
    setupHardware();
    setupNetwork();
    connectBroker();
    Serial.println("Starting");
}

/**
 * @brief      Arduino main function
 */
int record = 1;
int i = 0;

void loop()
{
  //status
  static uint32_t prev_millis = 0;
  char json_body[200];
  const char json_status[] = "{\"status\": \"%s\"}";
  sensors_event_t temp, humid;
  sensors_event_t a, g;
  if(record == 1 ){
    if (millis() - prev_millis > 100){
      prev_millis = millis();
      float p = bmp.readPressure();
      hts.getEvent(&humid, &temp);
      mpu.getEvent(&a, &g, &temp);
      float ax = a.acceleration.x;
      float ay = a.acceleration.y;
      float az = a.acceleration.z;

      features[i] = ax;
      features[i+1] = ay;
      features[i+2] = az;
      i = i+3;
      //Serial.printf("%.2f,%.2f,%.2f \n",ax, ay, az);
      }
     if(i==39){
      Serial.println("record complete");
      record = 0;
      i=0;
      delay(500);
      } 
    }
    else{
    ei_printf("Edge Impulse standalone inferencing (Arduino)\n");

    if (sizeof(features) / sizeof(float) != EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        ei_printf("The size of your 'features' array is not correct. Expected %lu items, but had %lu\n",
            EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, sizeof(features) / sizeof(float));
        delay(1000);
        return;
    }

    ei_impulse_result_t result = { 0 };

    // the features are stored into flash, and we don't want to load everything into RAM
    signal_t features_signal;
    features_signal.total_length = sizeof(features) / sizeof(features[0]);
    features_signal.get_data = &raw_feature_get_data;

    // invoke the impulse
    EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false /* debug */);
    ei_printf("run_classifier returned: %d\n", res);

    if (res != 0) return;

    // print the predictions
    ei_printf("Predictions ");
    ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
        result.timing.dsp, result.timing.classification, result.timing.anomaly);
    ei_printf(": \n");
    ei_printf("[");
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        ei_printf("%.5f", result.classification[ix].value);
#if EI_CLASSIFIER_HAS_ANOMALY == 1
        ei_printf(", ");
#else
        if (ix != EI_CLASSIFIER_LABEL_COUNT - 1) {
            ei_printf(", ");
        }
#endif
    }
#if EI_CLASSIFIER_HAS_ANOMALY == 1
    ei_printf("%.3f", result.anomaly);
#endif
    ei_printf("]\n");

    // human-readable predictions
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        ei_printf("    %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);
    }
#if EI_CLASSIFIER_HAS_ANOMALY == 1
    ei_printf("    anomaly score: %.3f\n", result.anomaly);
#endif
    record = 1;

    //humidity
    static uint32_t prev_millis2 = 0;
    char json_body2[200];
    const char json_humid[] = "{\"humidity\": %.2f}";
  
    if (millis() - prev_millis2 > 15000) {
      prev_millis2 = millis();
      hts.getEvent(&humid, &temp);
      float t = temp.temperature;
      float h = humid.relative_humidity;
      sprintf(json_body2, json_humid, h);
      Serial.println(json_body2);
      mqttClient.publish(SENSOR_TOPIC2, json_body2);
    }
    
    if(result.classification[1].value >= 0.55){
      sprintf(json_body, json_status, "SingleMove");
      Serial.println(json_body);
      mqttClient.publish(SENSOR_TOPIC, json_body);
    }
    if (!mqttClient.connected()) {
    connectBroker();
    }
    mqttClient.loop();
//    delay(0);
    Serial.println("start");
    }
}

/**
 * @brief      Printf function uses vsnprintf and output using Arduino Serial
 *
 * @param[in]  format     Variable argument list
 */
void ei_printf(const char *format, ...) {
    static char print_buf[1024] = { 0 };

    va_list args;
    va_start(args, format);
    int r = vsnprintf(print_buf, sizeof(print_buf), format, args);
    va_end(args);

    if (r > 0) {
        Serial.write(print_buf);
    }
}
