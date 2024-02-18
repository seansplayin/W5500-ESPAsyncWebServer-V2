// pump 1 relay output (on=pin low, off = pin high) is now congruent with serial monitor,
// websocket console and checkbox on webpage but can now be changed pin low = on, pin high = off or 
// pin high = on and pin low = off in each pumps togglePumpxState function
//02-18-24; Auto mode now integrated into pump 1. When in Auto Mode pump relay engage if simulated
// temperature is raised above temperature threshold. pump state (on, off, auto) are sent to Websocket/Serial Monitor
// Sketch 41: add a drop down pump mode selector for pump 1 to webpage that now properly controls pump 1 mode for Auto, On, Off
// 
// 


#include <Ticker.h>
#include <SPI.h>
#include <WebServer_ESP32_SC_W5500.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>

Ticker timer;
Ticker pump1Timer;
Ticker pump2Timer;
Ticker pump3Timer;
Ticker pump4Timer;
Ticker pump5Timer;
Ticker pump6Timer;
Ticker pump7Timer;

unsigned long lastLoopTime = 0;

#define W5500_MOSI 11
#define W5500_MISO 13
#define W5500_SCK 12
#define W5500_SS 10
#define W5500_INT 4

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Global variables for simulated temperature and threshold
float simulatedTemperature = 20.0; // Initial simulated temperature
const float temperatureThreshold = 25.0; // Temperature threshold for AUTO mode


#define PUMP_OFF 0
#define PUMP_ON 1
#define PUMP_AUTO 2

const int pumpPin1 = 2;
// Define other pins...

// Example configurations for pump 1
bool pump1OnStateHigh = false;
bool pump1OffStateHigh = true;

int pump1Mode = PUMP_AUTO; // Added: Current mode of pump 1
int pump1State = PUMP_OFF;
// Define other states...

void setPumpState(int pumpState, int pumpPin, bool isOnStateHigh, bool isOffStateLow) {
  digitalWrite(pumpPin, pumpState == PUMP_ON ? (isOnStateHigh ? HIGH : LOW) : (isOffStateLow ? LOW : HIGH));
}

// Function to change the state of Pump 1 and broadcast it
void togglePump1State() {
    // Manual toggling should not affect when in AUTO mode
    if (pump1Mode == PUMP_AUTO) {
        Serial.println("Pump is in AUTO mode. Manual toggling is ignored.");
        return; // Exit the function without changing the state
    }

    // Toggle between ON and OFF when not in AUTO mode
    pump1Mode = (pump1Mode == PUMP_ON) ? PUMP_OFF : PUMP_ON;
    applyPumpMode(); // This function applies the mode change
}

void applyPumpMode() {
    if (pump1Mode == PUMP_ON) {
        setPumpState(PUMP_ON, pumpPin1, pump1OnStateHigh, !pump1OnStateHigh);
        Serial.println("Pump is set to ON.");
    } else if (pump1Mode == PUMP_OFF) {
        setPumpState(PUMP_OFF, pumpPin1, !pump1OnStateHigh, pump1OnStateHigh);
        Serial.println("Pump is set to OFF.");
    } else if (pump1Mode == PUMP_AUTO) {
        // The actual pump state in AUTO mode will be determined by checkAutoModeConditions
        Serial.println("Pump is set to AUTO mode.");
    }

    // Concatenate and broadcast the new mode state
    String message = "pump1Mode:";
    message += (pump1Mode == PUMP_ON) ? "on" : (pump1Mode == PUMP_OFF) ? "off" : "auto";
    ws.textAll(message);
}




// Add toggle functions for other pumps as needed...

// Function to check and apply AUTO mode logic based on simulated temperature
void checkAutoModeConditions() {
  if (pump1Mode == PUMP_AUTO) {
    if (simulatedTemperature > temperatureThreshold) {
      if (pump1State != PUMP_ON) { // Only change state if different
        pump1State = PUMP_ON;
        setPumpState(pump1State, pumpPin1, pump1OnStateHigh, !pump1OnStateHigh);
        Serial.println("AUTO mode: Pump turned ON due to high temperature");
      }
    } else {
      if (pump1State != PUMP_OFF) { // Only change state if different
        pump1State = PUMP_OFF;
        setPumpState(pump1State, pumpPin1, !pump1OnStateHigh, pump1OnStateHigh);
        Serial.println("AUTO mode: Pump turned OFF due to low temperature");
      }
    }
  }
  // After checking conditions, broadcast the new state
  broadcastPumpState();
}

void broadcastPumpState() {
    // Construct the message to send
    String message = "pump1State:";
    message += (pump1State == PUMP_ON) ? "on" : "off";
    message += ",pump1Mode:";
    message += (pump1Mode == PUMP_AUTO) ? "auto" : (pump1Mode == PUMP_ON) ? "on" : "off";

    // Send the message to all connected WebSocket clients
    ws.textAll(message.c_str());

    // Optionally, print the message to the Serial Monitor for debugging
    Serial.println("Broadcasting: " + message);
}



// Function to set the pump mode (ON, OFF, AUTO)
void setPumpMode(int mode) {
    Serial.print("Current mode before setting: ");
    Serial.println((pump1Mode == PUMP_AUTO) ? "AUTO" : (pump1Mode == PUMP_ON) ? "ON" : "OFF");

    pump1Mode = mode;

    Serial.print("New mode set: ");
    Serial.println((pump1Mode == PUMP_AUTO) ? "AUTO" : (pump1Mode == PUMP_ON) ? "ON" : "OFF");

    if (pump1Mode == PUMP_ON) {
        // Explicitly turn the pump on
        pump1State = PUMP_ON;
        setPumpState(pump1State, pumpPin1, pump1OnStateHigh, !pump1OnStateHigh);
    } else if (pump1Mode == PUMP_OFF) {
        // Explicitly turn the pump off
        pump1State = PUMP_OFF;
        setPumpState(pump1State, pumpPin1, !pump1OnStateHigh, pump1OnStateHigh);
    } else if (pump1Mode == PUMP_AUTO) {
        // In AUTO mode, let checkAutoModeConditions handle the state based on temperature
        checkAutoModeConditions();
    }

    Serial.print("Broadcasting mode change: pump1Mode:");
    Serial.println((pump1Mode == PUMP_ON) ? "on" : (pump1Mode == PUMP_OFF) ? "off" : "auto");

    // Broadcast the new mode and state after change
    broadcastPumpState();
}



// Initialize WebSocket communications
void initWebSocket() {
  // WebSocket initialization and event handling setup
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->opcode == WS_TEXT) {
    data[len] = 0; // Null-terminate the data
    String message = (char*)data;
    
    if (message.startsWith("setPumpMode:")) {
    String modeStr = message.substring(12); // Extract the mode from the message
    modeStr.toLowerCase(); // Convert the mode string to lowercase
    Serial.print("Mode string received: "); // Debugging line
    Serial.println(modeStr); // Print the extracted mode string for debugging

    if (modeStr == "auto") {
        setPumpMode(PUMP_AUTO);
    } else if (modeStr == "on") {
        setPumpMode(PUMP_ON);
    } else if (modeStr == "off") {
        setPumpMode(PUMP_OFF);
    }
    broadcastPumpState(); // Confirm the change back to the client
}
}}


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    /* Your CSS styles here */
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  
  <!-- Pump 1 Control -->
  <label class="switch">
    <input type="checkbox" id="pump1" onchange="togglePump(1)">
    <span class="slider"></span>
  </label>
  
  <!-- Pump 1 State Display -->
  <p>Pump 1 State: <span id="pump1State">off</span></p>
  
  <!-- Pump 1 Mode Display -->
  <p>Pump 1 Mode: <span id="pump1Mode">auto</span></p>
  
  <select id="pumpModeSelect" onchange="changePumpMode()">
  <option value="Auto">Auto</option>
  <option value="On">On</option>
  <option value="Off">Off</option>
  
  
</select>

<script>
var ws; // Declare ws globally

function connectWebSocket() {
    console.log("Connecting to WebSocket...");
    ws = new WebSocket('ws://' + window.location.hostname + '/ws');
    ws.onopen = function(event) {
        console.log("WebSocket connection established.");
    };

    ws.onmessage = function(event) {
        console.log("WebSocket message received:", event.data);
        // Split the message by commas for multiple pieces of data
        var dataParts = event.data.split(',');
        dataParts.forEach(function(part) {
            var keyValue = part.split(':');
            var key = keyValue[0];
            var value = keyValue[1];
            
            // Update webpage elements based on the key
            if (key === 'pump1State') {
                document.getElementById("pump1State").textContent = value;
            } else if (key === 'pump1Mode') {
                document.getElementById("pump1Mode").textContent = value;
                // Also update the select dropdown to reflect the current mode
                var selectElement = document.getElementById("pumpModeSelect");
                if (selectElement) {
                    selectElement.value = value;
                }
            }
        });
    };
}

function changePumpMode() {
    var select = document.getElementById('pumpModeSelect');
    var mode = select.value;
    console.log('Changing pump mode to:', mode); // Add for debugging
    ws.send('setPumpMode:' + mode);
}


window.onload = function() {
    connectWebSocket();
};
</script>


</body>
</html>

)rawliteral";

void setup() {
  Serial.begin(115200);
  pinMode(pumpPin1, OUTPUT);
  
  // Set default mode to AUTO and apply initial conditions
  pump1Mode = PUMP_AUTO;
  checkAutoModeConditions(); // This will apply AUTO logic based on initial simulatedTemperature


  initWebSocket();
  
// Check for valid configuration and set the initial state of Pump 1
    if (!pump1OnStateHigh && pump1OffStateHigh) {
        // Valid configuration: ON = LOW, OFF = HIGH
        digitalWrite(pumpPin1, HIGH); // Sets Pump 1 to OFF, which is HIGH in this configuration
    } else if (pump1OnStateHigh && !pump1OffStateHigh) {
        // Valid configuration: ON = HIGH, OFF = LOW
        digitalWrite(pumpPin1, LOW); // Sets Pump 1 to OFF, which is LOW in this configuration
    } else if (pump1OnStateHigh == pump1OffStateHigh) {
        // Error: ON and OFF states are configured to the same level
        Serial.println("Error, Pump 1 configuration will leave output pin at the same level for both ON and OFF states.");
        if (pump1OnStateHigh && pump1OffStateHigh) {
            // Both ON and OFF are HIGH
            Serial.println("Both ON and OFF states are configured as HIGH for Pump 1.");
        } else {
            // Both ON and OFF are LOW
            Serial.println("Both ON and OFF states are configured as LOW for Pump 1.");
        }
    } else {
        // Catch-all for any other misconfiguration
        Serial.println("Misconfiguration detected in Pump 1 settings.");
    }

  // Setup other pins...

  SPI.begin();
  ETH.begin(W5500_MISO, W5500_MOSI, W5500_SCK, W5500_SS, W5500_INT, 25, SPI3_HOST, mac);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    handleWebSocketMessage(arg, data, len);
  });
  server.addHandler(&ws);

  server.begin();
broadcastPumpState();
}

void loop() {
  //static unsigned long lastToggleTimePump1 = 0;
  //if (millis() - lastToggleTimePump1 >= 10000) {
  //  togglePump1State();
  //  lastToggleTimePump1 = millis();
  //}
  
// Check for serial input to change simulated temperature
  if (Serial.available() > 0) {
    char command = Serial.read();
    // Increase temperature
    if (command == '+') {
      simulatedTemperature += 1.0;
      Serial.println("Simulated Temperature Increased: " + String(simulatedTemperature));
    }
    // Decrease temperature
    else if (command == '-') {
      simulatedTemperature -= 1.0;
      Serial.println("Simulated Temperature Decreased: " + String(simulatedTemperature));
    }
    // Check AUTO mode conditions based on updated temperature
    checkAutoModeConditions();
  }

  // Other loop activities...


}
