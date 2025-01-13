#pragma once
#include <Preferences.h>

// Some config is kept in a key/value store in the NVS partition
Preferences preferences;

// A struct to hold flags, some of which trigger actions in the main loop
struct Flags
{
  bool firstConnect : 1;     // false until the first WiFi connect
  bool doConnectActions : 1; // true each time WiFi connects
  bool restartPending : 1;   // signals the firmware to reboot (not for OTA)
  bool wifiConnected : 1;    // true when WiFi is connected
  bool serialTelemetry : 1;  // enable serial telemetry
  bool udpTelemetry : 1;     // enable UDP telemetry
} flags;

// Edit this function to set your preferences, upload, then undo the edit.
// Your preferences will be saved in the NVS partition, so will persist
// across reboots, firmware updates, and even flashing other sketches. They are
// lost when the NVS partition is erased or the partition table layout changed.
void storePreferences()
{
  // preferences.clear(); // uncomment this line to clear all stored preferences

  return; // comment out this line to set your preferences
  preferences.putString("wifi_ssid", "your_ssid");
  preferences.putString("wifi_password", "your_password");
  preferences.putString("wifi_hostname", "esp32-s3-playpen");
  preferences.putString("ntp_server1", "time.cloudflare.com");
  preferences.putUInt("baudrate", 115200);
  preferences.putBool("serialTelemetry", false); // send stats via Serial
  preferences.putBool("udpTelemetry", true);     // send stats via UDP
  preferences.putUInt("telemetry_port", 47269);  // port your PC listens on
  preferences.putString("telemetry_host", "your_pc_ip_address");

  // Find the string for your timezone here:
  //   https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  String timezone = "GMT0BST,M3.5.0/1,M10.5.0"; // UK
  // String timezone = "WET0WEST,M3.5.0/1,M10.5.0";     // Western Europe
  // String timezone = "CET-1CEST,M3.5.0,M10.5.0/3";    // Central Europe
  // String timezone = "EET-2EEST,M3.5.0/3,M10.5.0/4";  // Eastern Europe
  // String timezone = "JST-9";                         // Japan
  // String timezone = "EST5EDT,M3.2.0,M11.1.0";        // US Eastern
  // String timezone = "CST6CDT,M3.2.0,M11.1.0";        // US Central
  // String timezone = "MST7MDT,M3.2.0,M11.1.0";        // US Mountain
  // String timezone = "PST8PDT,M3.2.0,M11.1.0";        // US Pacific
  preferences.putString("timezone", timezone);
}

void preferencesBegin()
{
  preferences.begin("FastLED-playpen");
  storePreferences();
  flags.serialTelemetry = preferences.getBool("serialTelemetry", true);
  flags.udpTelemetry = preferences.getBool("udpTelemetry", false);
}
