#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Ticker.h>
#include "ThingSpeak.h"
#include "HX711.h"

// CHANGE // // CHANGE // // CHANGE // // CHANGE //
String SECRET_NAME_TEAM = "Carlos-Fizet";      // replace Carlos-Fizet with your name Team
boolean Calibration_Weight_Sensor = false;     // false Not Calibration // true Calibration
double Known_Weight = 14.7;                    // Replace 14.6 with the value of the known object's weight in grams
double Scale_Calibration_Result = 773;         // Replace 817 with the value result after of the process of calibration
#define SECRET_CH_ID 2258947                   // replace 0000000 with your channel number of ThingSpeak
#define SECRET_WRITE_APIKEY "WFQCMUA1Y5X6QIE5" // replace XYZ with your channel write API Key of ThingSpeak
// CHANGE // // CHANGE // // CHANGE // // CHANGE //

// HX711 Circuit wiring
const int LOADCELL_DOUT_PIN = 2; // GPIO2 D2
const int LOADCELL_SCK_PIN = 4;  /// GPIO4 D4

HX711 scale;

WiFiClient client;
unsigned long myChannelNumber = SECRET_CH_ID;
const char *myWriteAPIKey = SECRET_WRITE_APIKEY;
String myStatus = "";

unsigned long previousMillis = 0;
const long interval = 1000;

Ticker ticker;
int LED_WiFi = 2;
float tempC_Sensor_1;
float tempC_Sensor_2;
float Weight_Sensor;
void tick()
{
  // toggle state
  int state = digitalRead(LED_WiFi); // get the current state of GPIO1 pin
  digitalWrite(LED_WiFi, !state);    // set pin to the opposite state
}
void tick_update_channel()
{

  ThingSpeak.setField(1, tempC_Sensor_1);
  ThingSpeak.setField(2, tempC_Sensor_2);
  ThingSpeak.setField(3, Weight_Sensor);
  myStatus = String("Deshidratador IoT OK");
  ThingSpeak.setStatus(myStatus);

  // write to the ThingSpeak channel
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (x == 200)
  {
    Serial.println("Channel update successful.");
  }
  else
  {
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }
}

#define ONE_WIRE_BUS 15 // GPIO4 D4
// GPIO15 D15

OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

DeviceAddress Sensor_1, Sensor_2;

void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16)
      Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void printResolution(DeviceAddress deviceAddress)
{
  Serial.print("Resolution: ");
  Serial.print(sensors.getResolution(deviceAddress));
  Serial.println();
}

///

void setup()
{

  pinMode(LED_WiFi, OUTPUT);
  digitalWrite(LED_WiFi, LOW);
  ticker.attach(0.6, tick);

  Serial.begin(115200);
  Serial.println("Deshidratador IoT TEC EAN 2023");

  if (Calibration_Weight_Sensor == false)
  {
    Serial.println("Mode Normal");
    WiFiManager wm;

    bool res;
    String name_iot = "Deshidratador IoT ";
    String name_device = name_iot + SECRET_NAME_TEAM;
    res = wm.autoConnect("Deshidratador IoT"); // password protected ap
    ticker.attach(0.1, tick);

    if (!res)
    {
      Serial.println("Failed to connect");
      // ESP.restart();
    }
    else
    {
      // if you get here you have connected to the WiFi
      Serial.println("connected...yeey :)");
      ticker.detach();
      digitalWrite(LED_WiFi, LOW);
    }
  }
  else
  {
    Serial.println("Mode Calibration Weight Sensor");
  }
  //
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  if (Calibration_Weight_Sensor == false)
  {
    sensors.begin();
    // locate devices on the bus
    Serial.println("Buscando Sensores de Temperatura DS18B20...");

    if (!sensors.getAddress(Sensor_1, 0))
    {
      Serial.println("Error en Sensor 1");
      digitalWrite(LED_WiFi, HIGH);
      while (true)
        ;
    }
    if (!sensors.getAddress(Sensor_2, 1))
    {
      Serial.println("Error en Sensor 2");
      digitalWrite(LED_WiFi, HIGH);
      while (true)
        ;
    }
    Serial.println("Sensores OK!");
    sensors.setResolution(Sensor_1, 12);
    sensors.setResolution(Sensor_2, 12);

    Serial.print("Sensor 1: ");
    printAddress(Sensor_1);
    Serial.print(" Resolución en Bits ");
    Serial.print(sensors.getResolution(Sensor_1), DEC);
    Serial.println();

    Serial.print("Sensor 2: ");
    printAddress(Sensor_2);
    Serial.print(" Resolución en Bits ");
    Serial.print(sensors.getResolution(Sensor_2), DEC);
    Serial.println();

    if (Calibration_Weight_Sensor == false)
    {
      ThingSpeak.begin(client);
      ticker.attach(20, tick_update_channel);
    }

    Serial.println("HX711 Demo");

    Serial.println("Initializing the scale");

    scale.set_scale(Scale_Calibration_Result);
    scale.tare(); // reset the scale to 0

    Serial.println("After setting up the scale:");

    Serial.print("read: \t\t");
    Serial.println(scale.read()); // print a raw reading from the ADC

    Serial.print("read average: \t\t");
    Serial.println(scale.read_average(20)); // print the average of 20 readings from the ADC

    Serial.print("get value: \t\t");
    Serial.println(scale.get_value(5)); // print the average of 5 readings from the ADC minus the tare weight, set with tare()

    Serial.print("get units: \t\t");
    Serial.println(scale.get_units(5), 1); // print the average of 5 readings from the ADC minus tare weight, divided
                                           // by the SCALE parameter set with set_scale

    Serial.println("Readings:");
  }
}

void loop()
{
  if (Calibration_Weight_Sensor == false)
  {
    Serial.println("-----------------------------");
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      sensors.requestTemperatures();

      digitalWrite(LED_WiFi, HIGH);
      tempC_Sensor_1 = sensors.getTempC(Sensor_1);
      if (tempC_Sensor_1 == DEVICE_DISCONNECTED_C)
      {
        Serial.println("Error: Could not read temperature data");
        return;
      }
      Serial.print("Sensor 1 => ");
      Serial.print("Temp C: ");
      Serial.print(tempC_Sensor_1);
      Serial.print(" Temp F: ");
      Serial.println(DallasTemperature::toFahrenheit(tempC_Sensor_1));

      tempC_Sensor_2 = sensors.getTempC(Sensor_2);
      if (tempC_Sensor_2 == DEVICE_DISCONNECTED_C)
      {
        Serial.println("Error: Could not read temperature data");
        return;
      }
      Serial.print("Sensor 2 => ");
      Serial.print("Temp C: ");
      Serial.print(tempC_Sensor_2);
      Serial.print(" Temp F: ");
      Serial.println(DallasTemperature::toFahrenheit(tempC_Sensor_2));
      digitalWrite(LED_WiFi, LOW);
    }

    scale.power_up();
    Serial.print("one reading:\t");
    Serial.print(scale.get_units(), 1);
    Serial.print("\t| average:\t");
    Weight_Sensor = scale.get_units(10);
    Serial.println(scale.get_units(10), 1);

    scale.power_down(); // put the ADC in sleep mode
  }
  else
  {

    if (scale.is_ready())
    {
      scale.set_scale();
      Serial.println("Tare... remove any weights from the scale.");
      delay(5000);
      scale.tare();
      Serial.println("Tare done...");
      Serial.print("Place a known weight on the scale...");
      delay(5000);
      long reading = scale.get_units(10);
      Serial.print("Result of Calibration: ");
      long calibration_final = reading / Known_Weight;
      Serial.println(calibration_final);
    }
    else
    {
      Serial.println("HX711 not found.");
    }
    delay(1000);
  }
}
