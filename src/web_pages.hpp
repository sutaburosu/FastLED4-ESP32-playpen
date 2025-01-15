#pragma once
#include "preferences.hpp"
#include "wifi.hpp"
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

const char* indexContent = 
/* html */
R"raw_literal_html(
<!DOCTYPE html>
<html>
<link rel="icon" href="favicon.ico" />
<head>
  <title>Playpen</title>
</head>
<body>
  <h1>ESP32-S3 FastLED 4 Playpen</h1>
  <button id="restart">Restart</button>
  <button id="telemetryon">Serial telemetry on</button>
  <button id="telemetryoff">Serial telemetry off</button>
  <button id="UDPtelemetryon">UDP telemetry on</button>
  <button id="UDPtelemetryoff">UDP telemetry off</button>
  <button id="updateButton">OTA Update</button>
  <div id="responsediv"></div>
  <div id="updatediv">
  <iframe id="updateframe">
  </iframe></div>
</body>
</html>
)raw_literal_html"
/* css */
R"raw_literal_css(
<style>
  body {
    font-family: Arial, sans-serif;
    background-color: #f0f0f0;
    margin: 0;
    padding: 0;
  }
  h1 {
    color: #202020;
  }
  button {
    background-color: #45a049;
    color: white;
    padding: 10px 20px;
    margin: 10px;
    border: none;
    cursor: pointer;
  }
  button:hover {
    background-color: #75d089;
  }
  #responsediv {
    transition: all 0.5s ease-in-out;
    height: 0px;
    margin-top: 0px;
    font-size: 14px;
    color: #000000;
    background-color: #e0e0e0;
    padding: 0px;
    border: none;
    overflow: hidden !important;
  }
  #updatediv {
    transition: all 0.5s ease-in-out;
    margin-top: 0px;
    padding: 0px;
    border: none;
    width: 500px;
    height: 0px;
    overflow: hidden !important;
  }
  #updateframe {
    width: 100%;
    height: 100%;
    margin-top: 0px;
    padding: 0px;
    border-radius: 5px;
  }
</style>
)raw_literal_css"
/* javascript */
R"raw_literal_js(
<script>
  function sendRequest(endpoint) {
    const responseDiv = document.getElementById('responsediv');
    function hideResponse() {
      setTimeout(() => {
        responseDiv.innerText = '';
        responseDiv.style.height = '0px';
      }, 1500);
    }

    fetch(endpoint)
      .then(response => response.text())
      .then(data => {
        responseDiv.innerText = data;
        responseDiv.style.height = 'auto';
        hideResponse();
      })
      .catch(error => console.error('Error:', error));
  }

  document.addEventListener('DOMContentLoaded', (event) => {
    document.getElementById('restart').onclick = () => sendRequest('/restart');
    document.getElementById('telemetryon').onclick = () => sendRequest('/telemetryon');
    document.getElementById('telemetryoff').onclick = () => sendRequest('/telemetryoff');
    document.getElementById('UDPtelemetryon').onclick = () => sendRequest('/UDPtelemetryon');
    document.getElementById('UDPtelemetryoff').onclick = () => sendRequest('/UDPtelemetryoff');
    document.getElementById('updateButton').onclick = () => {
      const updateDiv = document.getElementById('updatediv');
      if (updateDiv.style.height != '400px') {
        updateDiv.style.height = '400px';
        const iframe = document.getElementById('updateframe');
        if (iframe.src === '') {
          iframe.src = '/update';
        }
      } else {
        updateDiv.style.height = '0px';
      }
    };
  });
</script>
)raw_literal_js";


void setupWebServer() {
  // Set up web server and OTA updates
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", indexContent); });
  server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request)
            { flags.restartPending = true;
              request->send(200, "text/plain", "Restarting..."); });
  server.on("/telemetryon", HTTP_GET, [](AsyncWebServerRequest *request)
            { flags.serialTelemetry = true;
              preferences.putBool("serialTelemetry", true);
              request->send(200, "text/plain", "Serial telemetry on"); });
  server.on("/telemetryoff", HTTP_GET, [](AsyncWebServerRequest *request)
            { flags.serialTelemetry = false;
              preferences.putBool("serialTelemetry", false);
              request->send(200, "text/plain", "Serial telemetry off"); });
  server.on("/UDPtelemetryon", HTTP_GET, [](AsyncWebServerRequest *request)
            { flags.udpTelemetry = true;
              preferences.putBool("udpTelemetry", true);
              request->send(200, "text/plain", "UDP telemetry on"); });
  server.on("/UDPtelemetryoff", HTTP_GET, [](AsyncWebServerRequest *request)
            { flags.udpTelemetry = false;
              preferences.putBool("udpTelemetry", false);
              request->send(200, "text/plain", "UDP telemetry off"); });

  ElegantOTA.begin(&server);
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
  server.begin();
}