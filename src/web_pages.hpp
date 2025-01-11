#pragma once

const char* indexContent = 
/* html */
R"raw_literal_html(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32-S3 FastLED 4 Playpen</title>
</head>
<body>
  <h1>Control Panel</h1>
  <button onclick="location.href='/restart'">Restart</button>
  <button onclick="location.href='/telemetryon'">Serial telemetry on</button>
  <button onclick="location.href='/telemetryoff'">Serial telemetry off</button>
  <button onclick="location.href='/UDPtelemetryon'">UDP telemetry on</button>
  <button onclick="location.href='/UDPtelemetryoff'">UDP telemetry off</button>
  <button id="updateButton">OTA Update</button>
  <div id="response" style="border: 2px solid #4CAF50; padding: 10px; border-radius: 5px;"></div>
  <div id="update" style="display:none; width:75%; height:650px; padding: 10px; border: 2px solid #4CAF50; border-radius: 5px;">
  <iframe id="updateFrame">
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
    color: #333;
  }
  button {
    background-color: #4CAF50;
    color: white;
    padding: 10px 20px;
    margin: 10px;
    border: none;
    cursor: pointer;
  }
  button:hover {
    background-color: #45a049;
  }
  #response {
    margin-top: 20px;
    font-size: 14px;
    color: #333;
    background-color: #e0e0e0;
    padding: 10px;
    border-radius: 5px;
    border: 1px solid #ccc;
  }
  #updateFrame {
    margin-top: 20px;
  }
</style>
)raw_literal_css"
/* javascript */
R"raw_literal_js(
<script>
  function sendRequest(endpoint) {
    fetch(endpoint)
      .then(response => response.text())
      .then(data => {
        console.log(data);
        document.getElementById('response').innerText = 'Response from ' + endpoint + ': ' + data;
      })
      .catch(error => console.error('Error:', error));
  }

  document.addEventListener('DOMContentLoaded', (event) => {
    document.querySelector('button[onclick="location.href=\'/restart\'"]').onclick = () => sendRequest('/restart');
    document.querySelector('button[onclick="location.href=\'/telemetryon\'"]').onclick = () => sendRequest('/telemetryon');
    document.querySelector('button[onclick="location.href=\'/telemetryoff\'"]').onclick = () => sendRequest('/telemetryoff');
    document.querySelector('button[onclick="location.href=\'/UDPtelemetryon\'"]').onclick = () => sendRequest('/UDPtelemetryon');
    document.querySelector('button[onclick="location.href=\'/UDPtelemetryoff\'"]').onclick = () => sendRequest('/UDPtelemetryoff');
    document.getElementById('updateButton').onclick = () => {
      const iframe = document.getElementById('updateFrame');
      if (iframe.style.display === 'none') {
        iframe.style.display = 'block';
        iframe.src = '/update';
      } else {
        iframe.style.display = 'none';
      }
    };
  });
</script>
)raw_literal_js";
