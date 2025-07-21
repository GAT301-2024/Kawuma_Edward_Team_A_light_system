// Required Libraries for ESP32 Web Server  
#include <WiFi.h> // For ESP32 Wi-Fi functionalities
#include <AsyncTCP.h> // Asynchronous TCP library, a dependency for ESPAsyncWebServer
#include <ESPAsyncWebServer.h> // Asynchronous Web Server library for ESP32

// --- Wi-Fi Configuration ---
// Define the SSID (network name) for your ESP32's Access Point
const char* ssid = "EDDIE 2919";
// Define the password for your ESP32's Access Point. IMPORTANT: Change this to a strong, unique password!
const char* password = "S2i+4603"; // <--- CHANGE THIS PASSWORD!

// --- LED Pin Definitions (Based on Schematic) ---
// These GPIO pins control the BASE of the NPN transistors that switch the LEDs.
const int LED1_CTRL_PIN = 18;  // GPIO18 → Q1 base
const int LED2_CTRL_PIN = 19;  // GPIO19 → Q2 base
const int LED3_CTRL_PIN = 21;  // GPIO21 → Q3 base

// --- LDR Pin Definition (Based on Schematic) ---
const int LDR_PIN = 34; // Connected to GPIO 34 for analog reading

// --- Automatic Control Settings ---
// Threshold for determining night/day based on LDR reading.
// Values range from 0 (darkest) to 4095 (brightest) for a 12-bit ADC.
// You will likely need to CALIBRATE this value for your specific LDR and environment.
// Lower value = darker for "night" detection.
const int NIGHT_THRESHOLD = 800; // Example: Below 800 is considered night. ADJUST THIS!

// Delay between automatic light checks (in milliseconds)
const long AUTO_CHECK_INTERVAL = 10000; // Check every 10 seconds

// --- Web Server Object ---
// Create an instance of the AsyncWebServer on port 80 (standard HTTP port)
AsyncWebServer server(80);

// --- LED State Variables ---
// Boolean variables to keep track of the current state of each LED (true for ON, false for OFF)
bool led1State = false;
bool led2State = false;
bool led3State = false;

// --- Automatic Mode State Variable ---
bool autoModeEnabled = false;

// --- Timing Variable for Automatic Control ---
unsigned long lastAutoCheckMillis = 0;

// --- Helper function to set LED state (turns transistor ON/OFF) ---
void setLED(int pin, bool state) {
  // Since we are driving NPN transistors in common-emitter configuration:
  // HIGH on base turns transistor ON -> LED ON
  // LOW on base turns transistor OFF -> LED OFF
  digitalWrite(pin, state ? HIGH : LOW);
}

// --- HTML Content for the Web Dashboard ---
String getDashboardHtml() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Intelligent Lighting Control - TEAM_A Project</title>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600&display=swap" rel="stylesheet">
    <style>
        :root {
            --primary: #4CAF50; /* Light green */
            --secondary: #2196F3; /* Blue */
            --success: #8BC34A; /* Light green */
            --danger: #F44336;
            --light: #f8f9fa;
            --dark: #343a40;
            --info: #00BCD4; /* Cyan */
        }
        
        /* Base Styles */
        body {
            font-family: 'Inter', sans-serif;
            background-color: #f5f7fa;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
            margin: 0;
            color: #333;
            line-height: 1.6;
        }
        
        /* Card Container */
        .dashboard-card {
            background-color: white;
            padding: 30px;
            border-radius: 12px;
            box-shadow: 0 10px 25px rgba(0,0,0,0.08);
            width: 95%;
            max-width: 600px;
            margin: 20px auto;
            position: relative;
            overflow: hidden;
        }
        
        .dashboard-card::before {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 8px;
            background: linear-gradient(90deg, var(--primary), var(--secondary));
        }
        
        /* Header Section */
        .header {
            text-align: center;
            margin-bottom: 30px;
            border-bottom: 1px solid #eee;
            padding-bottom: 20px;
        }
        
        .header h1 {
            color: var(--primary);
            margin: 0 0 10px 0;
            font-weight: 600;
            font-size: 1.8rem;
        }
        
        .header h2 {
            color: var(--secondary);
            margin: 0;
            font-weight: 500;
            font-size: 1.2rem;
        }
        
        /* Student Info Section */
        .student-info {
            background-color: #E8F5E9; /* Very light green */
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 25px;
            display: flex;
            flex-wrap: wrap;
            justify-content: space-between;
        }
        
        .info-item {
            margin: 5px 0;
            flex: 1 1 45%;
            min-width: 200px;
        }
        
        .info-label {
            font-weight: 500;
            color: var(--dark);
            display: block;
            margin-bottom: 3px;
            font-size: 0.85rem;
        }
        
        .info-value {
            color: var(--secondary);
            font-weight: 600;
        }
        
        /* WiFi Info */
        .wifi-info {
            background-color: #E3F2FD; /* Light blue */
            padding: 12px;
            border-radius: 8px;
            margin-bottom: 25px;
            display: flex;
            align-items: center;
        }
        
        .wifi-icon {
            font-size: 1.5rem;
            margin-right: 12px;
            color: var(--secondary);
        }
        
        .wifi-details {
            flex: 1;
        }
        
        .wifi-name {
            font-weight: 600;
            color: var(--secondary);
            margin-bottom: 3px;
        }
        
        .wifi-status {
            color: #666;
            font-size: 0.85rem;
        }
        
        /* Control Sections */
        .control-section {
            margin-bottom: 25px;
            padding: 15px;
            border: 1px solid #e0e0e0;
            border-radius: 10px;
            background-color: #F1F8E9; /* Light green */
        }
        
        .section-title {
            color: var(--secondary);
            margin-top: 0;
            margin-bottom: 15px;
            font-size: 1.3rem;
            display: flex;
            align-items: center;
        }
        
        .section-title i {
            margin-right: 10px;
            color: var(--primary);
        }
        
        /* Buttons */
        .btn {
            background-color: var(--primary);
            color: white;
            padding: 12px 25px;
            border: none;
            border-radius: 8px;
            font-size: 1rem;
            cursor: pointer;
            transition: all 0.3s ease;
            width: 100%;
            max-width: 250px;
            margin: 8px 0;
            display: inline-flex;
            align-items: center;
            justify-content: center;
        }
        
        .btn:hover {
            background-color: var(--secondary);
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(0,0,0,0.1);
        }
        
        .btn:active {
            transform: translateY(0);
        }
        
        .btn i {
            margin-right: 8px;
        }
        
        .btn.on {
            background-color: var(--success);
        }
        
        /* Status Indicators */
        .status-indicator {
            display: inline-block;
            width: 18px;
            height: 18px;
            border-radius: 50%;
            margin-left: 10px;
            vertical-align: middle;
        }
        
        .status-indicator.on {
            background-color: var(--success);
            box-shadow: 0 0 10px rgba(139, 195, 74, 0.5);
        }
        
        .status-indicator.off {
            background-color: var(--danger);
            box-shadow: 0 0 10px rgba(244, 67, 54, 0.5);
        }
        
        /* Footer */
        footer {
            margin-top: 30px;
            text-align: center;
            color: #666;
            font-size: 0.85rem;
            width: 100%;
        }
        
        /* Responsive Adjustments */
        @media (max-width: 480px) {
            .dashboard-card {
                padding: 20px;
            }
            
            .header h1 {
                font-size: 1.5rem;
            }
            
            .info-item {
                flex: 1 1 100%;
            }
        }
    </style>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0-beta3/css/all.min.css">
</head>
<body>
    <div class="dashboard-card">
        <div class="header">
            <h1>Intelligent Lighting Control System</h1>
            <h2>ESP32 Web Control Dashboard</h2>
        </div>
        
        <div class="student-info">
            <div class="info-item">
                <span class="info-label">Student Name:</span>
                <span class="info-value" id="studentName">Kawuma Edward</span>
            </div>
            <div class="info-item">
                <span class="info-label">Registration Number:</span>
                <span class="info-value" id="regNumber">24/U/0045/GCS</span>
            </div>
            <div class="info-item">
                <span class="info-label">TEAM_A Project:</span>
                <span class="info-value">Intelligent Lighting System</span>
            </div>
            <div class="info-item">
                <span class="info-label">Date:</span>
                <span class="info-value" id="currentDate">Loading...</span>
            </div>
        </div>
        
        <div class="wifi-info">
            <div class="wifi-icon">
                <i class="fas fa-wifi"></i>
            </div>
            <div class="wifi-details">
                <div class="wifi-name" id="wifiSSID">Not connected</div>
                <div class="wifi-status" id="wifiStatus">IP: Loading...</div>
            </div>
        </div>
        
        <div class="control-section">
            <h2 class="section-title"><i class="fas fa-robot"></i> Automatic Mode</h2>
            <button id="autoModeButton" class="btn" onclick="toggleAutoMode()">
                <i class="fas fa-magic"></i>  Automatic Mode
            </button>
            <span id="autoModeStatus" class="status-indicator off"></span>
            <p style="text-align: center; margin-top: 10px;">
                Light Sensor: <strong id="ldrValue">---</strong> (Threshold: <span id="thresholdValue">800</span>)
            </p>
        </div>
        
        <div class="control-section">
            <h2 class="section-title"><i class="fas fa-lightbulb"></i> Manual Control</h2>
            <div style="display: flex; flex-wrap: wrap; justify-content: center; gap: 10px;">
                <div style="text-align: center;">
                    <button id="led1Button" class="btn" onclick="toggleLED(1)">
                        <i class="fas fa-lightbulb"></i> LED 1
                    </button>
                    <span id="led1Status" class="status-indicator off"></span>
                </div>
                <div style="text-align: center;">
                    <button id="led2Button" class="btn" onclick="toggleLED(2)">
                        <i class="fas fa-lightbulb"></i> LED 2
                    </button>
                    <span id="led2Status" class="status-indicator off"></span>
                </div>
                <div style="text-align: center;">
                    <button id="led3Button" class="btn" onclick="toggleLED(3)">
                        <i class="fas fa-lightbulb"></i> LED 3
                    </button>
                    <span id="led3Status" class="status-indicator off"></span>
                </div>
            </div>
        </div>
    </div>
    
    <footer>
        <p>© <span id="currentYear"></span> Intelligent Lighting Control System - TEAM_A Project</p>
    </footer>

    <script>
        // Set current date and year
        const now = new Date();
        document.getElementById('currentDate').textContent = now.toLocaleDateString();
        document.getElementById('currentYear').textContent = now.getFullYear();
        
        // Function to update WiFi information
        function updateWifiInfo(ssid, ip) {
            const wifiSSID = document.getElementById('wifiSSID');
            const wifiStatus = document.getElementById('wifiStatus');
            
            if (ssid) {
                wifiSSID.textContent = ssid;
                wifiStatus.textContent = `IP: ${ip || 'Not available'}`;
            } else {
                wifiSSID.textContent = "Not connected";
                wifiStatus.textContent = "Connect to ESP32 AP";
            }
        }
        
        // Function to send a request to the ESP32 to toggle an LED
        async function toggleLED(ledNum) {
            const button = document.getElementById(`led${ledNum}Button`);
            const statusIndicator = document.getElementById(`led${ledNum}Status`);

            try {
                const response = await fetch(`/led${ledNum}/toggle`);
                const data = await response.text();
                console.log(`Response for LED${ledNum}:`, data);
                updateUI(ledNum, data.includes("ON"));
            } catch (error) {
                console.error('Error toggling LED:', error);
                alert('Connection error. Please check your WiFi connection.');
            }
        }

        // Function to toggle Automatic Mode
        async function toggleAutoMode() {
            const button = document.getElementById('autoModeButton');
            const statusIndicator = document.getElementById('autoModeStatus');

            try {
                const response = await fetch('/automode/toggle');
                const data = await response.json();
                console.log('Auto Mode Toggled:', data);
                updateAutoModeUI(data.autoModeEnabled);
            } catch (error) {
                console.error('Error toggling Auto Mode:', error);
                alert('Connection error. Please check your WiFi connection.');
            }
        }

        // Function to fetch the current status from ESP32
        async function updateAllStatus() {
            try {
                const response = await fetch('/status');
                const data = await response.json();
                console.log("Current System Status:", data);

                updateUI(1, data.led1);
                updateUI(2, data.led2);
                updateUI(3, data.led3);
                updateAutoModeUI(data.autoModeEnabled);
                document.getElementById('ldrValue').textContent = data.ldrValue;
                
                // Update WiFi info if available in response
                if (data.wifiSSID) {
                    updateWifiInfo(data.wifiSSID, data.ipAddress);
                }

            } catch (error) {
                console.error('Error fetching system status:', error);
                updateWifiInfo(null);
            }
        }

        // Helper function to update LED UI
        function updateUI(ledNum, state) {
            const button = document.getElementById(`led${ledNum}Button`);
            const statusIndicator = document.getElementById(`led${ledNum}Status`);

            if (state) {
                button.classList.add('on');
                statusIndicator.className = 'status-indicator on';
            } else {
                button.classList.remove('on');
                statusIndicator.className = 'status-indicator off';
            }
        }

        // Helper function to update Auto Mode UI
        function updateAutoModeUI(state) {
            const button = document.getElementById('autoModeButton');
            const statusIndicator = document.getElementById('autoModeStatus');

            if (state) {
                button.classList.add('on');
                button.innerHTML = '<i class="fas fa-magic"></i> Automatic Mode: ON';
                statusIndicator.className = 'status-indicator on';
            } else {
                button.classList.remove('on');
                button.innerHTML = '<i class="fas fa-magic"></i> Automatic Mode: OFF';
                statusIndicator.className = 'status-indicator off';
            }
        }

        // Initialize on load
        window.onload = function() {
            updateAllStatus();
            setInterval(updateAllStatus, 3000); // Refresh every 3 seconds
        };
    </script>
</body>
</html>
  )rawliteral";
  return html;
}

// --- Arduino Setup Function ---
void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting ESP32 Intelligent Lighting System...");

  // Set LED control pins as OUTPUTs
  pinMode(LED1_CTRL_PIN, OUTPUT);
  pinMode(LED2_CTRL_PIN, OUTPUT);
  pinMode(LED3_CTRL_PIN, OUTPUT);
  
  // Set LDR pin as INPUT (implicitly done for analogRead, but good practice)
  pinMode(LDR_PIN, INPUT);

  // Initialize all LEDs to OFF state
  setLED(LED1_CTRL_PIN, LOW);
  setLED(LED2_CTRL_PIN, LOW);
  setLED(LED3_CTRL_PIN, LOW);
  led1State = false;
  led2State = false;
  led3State = false;

  // Start the ESP32 in Access Point (AP) mode
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point (AP) IP address: ");
  Serial.println(IP);
  Serial.print("Connect to Wi-Fi network: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.println("Then open a web browser and go to the IP address above.");

  // --- Web Server Route Handlers ---

  // Route for the root URL ("/") - serves the main HTML dashboard
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Client requested root URL '/'");
    request->send(200, "text/html", getDashboardHtml());
  });

  // Route to toggle LED 1
  server.on("/led1/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    led1State = !led1State;
    setLED(LED1_CTRL_PIN, led1State);
    Serial.printf("LED 1 toggled to: %s\n", led1State ? "ON" : "OFF");
    request->send(200, "text/plain", led1State ? "LED1_ON" : "LED1_OFF");
  });

  // Route to toggle LED 2
  server.on("/led2/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    led2State = !led2State;
    setLED(LED2_CTRL_PIN, led2State);
    Serial.printf("LED 2 toggled to: %s\n", led2State ? "ON" : "OFF");
    request->send(200, "text/plain", led2State ? "LED2_ON" : "LED2_OFF");
  });

  // Route to toggle LED 3
  server.on("/led3/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    led3State = !led3State;
    setLED(LED3_CTRL_PIN, led3State);
    Serial.printf("LED 3 toggled to: %s\n", led3State ? "ON" : "OFF");
    request->send(200, "text/plain", led3State ? "LED3_ON" : "LED3_OFF");
  });

  // Route to toggle Automatic Mode
  server.on("/automode/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    autoModeEnabled = !autoModeEnabled;
    Serial.printf("Automatic Mode toggled to: %s\n", autoModeEnabled ? "ENABLED" : "DISABLED");
    String jsonResponse = "{ \"autoModeEnabled\": " + String(autoModeEnabled ? "true" : "false") + " }";
    request->send(200, "application/json", jsonResponse);
  });

  // Route to get the current status of all LEDs, Auto Mode, and LDR value as JSON
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    // Serial.println("Client requested system status '/status'"); // Uncomment for more verbose logging
    int ldrValue = analogRead(LDR_PIN); // Read LDR value
    
    String jsonResponse = "{";
    jsonResponse += "\"led1\":" + String(led1State ? "true" : "false") + ",";
    jsonResponse += "\"led2\":" + String(led2State ? "true" : "false") + ",";
    jsonResponse += "\"led3\":" + String(led3State ? "true" : "false") + ",";
    jsonResponse += "\"autoModeEnabled\":" + String(autoModeEnabled ? "true" : "false") + ",";
    jsonResponse += "\"ldrValue\":" + String(ldrValue);
    jsonResponse += "}";
    request->send(200, "application/json", jsonResponse);
  });

  // Start the web server
  server.begin();
  Serial.println("Web server started.");
}

// --- Arduino Loop Function ---
// This function runs repeatedly after setup()
void loop() {
  // Check for automatic light control if enabled
  if (autoModeEnabled) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastAutoCheckMillis >= AUTO_CHECK_INTERVAL) {
      lastAutoCheckMillis = currentMillis;

      int ldrValue = analogRead(LDR_PIN);
      Serial.printf("LDR Value: %d, Threshold: %d\n", ldrValue, NIGHT_THRESHOLD);

      if (ldrValue < NIGHT_THRESHOLD) { // It's dark (LDR value is low)
        Serial.println("It's NIGHT - Turning ALL LEDs ON automatically.");
        if (!led1State) { led1State = true; setLED(LED1_CTRL_PIN, HIGH); }
        if (!led2State) { led2State = true; setLED(LED2_CTRL_PIN, HIGH); }
        if (!led3State) { led3State = true; setLED(LED3_CTRL_PIN, HIGH); }
      } else { // It's bright (LDR value is high)
        Serial.println("It's DAY - Turning ALL LEDs OFF automatically.");
        if (led1State) { led1State = false; setLED(LED1_CTRL_PIN, LOW); }
        if (led2State) { led2State = false; setLED(LED2_CTRL_PIN, LOW); }
        if (led3State) { led3State = false; setLED(LED3_CTRL_PIN, LOW); }
      }
    }
  }
}
