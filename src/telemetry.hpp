#pragma once

#include <deque>
#include <map>
#include <time.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include "preferences.hpp"

// Return a String representing the current time
String timeString()
{
  time_t now;
  struct tm timeInfo;
  char timeString[32];
  time(&now);
  localtime_r(&now, &timeInfo);
  strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S %Z", &timeInfo);
  return String(timeString);
}

// Store Teleplot-compatible telemetry data, where each datum has it's own
// sending interval. Periodically coalesce only changed values, and send them
// in chunks over Serial and/or UDP to Teleplot, or just a terminal or netcat
// (e.g. `nc -l -u -p 47269`).

struct TelemetryDatum
{
  uint32_t sentMs = 0;    // when the last update was sent
  uint32_t minMs = 1000;  // send changes no more frequently than this
  uint32_t maxMs = 60000; // send at least this frequently (even if unchanged)
  String value;           // current value
  String lastValue;       // previous value that was sent
  String unit;            // unit of the value
  String teleplot = "";   // teleplot configuration flags
  bool udpOnly = false;   // send only via UDP, never Serial
  bool sanitise = true;   // replace : and | with Unicode characters
};

struct Telemetry
{
  std::map<String, TelemetryDatum> telemetryData;
  std::deque<String> serialQueue;
  std::deque<String> udpQueue;

  int udpSocket = -1;             // a socket for sending UDP telemetry
  struct sockaddr_in udpSockAddr; // the destination address for UDP telemetry
  uint32_t lastUDPsend = 0;       // when the last UDP telemetry was sent

  void log(String name, const TelemetryDatum &datum)
  {
    auto it = telemetryData.find(name);
    if (it == telemetryData.end())
    {
      telemetryData[name] = datum;
      return;
    }
    it->second.value = datum.value;
    it->second.unit = datum.unit;
    it->second.teleplot = datum.teleplot;
    it->second.minMs = datum.minMs;
    it->second.maxMs = datum.maxMs;
    it->second.udpOnly = datum.udpOnly;
    it->second.sanitise = datum.sanitise;
  }

  void log(const String name, const String value)
  {
    auto it = telemetryData.find(name);
    if (it == telemetryData.end())
      telemetryData[name].value = value;
    else
      telemetryData[name] = TelemetryDatum{.value = value};
  }

  void update(String name,
              String value,
              String unit = "",
              String teleplot = "np",
              uint32_t minMs = 1000,
              uint32_t maxMs = 60000,
              bool udpOnly = false,
              bool sanitise = true)
  {
    auto datum = telemetryData.find(name);
    if (datum != telemetryData.end())
      datum->second.value = value;
    else
      telemetryData[name] = TelemetryDatum{.minMs = minMs,
                                           .maxMs = maxMs,
                                           .value = value,
                                           .unit = unit,
                                           .teleplot = teleplot,
                                           .udpOnly = udpOnly,
                                           .sanitise = sanitise};
  }

  void sanitiseValue(String &value)
  {
    // Teleplot doesn't like : or | in values
    value.replace(":", "﹕"); // U+FE55 SMALL COLON
    value.replace("|", "｜"); // U+FF1C FULLWIDTH VERTICAL LINE
  }

  void send()
  {
    if (!flags.udpTelemetry && !flags.serialTelemetry)
    {
      serialQueue.clear();
      udpQueue.clear();
      return;
    }

    uint32_t now = millis();
    for (auto &kv : telemetryData)
    {
      TelemetryDatum &td = kv.second;
      uint32_t elapsed = now - td.sentMs;
      if (elapsed < td.maxMs)
      {
        if (elapsed < td.minMs)
          continue;
        if (td.value == td.lastValue)
          continue;
      }
      td.sentMs = now;
      td.lastValue = td.value;

      String value = td.value;
      if (td.sanitise)
        sanitiseValue(value);
      String report = kv.first + ":" + value;

      if (td.unit.length())
        report += "§" + td.unit;
      if (td.teleplot.length())
        report += "|" + td.teleplot;
      report += "\n";

      if (flags.serialTelemetry && !td.udpOnly)
        serialQueue.push_back(report);
      if (flags.udpTelemetry)
        udpQueue.push_back(report);
    }
    sendUDP();
    sendSerial();
  }

  void sendSerial()
  {
    static size_t position{0};
    static String item{""};

    // Drop oldest entries if the queue gets too long
    while (serialQueue.size() > 10)
      serialQueue.pop_front();

    if (serialQueue.empty() || !flags.serialTelemetry)
      if (item.isEmpty())
        return;

    unsigned int charsToSend = 8;
    while (charsToSend > 0)
    {
      if (item.isEmpty())
      {
        if (serialQueue.empty())
          break;
        // Prepend each entry with '>' for Teleplot over Serial
        // Lines without '>' are logged in a different pane by Teleplot
        item = ">" + serialQueue.front();
        serialQueue.pop_front();
        position = 0;
      }
      int chunkSize = min(charsToSend, item.length() - position);
      Serial.print(item.substring(position, position + chunkSize));
      position += chunkSize;
      charsToSend -= chunkSize;
      if (position >= item.length())
        item = "";
    }
  }

  void sendUDP()
  {
    if (!flags.wifiConnected || !flags.udpTelemetry || 0xffffffff == udpSockAddr.sin_addr.s_addr)
    {
      udpQueue.clear();
      return;
    }

    if (0 == udpQueue.size())
      return;

    // Cap at 5 packets per second, to aid coalescing and reduce header overhead
    if (!lastUDPsend)
      lastUDPsend = millis();
    if (millis() - lastUDPsend < 200)
      return;
    lastUDPsend = millis();

    // Create a UDP socket if it doesn't exist
    static int udpSocket = -1;
    if (udpSocket < 0)
    {
      udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      if (udpSocket < 0)
      {
        Serial.println("Failed to create UDP socket for telemetry");
        return;
      }
    }

    // Set up the destination address, if not already done
    if (!udpSockAddr.sin_addr.s_addr)
    {
      udpSockAddr.sin_family = AF_INET;
      udpSockAddr.sin_port = htons(preferences.getUInt("telemetry_port", 47269));
      udpSockAddr.sin_len = sizeof(udpSockAddr);
      String host = preferences.getString("telemetry_host", "192.168.1.194");
      const char *host_cstr = host.c_str();
      int res = inet_aton(host_cstr, &udpSockAddr.sin_addr);
      if (1 != res)
      {
        // try to interpret as IPv6. Untested. I'm not sure if I'm doing this properly.
        res = inet_pton(AF_INET6, host_cstr, &(((sockaddr_in6 *)&udpSockAddr)->sin6_addr));
        if (1 == res)
        {
          udpSockAddr.sin_family = AF_INET6;
          udpSockAddr.sin_len = sizeof(udpSockAddr);
        }
        else
        {
          // try to resolve it as a hostname
          // again, I'm not sure if I'm doing this properly
          struct hostent *hostEnt = gethostbyname(host_cstr);
          if (NULL == hostEnt)
          {
            Serial.printf("Failed to resolve telemetry host '%s'\n", host_cstr);
            udpSockAddr.sin_addr.s_addr = 0xffffffff;
            return;
          }
          memcpy(&udpSockAddr.sin_addr, hostEnt->h_addr_list[0], hostEnt->h_length);
          if (hostEnt->h_addrtype == AF_INET6)
          {
            udpSockAddr.sin_family = AF_INET6;
            udpSockAddr.sin_len = sizeof(udpSockAddr);
          }
        }
      }
      Serial.printf("Sending telemetry to '%s' %s:%u\n", host_cstr,
                    inet_ntoa(udpSockAddr.sin_addr.s_addr),
                    ntohs(udpSockAddr.sin_port));
    }

    // Concatenate up to 1KiB of telemetry in 1 packet
    String telemetryPacket = "";
    while (udpQueue.size() > 0)
    {
      String entry = udpQueue.front();
      if (telemetryPacket.length() + entry.length() > 1024 && telemetryPacket.length())
        break;
      udpQueue.pop_front();
      telemetryPacket += entry;
    }

    // Send the packet
    const char *pkt_cstr = telemetryPacket.c_str();
    size_t len = sendto(udpSocket, pkt_cstr, telemetryPacket.length(),
                        0, (struct sockaddr *)&udpSockAddr, sizeof(udpSockAddr));
    if (len <= 0)
      Serial.printf("sendto() failed: %d\n", len);
    else
    {
      static uint32_t udpBytes = 0;
      static uint32_t udpPackets = 0;
      static uint32_t udpReportms = 0;
      udpBytes += len;
      udpPackets++;
      if (!udpReportms)
        udpReportms = millis();

      if (millis() - udpReportms > 3000)
      {
        uint32_t elapsed = millis() - udpReportms;
        udpReportms = millis();
        update("UDP bandwidth", String(udpBytes * 1000.f / elapsed), "B/s", "np");
        update("UDP packets", String(udpPackets * 1000.f / elapsed), "pkts/s", "np");
        udpBytes = udpPackets = 0;
      }
    }
  }
} telemetry;

// Send some system statistics at most once per second each
void sysStats()
{
  static uint32_t lastSysStats = 0;
  static uint32_t selector = 0;
  const int statsCount = 9;
  if (millis() - lastSysStats < (1000 / statsCount))
    return;
  lastSysStats = millis();

  switch (selector)
  {
  case 0:
    if (flags.wifiConnected)
      telemetry.log("RSSI", {.value = String(WiFi.RSSI()),
                             .unit = "dBm"});
    break;
  case 1:
    telemetry.log("Heap Free", {.maxMs = 5000,
                                .value = String(ESP.getFreeHeap() / 1024.f),
                                .unit = "KiB"});
    break;
  case 2:
    telemetry.log("Heap Min", {.maxMs = 5000,
                               .value = String(ESP.getMinFreeHeap() / 1024.f),
                               .teleplot = "np"});
    break;
  case 3:
    telemetry.log("Heap Max", {.maxMs = 5000,
                               .value = String(ESP.getMaxAllocHeap() / 1024.f),
                               .teleplot = "np"});
    break;
  case 4:
    telemetry.log("PS Free", {.maxMs = 5000,
                              .value = String(ESP.getFreePsram() / 1024.f),
                              .unit = "KiB"});
    break;
  case 5:
    telemetry.log("PS Min", {.maxMs = 5000,
                             .value = String(ESP.getMinFreePsram() / 1024.f),
                             .teleplot = "np"});
    break;
  case 6:
    telemetry.log("PS Max", {.maxMs = 5000,
                             .value = String(ESP.getMaxAllocPsram() / 1024.f),
                             .teleplot = "np"});
    break;
  case 7:
    telemetry.log("Uptime", {.value = String(millis() / 3600000.f),
                             .unit = "hours",
                             .teleplot = "np"});
    break;
  case 8:
    telemetry.log("Time", {.value = timeString(),
                           .teleplot = "t,np"});
    break;
  }
  selector = (selector + 1) % statsCount;
}
