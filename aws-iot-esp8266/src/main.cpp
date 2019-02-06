#include "FS.h"

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Accelerometer
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>

#include "main.h"

// FFT and Filtering
#include <Filters.h>
#include <arduinoFFT.h>

// FFT and Filtering definitions
#define Nbins 32
#define FILTER_FREQUENCY 10   // filter 10Hz and higher
#define SAMPLES_PER_SECOND 64 //sample at 30Hz - Needs to be minimum 2x higher than desired filterFrequency

// Serial
#define BAUD_RATE 115200

/********************************************/
/*                 Globals                  */
/********************************************/
// Accelerometer MMA8451
Adafruit_MMA8451 mma = Adafruit_MMA8451();

// FFT
arduinoFFT FFT = arduinoFFT();

// Accelerometer threshold
float energy_thresh = 15.0f;

// Misc Values (Filtering)
double vReal[Nbins];
double vImag[Nbins];
float delayTime;
float accl_mag;
int lastTime = 0;

FilterOnePole filterX(LOWPASS, FILTER_FREQUENCY);
FilterOnePole filterY(LOWPASS, FILTER_FREQUENCY);
FilterOnePole filterZ(LOWPASS, FILTER_FREQUENCY);

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("[AWS] Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

// WiFi / MQTT
WiFiClientSecure espClient;
PubSubClient client(aws_mqtt_server, 8883, callback, espClient); //set  MQTT port number to 8883 as per //standard

void working_led()
{
    digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
    delay(50);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);  // turn the LED off by making the voltage LOW
    delay(50);
}

void setup_wifi()
{

    delay(100);
    working_led();
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("[WIFI] Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        working_led();
    }

    Serial.println("");
    Serial.print("[WIFI] WiFi connected on ");
    Serial.println(WiFi.localIP());
}

void setup_certs()
{
    if (!SPIFFS.begin())
    {
        Serial.println("[CERTS] Failed to mount file system");
        return;
    }

    Serial.print("[CERTS] Heap: ");
    Serial.println(ESP.getFreeHeap());

    // Load certificate file
    File cert = SPIFFS.open("/cert.der", "r"); //replace cert.crt eith your uploaded file name
    if (!cert)
    {
        Serial.println("[CERTS] Failed to open cert file");
    }
    else
        Serial.println("[CERTS] Success to open cert file");

    delay(200);

    if (espClient.loadCertificate(cert))
        Serial.println("[CERTS] cert loaded");
    else
        Serial.println("[CERTS] cert not loaded");

    // Load private key file
    File private_key = SPIFFS.open("/private.der", "r"); //replace private eith your uploaded file name
    if (!private_key)
    {
        Serial.println("[CERTS] Failed to open private cert file");
    }
    else
        Serial.println("[CERTS] Success to open private cert file");

    delay(200);

    if (espClient.loadPrivateKey(private_key))
        Serial.println("[CERTS] private key loaded");
    else
        Serial.println("[CERTS] private key not loaded");

    // Load CA file
    File ca = SPIFFS.open("/ca.der", "r"); //replace ca eith your uploaded file name
    if (!ca)
    {
        Serial.println("[CERTS] Failed to open ca ");
    }
    else
        Serial.println("[CERTS] Success to open ca");
    delay(200);

    if (espClient.loadCACert(ca))
        Serial.println("[CERTS] ca loaded");
    else
        Serial.println("[CERTS] ca failed");
    Serial.print("[CERTS] Heap: ");
    Serial.println(ESP.getFreeHeap());
    working_led();
}

void aws_reconnect()
{
    // Loop until we're reconnected
    while (!client.connected())
    {
        Serial.print("[AWS] Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect(aws_mqtt_client_id))
        {
            Serial.println("[AWS] connected");
            // ... and resubscribe
            client.subscribe(aws_mqtt_thing_topic_sub);
        }
        else
        {
            Serial.print("[AWS] failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            working_led();
            delay(5000);
        }
    }
}

void setup_accl()
{
    if (!mma.begin())
    {
        Serial.println("accl [Failed]");
        while (1)
            ;
    }
    Serial.println("accl [Connected]");
    mma.setRange(MMA8451_RANGE_2_G);
}

void setup()
{
    // Debug LED
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(BAUD_RATE);
    delay(200);
    setup_accl();
    delay(200);
    setup_wifi();
    delay(200);
    setup_certs();
    delay(200);
    aws_reconnect();
}

double totalEnergy(double array[])
{
    int i;
    double integrate = 0;
    for (i = 2; i < FILTER_FREQUENCY * 2; i += 2)
    {
        //taking basic numerical value for integration, multiplying bin frequency width by its height.
        integrate += array[i] * 1;
    }
    return (integrate);
}

float filteredMagnitude(float ax, float ay, float az)
{
    filterX.input(ax);
    filterY.input(ay);
    filterZ.input(az);
    return (sqrt(pow(filterX.output(), 2) + pow(filterY.output(), 2) + pow(filterZ.output(), 2)));
}

float normalMagnitude(float ax, float ay, float az)
{
    return (sqrt(pow(ax, 2) + pow(ay, 2) + pow(az, 2)));
}

void loop()
{
    if (!client.connected())
    {
        aws_reconnect();
    }
    client.loop();

    sensors_event_t event;

    //run it at 30 samples/s
    for (int ii = 0; ii <= Nbins; ii++)
    {
        // delay
        if (millis() < lastTime + delayTime)
        {
            delay(lastTime + delayTime - millis());
        }

        // Read the 'raw' data in 14-bit counts
        // Get a new sensor event
        mma.read();
        mma.getEvent(&event);

        // Magnitude of values
        vReal[ii] = filteredMagnitude(event.acceleration.x, event.acceleration.y, event.acceleration.z);
        vImag[ii] = 0;
        lastTime = millis();
    }

    FFT.Windowing(vReal, Nbins, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(vReal, vImag, Nbins, FFT_FORWARD);
    FFT.ComplexToMagnitude(vReal, vImag, Nbins);
    accl_mag = normalMagnitude(event.acceleration.x, event.acceleration.y, event.acceleration.z);

    //since we've calculated the frequency, check the ranges we care about (1hz-8hz)
    //sum them and check if they're higher than our threshold.
    if (totalEnergy(vReal) >= energy_thresh)
    {
        const size_t bufferSize = JSON_OBJECT_SIZE(15) + 20;
        DynamicJsonBuffer jsonBuffer(bufferSize);
        JsonObject &root = jsonBuffer.createObject();
        JsonObject &state = root.createNestedObject("state");
        JsonObject &reported = state.createNestedObject("reported");
        reported["a_x"] = event.acceleration.x;
        reported["a_y"] = event.acceleration.y;
        reported["a_z"] = event.acceleration.z;
        //reported["a_m"] = accl_mag;

        // JsonArray &accl_fft_data = root.createNestedArray("accl_fft");
        // for (int ii = 2; ii <= Nbins - 1; ii += 2)
        // {
        //     accl_fft_data.add(vReal[ii]);
        // }

        String json_output;
        root.printTo(json_output);
        char payload[bufferSize];

        // Construct payload item
        json_output.toCharArray(payload, bufferSize);
        sprintf(payload, json_output.c_str());

        Serial.print("[AWS MQTT] Publish Message:");
        Serial.println(payload);
        client.publish(aws_mqtt_thing_topic_pub, payload);
    }
}
