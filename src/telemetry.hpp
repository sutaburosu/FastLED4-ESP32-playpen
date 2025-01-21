#pragma once

#include "preferences.hpp"
#include <deque>
#include <map>
#include <time.h>
#if !defined(TELEMETRYSERIALONLY)
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#endif

// Return a String representing the current time
String timeString() {
    time_t now;
    struct tm timeInfo;
    char timeString[32];
    time(&now);
    localtime_r(&now, &timeInfo);
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S %Z", &timeInfo);
    return String(timeString);
}

// Store Teleplot-compatible, human-readable telemetry data, where each datum
// may have unique sending intervals, units, etc. Periodically coalesce only
// changed values into a report. Send them over Serial without blocking and/or
// UDP to Teleplot. Or just a terminal and/or netcat (e.g. `nc -l -u -p 47269`).
class Telemetry {
  public:
    Telemetry() {};
    ~Telemetry() {};

    // Holds the default values for a telemetry datum
    struct Datum {
        uint32_t sentMs = 0;    // when the last update was sent
        uint32_t minMs = 1000;  // send changes no more frequently than this
        uint32_t maxMs = 60000; // send at least this frequently
        String value;           // current value
        String lastValue;       // previous value that was sent
        String unit;            // unit of the value
        String teleplot = "np"; // teleplot configuration flags
        bool udpOnly = false;   // send only via UDP, never Serial
        bool sanitise = true;   // replace :|; with Unicode characters
    };

    void add(String name, const Datum &datum);
    void add(const String name, const String value);
    void begin();
    void send();
    void sysStats();

  private:
    // Holds a log entry
    struct Log {
        uint32_t timestamp; // when the log entry was created
        String message;     // the log entry
    };

    std::map<String, Datum> data;
    std::deque<String> serialQueue;
    std::deque<String> udpQueue;
    std::deque<Log> logQueue;

#if !defined(TELEMETRYSERIALONLY)
    const int udpMaxPayload = 1024; // max payload size for UDP telemetry
    int udpSocket = -1;             // a socket for sending UDP telemetry
    struct sockaddr_in udpSockAddr; // the destination address for UDP telemetry
    uint32_t udpSendMs = 0;         // when the last UDP telemetry was sent
#endif

    void sanitiseValue(String &value);
    bool coalesceChanges(uint32_t minMs = 200);
    void sendSerial();
    void sendUDP(uint32_t minMs = 200);
};

// Add a custom datum, or update the value if it already exists
void Telemetry::add(String name, const Datum &datum) {
    auto it = data.find(name);
    if (it == data.end())
        data.emplace(name, datum);
    else
        it->second.value = datum.value;
}

// Add a default datum, or update the value if it already exists
void Telemetry::add(const String name, const String value) {
    auto it = data.find(name);
    if (it == data.end())
        data.emplace(name, Datum{.value = value});
    else
        it->second.value = value;
}

// Teleplot doesn't like :|; in values
void Telemetry::sanitiseValue(String &value) {
    value.replace(":", "∶"); // U+2236 Ratio
    value.replace("|", "∣"); // U+2223 Divides
    value.replace(";", "⁏"); // U+204F Reversed Semicolon
}

// Every 200ms get changed values, format for Teleplot, and queue to be sent
bool Telemetry::coalesceChanges(uint32_t minMs) {
    static uint32_t lastCoalesce = 0;
    bool changed = false;
    uint32_t now = millis();
    if (now - lastCoalesce < minMs)
        return changed;

    String udpReports{""};
    String serialReports{""};
    for (auto &kv : data) {
        Datum &td = kv.second;
        uint32_t elapsed = now - td.sentMs;
        if (elapsed < td.maxMs) {
            if (elapsed < td.minMs)
                continue;
            if (td.value == td.lastValue)
                continue;
        }
        td.sentMs = now;
        td.lastValue = td.value;
        changed = true;

        String value = td.value;
        if (td.sanitise)
            sanitiseValue(value);
        String report = kv.first + ":" + value;

        if (td.unit.length())
            report += "§" + td.unit; // U+00A7 SECTION SIGN
        if (td.teleplot.length())
            report += "|" + td.teleplot;
        report += "\n";

        if (flags.serialTelemetry && !td.udpOnly)
            serialReports += ">" + report;
#if !defined(TELEMETRYSERIALONLY)
        if (flags.udpTelemetry)
            udpReports += report;
        if (udpReports.length() >= udpMaxPayload)
            udpQueue.push_back(udpReports), udpReports = "";

#endif
    }

    if (udpReports.length())
        udpQueue.push_back(udpReports);
    if (serialReports.length())
        serialQueue.push_back(serialReports);

    return changed;
}

// Send changed data over Serial and/or UDP
void Telemetry::send() {
    static uint32_t lastSend = 0;
    if (millis() - lastSend < 100)
        return;
    lastSend = millis();

    if (!flags.udpTelemetry && !flags.serialTelemetry) {
        serialQueue.clear();
        udpQueue.clear();
        return;
    }

    coalesceChanges();
    sendUDP();
    sendSerial();
}

void Telemetry::sendSerial() {
    static size_t position{0};
    static String item{""};

    // Drop oldest entries if the queue gets too long
    while (serialQueue.size() > 10)
        serialQueue.pop_front();

    if (serialQueue.empty() || !flags.serialTelemetry)
        if (item.isEmpty())
            return;

    // fill the outgoing buffer to capacity
    int charsToSend = Serial.availableForWrite();
    while (charsToSend > 0) {
        if (item.isEmpty()) {
            if (serialQueue.empty())
                break;
            item = serialQueue.front();
            serialQueue.pop_front();
            position = 0;
        }
        int chunkSize = item.length() - position;
        if (chunkSize > charsToSend)
            chunkSize = charsToSend;
        Serial.print(item.substring(position, position + chunkSize));
        position += chunkSize;
        charsToSend -= chunkSize;
        if (position >= item.length())
            item = "";
    }
}

void Telemetry::sendUDP(uint32_t minMs) {
#if defined(TELEMETRYSERIALONLY)
    udpQueue.clear();
    return;
#else
    if (0 == udpQueue.size())
        return;

    if (!flags.wifiConnected || !flags.udpTelemetry ||
        0xffffffff == udpSockAddr.sin_addr.s_addr) {
        // Logging starts before WiFi. Queue stuff until WiFi connects.
        while (udpQueue.size() > 50)
            udpQueue.pop_back();
        return;
    }

    // Cap the packets per second
    if (!udpSendMs)
        udpSendMs = millis();
    if (millis() - udpSendMs < minMs)
        return;

    // Create a UDP socket if it doesn't exist
    static int udpSocket = -1;
    if (udpSocket < 0) {
        udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (udpSocket < 0) {
            Serial.println("Failed to create UDP socket for telemetry");
            return;
        }
    }

    // Set up the destination address, if not already done
    if (!udpSockAddr.sin_addr.s_addr) {
        udpSockAddr.sin_family = AF_INET;
        udpSockAddr.sin_port =
            htons(preferences.getUInt("telemetry_port", 47269));
        udpSockAddr.sin_len = sizeof(udpSockAddr);
        String host = preferences.getString("telemetry_host", "");
        const char *host_cstr = host.c_str();
        int res = inet_aton(host_cstr, &udpSockAddr.sin_addr);
        if (1 != res) {
            // try to interpret as IPv6. Untested. I'm not sure if I'm doing
            // this properly.
            res = inet_pton(AF_INET6, host_cstr,
                            &(((sockaddr_in6 *)&udpSockAddr)->sin6_addr));
            if (1 == res) {
                udpSockAddr.sin_family = AF_INET6;
                udpSockAddr.sin_len = sizeof(udpSockAddr);
            } else {
                // try to resolve it as a hostname
                // again, I'm not sure if I'm doing this properly
                struct hostent *hostEnt = gethostbyname(host_cstr);
                if (NULL == hostEnt) {
                    // Serial.printf("Failed to resolve telemetry host '%s'\n",
                    //               host_cstr);
                    udpSockAddr.sin_addr.s_addr = 0xffffffff;
                    return;
                }
                memcpy(&udpSockAddr.sin_addr, hostEnt->h_addr_list[0],
                       hostEnt->h_length);
                if (hostEnt->h_addrtype == AF_INET6) {
                    udpSockAddr.sin_family = AF_INET6;
                    udpSockAddr.sin_len = sizeof(udpSockAddr);
                }
            }
        }
        // Serial.printf("Sending telemetry to '%s' %s:%u\n", host_cstr,
        //               inet_ntoa(udpSockAddr.sin_addr.s_addr),
        //               ntohs(udpSockAddr.sin_port));
    }

    // Concatenate up to 1KiB of telemetry in 1 packet
    String telemetryPacket = "";
    while (udpQueue.size() > 0) {
        String entry = udpQueue.front();
        if (telemetryPacket.length() + entry.length() > udpMaxPayload)
            if (telemetryPacket.length())
                break;
        udpQueue.pop_front();
        telemetryPacket += entry;
    }

    // Send the packet
    udpSendMs = millis();
    const char *pkt_cstr = telemetryPacket.c_str();
    size_t len = sendto(udpSocket, pkt_cstr, telemetryPacket.length(), 0,
                        (struct sockaddr *)&udpSockAddr, sizeof(udpSockAddr));
    if (len <= 0)
        return;

    // Send stats on the bandwidth used by Telemetry
    if (false) {
        static uint32_t udpBytes = 0;
        static uint32_t udpPackets = 0;
        static uint32_t udpReportms = 0;
        udpBytes += len;
        udpPackets++;
        if (!udpReportms)
            udpReportms = millis();

        uint32_t elapsed = millis() - udpReportms;
        if (elapsed >= 5000) {
            add("UDP bandwidth",
                {.value = String(udpBytes * 1000.f / elapsed), .unit = "B/s"});
            add("UDP packets", {.value = String(udpPackets * 1000.f / elapsed),
                                .unit = "pkts/s"});
            add("UDP avg size",
                {.value = String(udpBytes / udpPackets), .unit = "B"});
            udpReportms = millis();
            udpBytes = udpPackets = 0;
        }
    }
#endif
}

void Telemetry::begin() {
    add("Heap Free", {.maxMs = 10000, .unit = "KiB", .teleplot = ""});
    add("Heap Min", {.maxMs = 10000});
    add("Heap Max", {.maxMs = 10000});
    add("PS Free", {.maxMs = 10000, .unit = "KiB", .teleplot = ""});
    add("PS Min", {.maxMs = 10000});
    add("PS Max", {.maxMs = 10000});
    // add("Uptime", {.unit = "hours"});
    // add("Time", {.teleplot = "t,np"});
    // add("RSSI", {.unit = "dBm"});
    sysStats();
}

// Send some system statistics at most once per second each
void Telemetry::sysStats() {
    static uint32_t lastSysStats = 0;
    static uint32_t selector = 0;
    const int statsCount = 6;

    if (millis() - lastSysStats < (1000 / statsCount))
        return;
    lastSysStats = millis();

    switch (selector++) {
    case 0:
        add("Heap Free", String(ESP.getFreeHeap() / 1024.f));
        break;
    case 1:
        add("Heap Min", String(ESP.getMinFreeHeap() / 1024.f));
        break;
    case 2:
        add("Heap Max", String(ESP.getMaxAllocHeap() / 1024.f));
        break;
    case 3:
        add("PS Free", String(ESP.getFreePsram() / 1024.f));
        break;
    case 4:
        add("PS Min", String(ESP.getMinFreePsram() / 1024.f));
        break;
    case 5:
        add("PS Max", String(ESP.getMaxAllocPsram() / 1024.f));
        break;
    // case 6:
    //     add("Uptime", String(millis() / 3600000.f));
    //     break;
    // case 7:
    //     add("Time", timeString());
    //     break;
    // case 8:
    //     if (flags.wifiConnected)
    //         add("RSSI", String(WiFi.RSSI()));
    //     break;
    default:
        selector = 0;
    }
}
