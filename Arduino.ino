// === Required libraries for the real-time optical monitoring system ===

//ArduinoJson is used to build structured messages
//The messages are sent to the PC for real time visualization and data logging
#include <ArduinoJson.hpp> 
#include <ArduinoJson.h>

//SparkFun library for the TSL2561 digital light sensor
//These sensors are used to measure scattered light intensity during gel formation.
#include <SparkFunTSL2561.h>

// Wire library provides the low-level I2C communication protocol,
// required to interface with sensors and the I2C multiplexer.
#include <Wire.h>

// SparkFun Qwiic I2C multiplexer library.
// The multiplexer allows multiple sensors with the same I2C address
// to be connected and read sequentially.
#include <SparkFun_I2C_Mux_Arduino_Library.h>


// === Multiplexer object ===

// This device selects which optical sensor is active on the I2C bus.
QWIICMUX mux;


// === System configuration ===

// Total number of optical sensors installed in the device.
// Each sensor corresponds to one measurement channel (e.g., one sample position).
#define NUMBER_OF_SENSORS 4

// Array of TSL2561 sensor objects.
// This enables parallel management of multiple detectors through the multiplexer.
SFE_TSL2561 tls[4];

// === Timing and acquisition control variables ===

// Stores the timestamp of the last recorded measurement for each sensor channel.
// Used to enforce the sampling interval.
unsigned long lastMeasurementTime[4] = {0, 0, 0, 0};

// Stores the starting time of the measurement cycle for each sensor.
// This is useful for tracking the gelation process over time.
unsigned long StartMeasurement[4] = {0, 0, 0, 0};

// Boolean flags indicating whether each sensor channel is currently active
// in a measurement run.
bool measuring[4] = {false, false, false, false};

// Total duration of a measurement cycle for each sensor (in milliseconds).
// After this time, the acquisition for that channel is stopped automatically.
unsigned long cycle[4] = {60000, 60000, 60000, 60000};


// Time interval between consecutive measurements (sampling period).
// Defines the temporal resolution of the real-time monitoring.
unsigned long interval[4] = {500, 500, 500, 500};

// Sensor integration time (exposure time) in milliseconds.
// Controls sensitivity and signal-to-noise ratio for scattered light detection.
unsigned long integration[4] = {20, 20, 20, 20};

// === Connection status variable ===

// This variable is used to track the communication state between the Arduino
// and the PC interface (e.g., handshake or connection confirmation).
int ConnectionStatus = 0;


void setup(void) {
  // === Serial communication initialization ===
  // The Arduino communicates with the PC interface through the serial port.
  // Measurement data and system status messages are transmitted in real time.
  Serial.begin(9600);
  // A very long timeout is set to prevent the serial parser from stopping
  // during long experimental runs (e.g., extended gelation monitoring).
  Serial.setTimeout(20000000000000000000000);
   // === I2C multiplexer initialization ===
  // The Qwiic multiplexer is required to manage multiple optical sensors
  // sharing the same I2C address.
  mux.begin();
   // === Sensor initialization loop ===
  // Each sensor is connected to a dedicated multiplexer port.
  // The system sequentially activates each port and configures the sensor.
  int i = 0;
  for (int i = 0; i < 4; i++) {
    // Select the active I2C channel on the multiplexer
    mux.setPort(i);
    // Initialize the TSL2561 light sensor on the selected port
    tls[i].begin();
    // Configure sensor timing parameters:
    // - Gain and integration settings affect sensitivity for scattered light detection
    tls[i].setTiming(1, 3);
    // Power up the sensor to start measurements
    tls[i].setPowerUp();
    // === Sensor availability check ===
    // If the sensor does not respond correctly, the system enters an infinite loop.
    // This ensures that experiments are not run with missing or faulty detectors.
    if (!tls[i].begin()) {
      while (1); // Halt execution if initialization fails
    }
  }
}

// === JSON message container ===

// A fixed-size JSON document is used to build lightweight structured messages.
// These messages are transmitted to the PC interface for real-time monitoring.
StaticJsonDocument<200> jsonDoc;

// === System status message ===
// Sends the global connection/state value between Arduino and the PC software.
// This can be used as a handshake indicator or general system readiness flag.
void sendMessageStatus() {
  // Message label identifies the type of information being sent
  jsonDoc["label"] = "State";
  // Current connection or system status value
  jsonDoc["value"] = ConnectionStatus;
  // Serialize the JSON object into a string and transmit it over serial
  String message;
  serializeJson(jsonDoc, message);
  Serial.println(message);
}

// === Photodiode channel status message ===
// Reports whether a specific sensor channel is currently active or measuring.
// This allows the PC interface to update the status of each detector in real time.
void sendMessagePhotodiode(int i) {
  // Main label for this message type
  jsonDoc["label"] = "PhotodiodeStatus";
  // Sub-label indicates which photodiode channel is being referenced
  jsonDoc["sublabel"] = i;
  // Boolean measurement state of the selected sensor channel
  jsonDoc["value"] = measuring[i];
  // Serialize and send through the serial link
  String message;
  serializeJson(jsonDoc, message);
  Serial.println(message);
}

// === Timing information message ===
// Sends timing-related parameters or elapsed time values to the PC.
// This is useful for synchronizing measurement cycles and tracking gelation kinetics.
void sendMessageTimes(int n , long times) {
  // Message label for timing data
  jsonDoc["label"] = "Times";
  // Sub-label specifies which timing variable or sensor channel is associated
  jsonDoc["sublabel"] = n;
  // Time value transmitted (e.g., elapsed time, cycle duration, sampling interval)
  jsonDoc["value"] = tieme;
  // Serialize and send the JSON message
  String message;
  serializeJson(jsonDoc, message);
  Serial.println(message);
}

void loop(void) {
  // === Incoming command handling ===
  // The Arduino continuously checks for incoming serial messages from the PC.
  // These messages may contain commands to start/stop measurements or update settings.
  if (Serial.available()) {
    ReadMessage();
  }
  // === Main acquisition loop over all sensor channels ===
  // Each photodiode channel is evaluated independently.
  // If a channel is active, measurements are performed at the defined sampling interval.
  for (int i = 0; i < 4; i++) {
     // Check whether this channel is currently measuring
    // and whether the sampling interval has elapsed since the last acquisition.
    if (measuring[i] && ((millis() - lastMeasurementTime[i]) >= interval[i])) {
      // === Multiplexer channel selection ===
      // Activate the corresponding I2C port to communicate with the selected sensor.
      mux.setPort(i);
      // === Manual integration measurement ===
      // The TSL2561 sensor is triggered manually to control the exposure time.
      // This is important for optimizing sensitivity during scattered-light detection.
      tls[i].manualStart();
      delay(integration[i]);
      tls[i].manualStop();
      // Raw sensor output registers:
      // data0 corresponds to broadband light intensity
      // data1 corresponds to infrared component (not used here)
      unsigned int data0, data1;
      // === Data acquisition ===
      // If valid sensor data is obtained, it is packaged into a JSON message
      // and transmitted to the PC for real-time plotting and storage.
      if (tls[i].getData(data0, data1)) {
        // Create a JSON document for the measurement packet
        StaticJsonDocument<200> medidaJson;
        // Message label indicates this is a data transmission
        medidaJson["label"] = "Data";
        // Sub-label identifies the sensor channel index
        medidaJson["sublabel"] = i;
        // Measurement payload:
        // [elapsed time since start of measurement, scattered light intensity]
        JsonArray jsonArray = medidaJson.createNestedArray("value");
        jsonArray.add(millis() - StartMeasurement[i]); // Time axis for gelation kinetics
        jsonArray.add(data0); // Scattering intensity signal
        // Serialize and send the measurement packet
        String message;
        serializeJson(medidaJson, message);
        Serial.println(message);
      }
       // Update timestamp of the last measurement for this channel
      lastMeasurementTime[i] = millis();
      // === Automatic end of measurement cycle ===
      // If the predefined acquisition cycle duration has been reached,
      // the channel is stopped and its status is reported to the PC interface.
      if (millis() - StartMeasurement[i] >= cycle[i]) {
         // Stop measurements for this sensor channel
        measuring[i] = false;
        // Send updated photodiode status message
        sendMessagePhotodiode(i)
      }
    }
  }

}

// === Command evaluation function ===
// This function interprets incoming JSON commands received from the PC software.
// Commands allow real-time configuration of acquisition parameters and control
// of each optical sensor channel independently
void AssessMessage(String label, long sublabel, long value) {
  // === Communication / handshake command ===
  // Updates the connection state variable and confirms system status back to the PC.
  if (label == "Comunication") {
    ConnectionStatus = value;
  sendMessageStatus();
  }
  // === Measurement cycle duration command ===
  // Updates the total acquisition time for the selected channel.
  // The value received from the PC is interpreted in minutes.
  else if (label == "Cycle") {
    cycle[sublabel] = value*60*1000; // Convert minutes to milliseconds
    sendMessageTimes(0 , cycle[sublabel]);
  }
  // === Sampling interval command ===
  // Updates the time between consecutive measurements for a given sensor channel. 
  else if (label == "Interval") {
    interval[sublabel] = value;
    sendMessageTimes(1 , interval[sublabel]);
  }
  // === Sensor integration time command ===
  // Adjusts the exposure (integration) time of the photodiode sensor,
  // affecting sensitivity and noise performance.
  else if (label == "Integration") {
    integration[sublabel] = value;
    sendMessageTimes(2 , integration[sublabel]);
  }
  // === Sensor start/stop command ===
  // Enables or disables real-time acquisition for the selected sensor channel.
  else if (label == "Sensor") {
    // If value == 1, start the measurement cycle
    if (value == 1) {
      measuring[sublabel] = true;
      // Store the starting time reference for kinetic monitoring
      StartMeasurement[sublabel] = millis();
      // Optional debug message through serial output
      Serial.println("The measurement began in " + sublabel);
    } 
    // Otherwise, stop acquisition on this channel
    else
      measuring[sublabel] = false;
    // Report updated sensor status back to the PC interface
    sendMessagePhotodiode(sublabel);
  }
}

// === Serial JSON message reader ===
// Reads one complete JSON command sent from the PC and dispatches it
// to the evaluation function above.
void ReadMessage() {
  // Read incoming serial message until newline character
  String jsonMessage = Serial.readStringUntil('\n');
  // Basic validation: ensure the message looks like a JSON object
  if (jsonMessage.startsWith("{")) {
    // Allocate a JSON document for parsing the received command
    StaticJsonDocument<200> receivedJson;
    // Deserialize JSON string into the document structure
    deserializeJson(receivedJson, jsonMessage);
    // Check that the required keys are present
    if (receivedJson.containsKey("label") &&
        receivedJson.containsKey("sublabel") &&
        receivedJson.containsKey("value")) {
      // Extract command fields
      String label = receivedJson["label"];
      int sublabel = receivedJson["sublabel"];
      long value = receivedJson["value"];
      // Execute the corresponding system action
      AssessMessage(label, sublabel, value);
    }
    // Clear document memory after processing
    receivedJson.clear();
  }
}
