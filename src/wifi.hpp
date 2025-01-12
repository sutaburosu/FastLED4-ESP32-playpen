#pragma once
#include <WiFi.h>
#include <esp_sntp.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include "preferences.hpp"
#include "telemetry.hpp"

void ntpSetup();
void logNetworkDetails();
void ntpCallback(timeval *tv);
void onOTAStart();
void onOTAProgress(size_t current, size_t final);
void onOTAEnd(bool success);
void wifiEvent(WiFiEvent_t event, WiFiEventInfo_t info);
const char *wifiEventName(WiFiEvent_t event);
const char *wifiAuthModeName(wifi_auth_mode_t mode);
AsyncWebServer server(80);


void setupWiFi()
{
  if ((! preferences.isKey("wifi_ssid")) ||
      (! preferences.isKey("wifi_password")) ||
      (! preferences.isKey("wifi_hostname")))
  {
    auto oldtimeout = Serial.getTimeout();
    Serial.setTimeout(45000);
    Serial.println("WiFi credentials not found in preferences\n"
                   "Please enter the following information to connect to your WiFi network.\n");
    Serial.print("Hostname: ");
    String hostname = Serial.readStringUntil('\n');
    hostname.trim();
    if (!hostname.length()) {
      Serial.setTimeout(oldtimeout);
      return;
    }
    Serial.println(hostname);

    Serial.print("AP SSID: ");
    String ssid = Serial.readStringUntil('\n');
    ssid.trim();
    Serial.println(ssid);

    Serial.print("Password: ");
    String password = Serial.readStringUntil('\n');
    password.trim();
    Serial.println(password);

    if (ssid.length() && password.length() && hostname.length())
    {
      Serial.println("Storing WiFi credentials to preferences");
      preferences.putString("wifi_hostname", hostname);
      preferences.putString("wifi_ssid", ssid);
      preferences.putString("wifi_password", password);
    }
    Serial.setTimeout(oldtimeout);
  }

  WiFi.onEvent(wifiEvent);
  WiFi.setHostname(preferences.getString("wifi_hostname").c_str());
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(preferences.getString("wifi_ssid").c_str(),
             preferences.getString("wifi_password").c_str());
}

// Log current network details
void logNetworkDetails()
{
  telemetry.update("WiFi IP", WiFi.localIP().toString(), "", "t,np", 1000, 120000);
  telemetry.update("WiFi Gateway", WiFi.gatewayIP().toString(), "", "t,np", 1000, 121100);
  telemetry.update("WiFi Mask", WiFi.subnetMask().toString(), "", "t,np", 1000, 122200);
  telemetry.update("WiFi DNS1", WiFi.dnsIP().toString(), "", "t,np", 1000, 123300);
  telemetry.update("WiFi DNS2", WiFi.dnsIP(1).toString(), "", "t,np", 1000, 124400);
  telemetry.update("WiFi Hostname", String(WiFi.getHostname()), "", "t,np", 1000, 125500);
  telemetry.update("WiFi MAC", WiFi.macAddress(), "", "t,np", 1000, 126600);
}

// Set NTP time sync and timezone
#if !defined(NTP_SERVER1)
#define NTP_SERVER1 "\"time.cloudflare.com\""
#endif
void ntpSetup()
{
  String ntp1 = preferences.getString("ntp_server1", NTP_SERVER1);
  const char *ntp1_str = ntp1.c_str();

  ip_addr_t ntp;
  // Check if the NTP server is an IPv4/6 address or hostname
  if (ip4addr_aton(ntp1_str, &ntp.u_addr.ip4))
    ntp.type = IPADDR_TYPE_V4, esp_sntp_setserver(0, &ntp);
  else if (ip6addr_aton(ntp1_str, &ntp.u_addr.ip6))
    ntp.type = IPADDR_TYPE_V6, esp_sntp_setserver(0, &ntp);
  else
  {
    // Serial.printf("Resolving NTP server %s\n", ntp1_str);
    struct hostent *he = gethostbyname(ntp1_str);
    if (he != NULL)
    {
      ntp.u_addr.ip4.addr = ((struct in_addr *)(he->h_addr))->s_addr;
      ntp.type = IPADDR_TYPE_V4;
      // Serial.printf("NTP server %s resolved to %s\n", ntp1_str, ipaddr_ntoa(&ntp));
      esp_sntp_setserver(0, &ntp);
    }
    else
    {
      Serial.printf("Failed to resolve NTP server %s\n", ntp1_str);
    }
  }
  const ip_addr_t *ntp_ip = esp_sntp_getserver(0);
  if (ntp_ip->type == IPADDR_TYPE_V4)
    telemetry.update("NTP server", String(ipaddr_ntoa(ntp_ip)),
                     "", "t,np", 1000, 900000, false);
  else if (ntp_ip->type == IPADDR_TYPE_V6)
    telemetry.update("NTP server", String(ip6addr_ntoa(&ntp_ip->u_addr.ip6)),
                     "", "t,np", 1000, 900000, false);
  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
  sntp_set_sync_interval(900000); // 15 minutes
  esp_sntp_set_time_sync_notification_cb(ntpCallback);
  esp_sntp_init();

  // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  String tz = preferences.getString("timezone", "GMT0");
  const char *tz_str = tz.c_str();
  setenv("TZ", tz_str, 1);
  tzset();
}

void ntpCallback(timeval *tv)
{
  telemetry.update("NTP updated", timeString(), "", "t,np", 1000, 909909, false);
}

void onOTAStart()
{
  telemetry.update("OTA progress", "0%", "", "t", 250, 908908);
  // brightness.setValue(4);
}

void onOTAProgress(size_t current, size_t final)
{
  static int percent = 255;
  const int step = 1;
  int new_percent = (current * 100llu / final / step) * step;
  if (new_percent != percent)
  {
    percent = new_percent;
    telemetry.update("OTA progress", String(new_percent) + "%" + " of "
                     + String(final) + " bytes");
  }
}

void onOTAEnd(bool success)
{
  if (success) {
    preferences.end();
    telemetry.update("OTA progress", "100\% success");
  }
  else
    telemetry.update("OTA progress", "Failed");
}

// React to, and print information about, a WiFi event
void wifiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
{
  bool prev_wifi_connected = flags.wifiConnected;
  Serial.printf("WiFi %s ", wifiEventName(event));
  switch (event)
  {
  case ARDUINO_EVENT_WIFI_STA_CONNECTED:
    Serial.printf("'%s' BSSID: %02x:%02x:%02x:%02x:%02x:%02x"
                  " Channel: %d Auth mode: %s AuthID: %x",
                  info.wifi_sta_connected.ssid,
                  info.wifi_sta_connected.bssid[0],
                  info.wifi_sta_connected.bssid[1],
                  info.wifi_sta_connected.bssid[2],
                  info.wifi_sta_connected.bssid[3],
                  info.wifi_sta_connected.bssid[4],
                  info.wifi_sta_connected.bssid[5],
                  info.wifi_sta_connected.channel,
                  wifiAuthModeName(info.wifi_sta_connected.authmode),
                  info.wifi_sta_connected.aid);
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    flags.wifiConnected = false;
    Serial.printf("'%s' BSSID: %02x:%02x:%02x:%02x:%02x:%02x"
                  " Reason: %d RSSI: %d",
                  info.wifi_sta_disconnected.ssid,
                  info.wifi_sta_disconnected.bssid[0],
                  info.wifi_sta_disconnected.bssid[1],
                  info.wifi_sta_disconnected.bssid[2],
                  info.wifi_sta_disconnected.bssid[3],
                  info.wifi_sta_disconnected.bssid[4],
                  info.wifi_sta_disconnected.bssid[5],
                  info.wifi_sta_disconnected.reason,
                  info.wifi_sta_disconnected.rssi);
    break;
  case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
    Serial.printf("%s -> %s",
                  wifiAuthModeName(info.wifi_sta_authmode_change.old_mode),
                  wifiAuthModeName(info.wifi_sta_authmode_change.new_mode));
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    flags.wifiConnected = true;
    if (info.got_ip.ip_changed)
      Serial.printf("IP: " IPSTR "  Mask: " IPSTR "  GW: " IPSTR,
                    IP2STR(&info.got_ip.ip_info.ip),
                    IP2STR(&info.got_ip.ip_info.netmask),
                    IP2STR(&info.got_ip.ip_info.gw));
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
    flags.wifiConnected = true;
    Serial.printf("IPV6: " IPV6STR, IPV62STR(info.got_ip6.ip6_info.ip));
    break;
  case ARDUINO_EVENT_WIFI_STA_LOST_IP:
    flags.wifiConnected = false;
    Serial.printf("IP lost");
    break;
  }
  Serial.printf("\n");
  if (prev_wifi_connected != flags.wifiConnected) {
    flags.doConnectActions = true;
    logNetworkDetails();
    if (! flags.firstConnect) {
      flags.firstConnect = true;
      ntpSetup();
    }
  }
}



// Return a pretty name for WiFiEvent_t
const char *wifiEventName(WiFiEvent_t event)
{
  switch (event)
  {
  case ARDUINO_EVENT_WIFI_OFF:
    return "Off";
  case ARDUINO_EVENT_WIFI_READY:
    return "Ready";
  case ARDUINO_EVENT_WIFI_SCAN_DONE:
    return "Scan done";
  case ARDUINO_EVENT_WIFI_STA_START:
    return "Station start";
  case ARDUINO_EVENT_WIFI_STA_STOP:
    return "Station stop";
  case ARDUINO_EVENT_WIFI_STA_CONNECTED:
    return "Station connected";
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    return "Station disconnected";
  case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
    return "Station auth mode change";
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    return "Station got IP";
  case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
    return "Station got IP6";
  case ARDUINO_EVENT_WIFI_STA_LOST_IP:
    return "Station lost IP";
  case ARDUINO_EVENT_WIFI_AP_START:
    return "AP start";
  case ARDUINO_EVENT_WIFI_AP_STOP:
    return "AP stop";
  case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
    return "AP STA connected";
  case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
    return "AP STA disconnected";
  case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
    return "AP STA IP assigned";
  case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
    return "AP PROBE req received";
  case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
    return "AP got IP6";
  default:
    return "Unknown WiFi_Event_t";
  }
}

// Return a pretty name for a wifi_auth_mode_t
const char *wifiAuthModeName(wifi_auth_mode_t mode)
{
  switch (mode)
  {
  case WIFI_AUTH_OPEN:
    return "Open";
  case WIFI_AUTH_WEP:
    return "WEP";
  case WIFI_AUTH_WPA_PSK:
    return "WPA PSK";
  case WIFI_AUTH_WPA2_PSK:
    return "WPA2 PSK";
  case WIFI_AUTH_WPA_WPA2_PSK:
    return "WPA WPA2 PSK";
  case WIFI_AUTH_ENTERPRISE:
    return "WPA2 EAP Enterprise";
  case WIFI_AUTH_WPA3_PSK:
    return "WPA3 PSK";
  case WIFI_AUTH_WPA2_WPA3_PSK:
    return "WPA2 WPA3 PSK";
  case WIFI_AUTH_WAPI_PSK:
    return "WAPI PSK";
  case WIFI_AUTH_OWE:
    return "OWE";
  case WIFI_AUTH_WPA3_ENT_192:
    return "WPA3 ENT suite B 192-bit";
  default:
    return "Unknown WiFi Auth Mode";
  }
}

