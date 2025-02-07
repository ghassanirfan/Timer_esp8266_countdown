#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>  // Tambahkan library mDNS

const char* ssid = "kebunlombok";
const char* password = "abc12345";

// IPAddress IP(192,168,0,123);
// IPAddress NETMASK(255,255,255,0);
// IPAddress NETWORK(192,168,0,1);
// IPAddress DNS(192,168,1,1);

ESP8266WebServer server(80);

int relay1Pin = D1; // Pump Feeding Active-Low
int relay2Pin = D2; // Pump Reversal Active-Low
unsigned long relay1Duration = 0;
unsigned long relay2Duration = 0;
unsigned long relayPauseDuration = 0;
unsigned long relay1StartTime = 0;
unsigned long relay2StartTime = 0;
unsigned long relayPauseStartTime = 0;
bool relay1Active = false;
bool relay2Active = false;
bool relayPauseActive = false;

void handleRoot() {
  String html = "<html><head><style>";
  html += "body {font-family: Arial, sans-serif; text-align: center;}";
  html += ".container {max-width: 400px;margin: 0 auto;padding: 20px;border: 1px solid #ccc;border-radius: 10px;box-shadow: 0px 0px 10px 0px #ccc;}";
  html += ".input-group {margin-bottom: 15px;}";
  html += ".input-group input {width: 100%;padding: 8px;font-size: 16px;border: 1px solid #ccc;border-radius: 5px;}";
  html += ".btn {background-color: #4CAF50;color: white;padding: 10px 20px;border: none;cursor: pointer;border-radius: 5px;}";
  html += ".btn:hover {background-color: #45a049;}";
  html += ".status {margin-top: 20prx;font-size: 18px;}";
  html += ".progress-bar {width: 100%;height: 30px;border: 1px solid #ccc;border-radius: 5px;margin-top: 10px;}";
  html += ".progress-bar-inner {height: 100%;text-align: center;line-height: 30px;color: white;border-radius: 5px;background-color: #4CAF50;}";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>Timer Pompa Universal</h1>";
  html += "<h3>Design and Program By Ghassan </h3>";
  html += "<h4>Untuk Kebun maupun Lainnya</h4>";
  html += "<div class='input-group'>Durasi Pompa 1 (minutes): <input type='number' id='relay1Duration'></div>";
  html += "<div class='input-group'>Durasi Pompa 2 (minutes): <input type='number' id='relay2Duration'></div>";
  html += "<div class='input-group'>Waktu tunggu sebelum pompa 1 (minutes): <input type='number' id='relayPauseDuration'></div>";
  html += "<button class='btn' onclick='startRelays()'>Start</button>";
  html += "<div class='status'>Status Pompa 1: <span id='statusText1'></span><span id='timeInput1'></span><div class='progress-bar'><div id='progressBar1' class='progress-bar-inner'></div></div></div>";
  html += "<div class='status'>Status Pompa 2: <span id='statusText2'></span><span id='timeInput2'></span><div class='progress-bar'><div id='progressBar2' class='progress-bar-inner'></div></div></div>";
  html += "<div class='status'>Waktu tunggu: <span id='timeInputPause'></span><div class='progress-bar'><div id='progressBarPause' class='progress-bar-inner'></div></div></div>";
  html += "</div><script>"
          "function startRelays() {"
          "var relay1Duration = document.getElementById('relay1Duration').value;"
          "var relay2Duration = document.getElementById('relay2Duration').value;"
          "var relayPauseDuration = document.getElementById('relayPauseDuration').value;"
          "var xhr = new XMLHttpRequest();"
          "xhr.open('GET', '/start?relay1Duration=' + relay1Duration + '&relay2Duration=' + relay2Duration + '&relayPauseDuration=' + relayPauseDuration, true);"
          "xhr.send();"
          "location.reload();"
          "}"
          "function updateProgress(elementId, remainingTime, totalTime) {"
          "var progress = 100 - (remainingTime / totalTime) * 100;"
          "document.getElementById(elementId).style.width = progress + '%';"
          "document.getElementById(elementId).textContent = progress.toFixed(2) + '%';"
          "}"
          "function updateStatus(elementId, status) {"
          "var statusText = (status) ? 'Active' : 'Inactive';"
          "document.getElementById('statusText' + elementId).textContent = statusText;"
          "}"
          "function updateTimeInput(elementId, time) {"
          "var minutes = Math.floor(time / 60);"
          "document.getElementById('timeInput' + elementId).textContent = ' (Input: ' + minutes + ' minutes)';"
          "}"
          "setInterval(function() {"
          "var xhr = new XMLHttpRequest();"
          "xhr.open('GET', '/getRemainingTime', true);"
          "xhr.onreadystatechange = function() {"
          "if (xhr.readyState == 4 && xhr.status == 200) {"
          "var remainingTimes = JSON.parse(xhr.responseText);"
          "var totalTime1 = " + String(relay1Duration) + ";"
          "var totalTime2 = " + String(relay2Duration) + ";"
          "var totalTimePause = " + String(relayPauseDuration) + ";"
          "updateProgress('progressBar1', remainingTimes.relay1, totalTime1);"
          "updateProgress('progressBar2', remainingTimes.relay2, totalTime2);"
          "updateProgress('progressBarPause', remainingTimes.pause, totalTimePause);"
          "updateStatus(1, remainingTimes.relay1 > 0);"
          "updateStatus(2, remainingTimes.relay2 > 0);"
          "updateTimeInput(1, totalTime1 / 1000);"
          "updateTimeInput(2, totalTime2 / 1000);"
          "updateTimeInput('Pause', totalTimePause / 1000);"
          "}"
          "};"
          "xhr.send();"
          "}, 1000);"
          "</script></body></html>";
  server.send(200, "text/html", html);
}

void handleRemainingTime() {
  unsigned long currentMillis = millis();

  unsigned long relay1Remaining = 0;
  unsigned long relay2Remaining = 0;
  unsigned long relayPauseRemaining = 0;

  if (relay1Active) {
    relay1Remaining = (currentMillis >= relay1StartTime) ? (relay1Duration - (currentMillis - relay1StartTime)) : relay1Duration;
  }
  if (relay2Active) {
    relay2Remaining = (currentMillis >= relay2StartTime) ? (relay2Duration - (currentMillis - relay2StartTime)) : relay2Duration;
  }
  if (relayPauseActive) {
    relayPauseRemaining = (currentMillis >= relayPauseStartTime) ? (relayPauseDuration - (currentMillis - relayPauseStartTime)) : relayPauseDuration;
  }

  String json = "{\"relay1\": " + String(relay1Remaining) +
                ", \"relay2\": " + String(relay2Remaining) +
                ", \"pause\": " + String(relayPauseRemaining) + "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  pinMode(relay1Pin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);

  // Membaca pengaturan waktu dari EEPROM
  EEPROM.begin(512);
  EEPROM.get(0, relay1Duration);
  EEPROM.get(sizeof(unsigned long), relay2Duration);
  EEPROM.get(2 * sizeof(unsigned long), relayPauseDuration);
  EEPROM.end();

  digitalWrite(relay1Pin, HIGH);
  digitalWrite(relay2Pin, HIGH);

  // Memeriksa apakah pengaturan waktu tersimpan di EEPROM dan memulai siklus jika iya
  if (relay1Duration > 0 && relay2Duration > 0 && relayPauseDuration > 0) {
    relay1StartTime = millis();
    relay2StartTime = millis();
    relayPauseStartTime = millis();
    relay1Active = true;
    relay2Active = false;
    relayPauseActive = false;
    digitalWrite(relay1Pin, LOW); // Aktifkan relay 1
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point started. IP address: ");
  Serial.println(IP);

  if (MDNS.begin("kebunlombok")) {  // Inisialisasi mDNS dengan nama host "my-esp8266"
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/getRemainingTime", HTTP_GET, handleRemainingTime);
  server.on("/start", HTTP_GET, [](){
    relay1Duration = server.arg("relay1Duration").toInt() * 60000;  // Ubah menit ke milidetik
    relay2Duration = server.arg("relay2Duration").toInt() * 60000;  // Ubah menit ke milidetik
    relayPauseDuration = server.arg("relayPauseDuration").toInt() * 60000;  // Ubah menit ke milidetik

    // Menyimpan pengaturan waktu ke EEPROM
    EEPROM.begin(512);
    EEPROM.put(0, relay1Duration);
    EEPROM.put(sizeof(unsigned long), relay2Duration);
    EEPROM.put(2 * sizeof(unsigned long), relayPauseDuration);
    EEPROM.commit();
    EEPROM.end();

    relay1StartTime = millis();
    relay2StartTime = millis();
    relayPauseStartTime = millis();

    relay1Active = true;
    relay2Active = false;
    relayPauseActive = false;

    digitalWrite(relay1Pin, LOW); // Aktifkan relay 1
    
    server.send(200, "text/plain", "Process completed");
  });

  server.begin();
}

void loop() {
  server.handleClient();
  MDNS.update();  // Menambahkan ini untuk memperbarui layanan mDNS
  
  // Logika untuk mengatur siklus relay
  unsigned long currentMillis = millis();

  if (relay1Active && currentMillis - relay1StartTime >= relay1Duration) {
    relay1Active = false;
    digitalWrite(relay1Pin, HIGH); // Matikan relay 1
    digitalWrite(relay2Pin, LOW); // Aktifkan relay 2
    relay2Active = true;
    relay2StartTime = currentMillis;
  }

  if (relay2Active && currentMillis - relay2StartTime >= relay2Duration) {
    relay2Active = false;
    digitalWrite(relay2Pin, HIGH); // Matikan relay 2
    relayPauseActive = true;
    relayPauseStartTime = currentMillis;
  }

  if (relayPauseActive && currentMillis - relayPauseStartTime >= relayPauseDuration) {
    relayPauseActive = false;
    digitalWrite(relay1Pin, LOW); // Aktifkan relay 1
    relay1Active = true;
    relay1StartTime = currentMillis;
  }
}
