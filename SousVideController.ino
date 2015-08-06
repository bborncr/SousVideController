/*
bentley@crcibernetica.com
http://crcibernetica.com

  This example code is in public domain

*/
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>
#include <Nextion.h>

#define RelayPin 13

SoftwareSerial nextion(2, 3);

Nextion myNextion(nextion, 9600);

double target_temp = 80.0;
double current_temp = 25.0;
double old_temp = 25.0;

int sleep_counter = 0;
int display_counter = 0;

double max_count = 20000; // duty cycle in milliseconds
double min_relay_time = 1000; // minimum time to switch relay (stops flapping)
double max_time = 0;
double on_count = 0;
double current_count = 0;

boolean heater = false;
boolean old_heater = false;
String page = "monitor";

//Thermometer parameters
// Data wire is plugged into port A5 on the Arduino
#define ONE_WIRE_BUS A5

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(9600);
  pinMode(RelayPin, OUTPUT);
  digitalWrite(RelayPin, LOW);
  sensors.begin();
  current_temp = getTemperature();

  myNextion.init("monitor");
  delay(250);
  myNextion.sendCommand("sleep=0");
  sleep_counter = 0;
  refreshAll();
  current_count = millis();
  max_time = current_count + max_count;
  on_count = heaterControl(target_temp, current_temp, max_count);

}

void loop() {

  String message = myNextion.listen();

  // if (message != "") {
  //   Serial.println(message);
  // }

  if (message == "65 0 1 1 ffff ffff ffff") {
    page = "config";
    myNextion.sendCommand("page config");
    myNextion.setComponentText("t0", String(target_temp, 0));
  }

  if (message == "65 1 e 0 ffff ffff ffff" || message == "65 1 1 1 ffff ffff ffff") {
    page = "monitor";
    myNextion.sendCommand("page monitor");
    refreshAll();
  }

  if (message.startsWith("68")) {
    myNextion.sendCommand("sleep=0");
    delay(250);
    sleep_counter = 0;
  }

  if (message.startsWith("70")) {
    String new_target = message.substring(3, message.length()); //Cut the begining text
    float temp = new_target.toFloat();
    if (temp > 15 && temp < 95) { //only change target_temp if within range of acceptable values
      target_temp = temp;
    }
  }

  updateTemperature();
  //updateDisplay(10000);

  if (page == "monitor") {
    if (abs(current_temp - old_temp) >= 1) {
      refreshAll();
      old_temp = current_temp;
    }
    if (heater != old_heater) {
      String comm = "b2.picc=" + String(int(heater));
      myNextion.sendCommand(comm.c_str());
      myNextion.sendCommand("ref b2");
      digitalWrite(RelayPin, heater);
      old_heater = heater;
    }
    //updateSleep(30000); //sleep after 30 seconds
  }

}

void updateDisplay(unsigned long timeOut) {
  display_counter++;
  if (display_counter % timeOut == 0) {
    String comm = "b2.picc=" + String(int(heater));
    myNextion.sendCommand(comm.c_str());
    myNextion.sendCommand("ref b2");
  }
}

void updateSleep(unsigned long timeOut) {
  sleep_counter++;
  if (sleep_counter % timeOut == 0) {
    myNextion.sendCommand("page monitor");
    myNextion.sendCommand("sleep=1");
  }
}

void refreshAll() {
  String comm = "b2.picc=" + String(int(heater));
  myNextion.sendCommand(comm.c_str());
  myNextion.sendCommand("ref b2");
  myNextion.setComponentText("t0", String(target_temp, 0));
  myNextion.setComponentText("t1", String(current_temp, 0));
}

double getTemperature() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

double heaterControl(double target, double current, double max_heater_time) {
  double temp_diff = target - current;
  double heat_time = map(temp_diff, 0, 50, 0, max_heater_time);
  if (heat_time > max_heater_time){
    heat_time = max_heater_time;
    }
  if (heat_time < min_relay_time) {
    heat_time = 0;
    }
  return heat_time;
}

void updateTemperature() {
  current_temp = getTemperature();
  current_count = millis();
  if (current_count < on_count) {
    heater = true;
  } else if ((current_count > on_count) && (current_count < max_time)){
    heater = false;
  } else if (current_count > max_time) {
    on_count = current_count + heaterControl(target_temp, current_temp, max_count);
    max_time = max_count + current_count;
  }
}
