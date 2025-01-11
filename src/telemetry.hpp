#pragma once

#include "main.hpp"
#include <map>

// Store Teleplot-compatible telemetry data, where each datum has it's own
// sending interval. Periodically coalesce only changed values, and send them
// in chunks over Serial and/or UDP to Teleplot, or just a terminal or netcat 
// (e.g. `nc -l -u -p 47269`).
struct Telemetry
{
  struct TelemetryDatum
  {
    uint32_t lastSentms;    // when the last update was sent
    uint32_t intervalms;    // send changes no more frequently than this
    uint32_t maxIntervalms; // send at least this frequently (even if unchanged)
    String value;           // the current value
    String lastValue;       // the previous value that was sent
    String unit;            // the unit of the value
    String teleplot;        // the teleplot configuration flags for this datum
    bool UDPonly;           // send only via UDP, never Serial
  };

  std::map<String, TelemetryDatum> telemetryData;
  std::deque<String> serialQueue;
  std::deque<String> UDPQueue;
  int udpSocket = -1;           // the socket for sending UDP telemetry
  struct sockaddr_in sock_addr; // the destination address for UDP telemetry

  // You don't need to add() a datum before update()ing it. If it needs custom
  // intervals or Teleplot flags AND there are multiple places in your code
  // where the same datum is updated, then it is better to add() it in setup()
  // with all the arguments, and simply update("name", "value") everywhere else.
  void add(String name,
           String unit = "",
           String teleplot = "t,np",
           String value = "",
           uint32_t intervalms = 1000,
           uint32_t maxIntervalms = 60000,
           bool UDPonly = true)
  {
    telemetryData[name] = TelemetryDatum {
        .lastSentms = 0,
        .intervalms = intervalms,
        .maxIntervalms = maxIntervalms,
        .value = value,
        .unit = unit,
        .teleplot = teleplot,
        .UDPonly = UDPonly,
    };
  }

  void update(String name, String value, String unit = "",
              String teleplot = "t,np", uint32_t intervalms = 1000,
              uint32_t maxIntervalms = 60000, bool UDPonly = true)
  {
    auto datum = telemetryData.find(name);
    if (datum != telemetryData.end())
      datum->second.value = value;
    else
      add(name, unit, teleplot, value, intervalms, maxIntervalms, UDPonly);
  }

  void send()
  {
    if (!flags.UDPTelemetry && !flags.serialTelemetry) {
      serialQueue.clear();
      UDPQueue.clear();
      return;
    }

    uint32_t now = millis();
    for (auto &kv : telemetryData)
    {
      TelemetryDatum &td = kv.second;
      uint32_t elapsed = now - td.lastSentms;
      if (elapsed < td.maxIntervalms) {
        if (elapsed < td.intervalms)
          continue;
        if (td.value == td.lastValue)
          continue;
      }
      td.lastSentms = now;
      td.lastValue = td.value;
      String report = kv.first + ":" + td.value;
      if (td.unit.length())
        report += "§" + td.unit;
      if (td.teleplot.length())
        report += "|" + td.teleplot;
      report += "\n";
      if (flags.serialTelemetry && !td.UDPonly)
        serialQueue.push_back(report);
      if (flags.UDPTelemetry)
        UDPQueue.push_back(report);
    }
    sendUDP();
    sendSerial();
  }

  void sendSerial()
  {
    static size_t position{0};
    static String item{""};

    if (! flags.serialTelemetry) {
      serialQueue.clear();
      return;
    }

    if (serialQueue.empty())
      return;

    // Drop oldest entries if the queue gets too long
    while (serialQueue.size() > 10)
      serialQueue.pop_front();

    unsigned int charsToSend = 16;
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
    if (!flags.wifiConnected || !flags.UDPTelemetry || 0xffffffff == sock_addr.sin_addr.s_addr)
    {
      UDPQueue.clear();
      return;
    }

    if (UDPQueue.size() == 0)
      return;

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
    if (!sock_addr.sin_addr.s_addr)
    {
      sock_addr.sin_family = AF_INET;
      sock_addr.sin_port = htons(preferences.getUInt("telemetry_port", 47269));
      sock_addr.sin_len = sizeof(sock_addr);
      String host = preferences.getString("telemetry_host", "192.168.1.194");
      const char *host_cstr = host.c_str();
      int res = inet_aton(host_cstr, &sock_addr.sin_addr);
      if (1 != res)
      {
        // try to interpret as IPv6. Untested. I'm not sure if I'm doing this properly.
        res = inet_pton(AF_INET6, host_cstr, &(((sockaddr_in6*)&sock_addr)->sin6_addr));
        if (res == 1)
        {
          sock_addr.sin_family = AF_INET6;
          sock_addr.sin_len = sizeof(sock_addr);
        }
        else
        {
          // try to resolve it as a hostname
          // again, I'm not sure if I'm doing this properly
          struct hostent *hostent = gethostbyname(host_cstr);
          if (hostent == NULL)
          {
            Serial.printf("Failed to resolve telemetry host '%s'\n", host_cstr);
            sock_addr.sin_addr.s_addr = 0xffffffff;
            return;
          }
          memcpy(&sock_addr.sin_addr, hostent->h_addr_list[0], hostent->h_length);
          if (hostent->h_addrtype == AF_INET6)
          {
            sock_addr.sin_family = AF_INET6;
            sock_addr.sin_len = sizeof(sock_addr);
          }
        }
      }
      Serial.printf("Sending telemetry to '%s' %s:%u\n", host_cstr, inet_ntoa(sock_addr.sin_addr.s_addr), sock_addr.sin_port);
    }

    // Concatenate up to 1KiB of telemetry in 1 packet
    String telemetryPacket = "";
    while (UDPQueue.size() > 0)
    {
      String entry = UDPQueue.front();
      if (telemetryPacket.length() + entry.length() > 1024 && telemetryPacket.length())
        break;
      telemetryPacket += entry;
      UDPQueue.pop_front();
    }

    // Send the packet
    const char *pkt_cstr = telemetryPacket.c_str();
    size_t len = sendto(udpSocket, pkt_cstr, telemetryPacket.length(),
                        0, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
    if (len <= 0)
      Serial.printf("sendto() failed: %d\n", len);
  }
};

Telemetry telemetry;
