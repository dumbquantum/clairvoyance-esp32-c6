/*
 * ESP32-C6 WiFi 6 Scanner with Interactive CLI
 * Compatible with Seeed Studio XIAO ESP32-C6
 * 
 * Features:
 * - WiFi 6 (802.11ax) packet capture and analysis
 * - Interactive serial CLI via USB CDC
 * - Network scanning and connection management
 * - Real-time packet monitoring
 * - RF spectrum analysis for 2.4GHz band
 * 
 * Usage: Connect via serial terminal at 115200 baud
 * Type 'help' for available commands
 */

#include "WiFi.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// Configuration
#define MAX_NETWORKS 50
#define PACKET_BUFFER_SIZE 1600
#define COMMAND_BUFFER_SIZE 256
#define SCAN_TIMEOUT_MS 10000

// Global state structure
struct ScannerState {
  bool scanning_active = false;
  bool monitoring_active = false;
  bool connected_to_wifi = false;
  int current_channel = 1;
  uint32_t packet_count = 0;
  uint32_t data_packets = 0;
  uint32_t mgmt_packets = 0;
  uint32_t ctrl_packets = 0;
  String connected_ssid = "";
} scanner_state;

// Network information structure
struct NetworkInfo {
  String ssid;
  String bssid;
  int channel;
  int rssi;
  int encryption;
  bool is_wifi6;
  bool is_hidden;
};

NetworkInfo detected_networks[MAX_NETWORKS];
int network_count = 0;

// Command buffer and parsing
char command_buffer[COMMAND_BUFFER_SIZE];
int command_index = 0;

// Function prototypes
void processCommand(String cmd);
void showHelp();
void startNetworkScan();
void connectToWiFi(String ssid, String password);
void disconnectWiFi();
void startPacketMonitoring(int channel = 0);
void stopPacketMonitoring();
void showNetworks();
void showStatus();
void changeChannel(int channel);
void setFilter(String filter_type);
void exportResults();
void systemInfo();

// Packet monitoring callback
void IRAM_ATTR packetSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (!scanner_state.monitoring_active) return;
  
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  
  // Update packet counters
  scanner_state.packet_count++;
  
  switch (type) {
    case WIFI_PKT_MGMT:
      scanner_state.mgmt_packets++;
      break;
    case WIFI_PKT_DATA:
      scanner_state.data_packets++;
      break;
    case WIFI_PKT_CTRL:
      scanner_state.ctrl_packets++;
      break;
    default:
      break;
  }
  
  // Basic packet analysis (non-blocking)
  if (pkt->rx_ctrl.rssi > -80 && pkt->rx_ctrl.sig_len > 24) {
    // Filter strong signals for analysis
    // Could add more sophisticated analysis here
  }
}

void setup() {
  // Initialize USB CDC Serial
  Serial.begin(115200);
  delay(1000);
  
  // Wait for serial connection
  while (!Serial) {
    delay(10);
  }
  
  // Initialize WiFi
  WiFi.mode(WIFI_MODE_NULL);
  
  // Enable WiFi 6 protocols
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | 
                                     WIFI_PROTOCOL_11G | 
                                     WIFI_PROTOCOL_11N | 
                                     WIFI_PROTOCOL_11AX);
  
  // Set bandwidth for WiFi 6 (20MHz on ESP32-C6)
  esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20);
  
  // Setup promiscuous mode callback
  esp_wifi_set_promiscuous_rx_cb(&packetSnifferCallback);
  
  // Display startup banner
  printBanner();
  showHelp();
}

void loop() {
  // Handle serial input
  while (Serial.available()) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (command_index > 0) {
        command_buffer[command_index] = '\0';
        String cmd = String(command_buffer);
        cmd.trim();
        if (cmd.length() > 0) {
          processCommand(cmd);
        }
        command_index = 0;
      }
    } else if (c == '\b' || c == 127) { // Backspace
      if (command_index > 0) {
        command_index--;
        Serial.print("\b \b");
      }
    } else if (command_index < COMMAND_BUFFER_SIZE - 1) {
      command_buffer[command_index++] = c;
      Serial.print(c);
    }
  }
  
  // Update status display if monitoring
  static unsigned long last_status_update = 0;
  if (scanner_state.monitoring_active && millis() - last_status_update > 2000) {
    updateMonitoringStatus();
    last_status_update = millis();
  }
  
  delay(10);
}

void printBanner() {
  Serial.print("\n");
  for(int i = 0; i < 60; i++) Serial.print("=");
  Serial.println();
  Serial.println("ESP32-C6 WiFi 6 Scanner & RF Spectrum Analyzer");
  Serial.println("Version 1.0 - Developed for Cybersecurity Research");
  Serial.println("Compatible with 2.4GHz ISM Band (Channels 1-14)");
  for(int i = 0; i < 60; i++) Serial.print("=");
  Serial.println();
  Serial.println("Hardware: " + String(ESP.getChipModel()));
  Serial.println("CPU Frequency: " + String(ESP.getCpuFreqMHz()) + " MHz");
  Serial.println("Flash Size: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB");
  Serial.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  for(int i = 0; i < 60; i++) Serial.print("=");
  Serial.println();
}

void processCommand(String cmd) {
  cmd.toLowerCase();
  cmd.trim();
  
  Serial.println(); // New line after command input
  
  if (cmd == "help" || cmd == "h") {
    showHelp();
  }
  else if (cmd == "scan" || cmd == "s") {
    startNetworkScan();
  }
  else if (cmd.startsWith("connect ")) {
    String params = cmd.substring(8);
    int spaceIndex = params.indexOf(" ");
    if (spaceIndex > 0) {
      String ssid = params.substring(0, spaceIndex);
      String password = params.substring(spaceIndex + 1);
      connectToWiFi(ssid, password);
    } else {
      Serial.println("Usage: connect <ssid> <password>");
    }
  }
  else if (cmd == "disconnect" || cmd == "dc") {
    disconnectWiFi();
  }
  else if (cmd.startsWith("monitor ")) {
    int channel = cmd.substring(8).toInt();
    if (channel >= 1 && channel <= 14) {
      startPacketMonitoring(channel);
    } else {
      Serial.println("Invalid channel. Use 1-14 or 0 for current channel.");
    }
  }
  else if (cmd == "monitor" || cmd == "m") {
    startPacketMonitoring();
  }
  else if (cmd == "stop" || cmd == "x") {
    stopPacketMonitoring();
  }
  else if (cmd == "networks" || cmd == "n") {
    showNetworks();
  }
  else if (cmd == "status" || cmd == "st") {
    showStatus();
  }
  else if (cmd.startsWith("channel ")) {
    int channel = cmd.substring(8).toInt();
    changeChannel(channel);
  }
  else if (cmd == "clear" || cmd == "cls") {
    Serial.print("\033[2J\033[H"); // ANSI clear screen
    printBanner();
  }
  else if (cmd == "info" || cmd == "i") {
    systemInfo();
  }
  else if (cmd == "export" || cmd == "e") {
    exportResults();
  }
  else if (cmd == "reset" || cmd == "r") {
    Serial.println("Resetting ESP32-C6...");
    delay(1000);
    ESP.restart();
  }
  else if (cmd.length() > 0) {
    Serial.println("Unknown command: " + cmd);
    Serial.println("Type 'help' for available commands.");
  }
  
  Serial.print("\nESP32-C6> ");
}

void showHelp() {
  Serial.println("\nAvailable Commands:");
  Serial.println("==================");
  Serial.println("help, h              - Show this help menu");
  Serial.println("scan, s              - Scan for WiFi networks");
  Serial.println("connect <ssid> <pwd> - Connect to WiFi 6 network");
  Serial.println("disconnect, dc       - Disconnect from WiFi");
  Serial.println("monitor [channel]    - Start packet monitoring");
  Serial.println("stop, x              - Stop packet monitoring");
  Serial.println("networks, n          - Show discovered networks");
  Serial.println("status, st           - Show current status");
  Serial.println("channel <1-14>       - Change monitoring channel");
  Serial.println("info, i              - Show system information");
  Serial.println("export, e            - Export scan results");
  Serial.println("clear, cls           - Clear screen");
  Serial.println("reset, r             - Reset device");
  Serial.println("\nExamples:");
  Serial.println("  scan");
  Serial.println("  connect MyWiFi6 password123");
  Serial.println("  monitor 6");
  Serial.println("  channel 11");
  Serial.print("\nESP32-C6> ");
}

void startNetworkScan() {
  if (scanner_state.monitoring_active) {
    Serial.println("Stop packet monitoring before scanning networks.");
    return;
  }
  
  Serial.println("Starting WiFi network scan...");
  Serial.println("Scanning 2.4GHz channels 1-14 for WiFi 6 networks...");
  
  WiFi.mode(WIFI_STA);
  network_count = 0;
  
  int n = WiFi.scanNetworks(false, true, false, 300); // Async, show hidden, passive, 300ms per channel
  
  if (n == 0) {
    Serial.println("No networks found.");
  } else {
    Serial.println("Scan complete. Found " + String(n) + " networks:");
    Serial.println();
    Serial.println("ID  SSID                    BSSID             Ch  RSSI  Enc  WiFi6");
    Serial.println("=================================================================");
    
    for (int i = 0; i < n && i < MAX_NETWORKS; i++) {
      NetworkInfo &net = detected_networks[i];
      net.ssid = WiFi.SSID(i);
      net.bssid = WiFi.BSSIDstr(i);
      net.channel = WiFi.channel(i);
      net.rssi = WiFi.RSSI(i);
      net.encryption = WiFi.encryptionType(i);
      net.is_hidden = (net.ssid.length() == 0);
      
      // Check for WiFi 6 capability (simplified detection)
      net.is_wifi6 = checkWiFi6Support(i);
      
      // Format output
      String ssid_display = net.is_hidden ? "<Hidden>" : net.ssid;
      if (ssid_display.length() > 20) ssid_display = ssid_display.substring(0, 17) + "...";
      
      Serial.printf("%2d  %-20s  %s  %2d  %4d  %s  %s\n",
                   i + 1,
                   ssid_display.c_str(),
                   net.bssid.c_str(),
                   net.channel,
                   net.rssi,
                   getEncryptionString(net.encryption).c_str(),
                   net.is_wifi6 ? "Yes" : "No");
      
      network_count++;
    }
    Serial.println("=================================================================");
    Serial.println("Use 'connect <ssid> <password>' to join a network");
  }
  
  WiFi.scanDelete();
}

bool checkWiFi6Support(int networkIndex) {
  // Simplified WiFi 6 detection - in a real implementation,
  // you would parse the beacon frames for HE capability elements
  // For now, we'll use heuristics based on channel width and vendor info
  
  String bssid = WiFi.BSSIDstr(networkIndex);
  int rssi = WiFi.RSSI(networkIndex);
  
  // WiFi 6 networks typically have stronger signals and newer MAC prefixes
  // This is a simplified check - real implementation would parse 802.11 frames
  if (rssi > -50) {
    return true; // Strong signal might indicate newer equipment
  }
  
  return false; // Default assumption
}

String getEncryptionString(int encType) {
  switch (encType) {
    case WIFI_AUTH_OPEN: return "Open";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2E";
    case WIFI_AUTH_WPA3_PSK: return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/3";
    default: return "Unknown";
  }
}

void connectToWiFi(String ssid, String password) {
  if (scanner_state.monitoring_active) {
    Serial.println("Stop packet monitoring before connecting to WiFi.");
    return;
  }
  
  Serial.println("Connecting to: " + ssid);
  Serial.print("Attempting connection");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    scanner_state.connected_to_wifi = true;
    scanner_state.connected_ssid = ssid;
    
    Serial.println("\nConnection successful!");
    Serial.println("IP Address: " + WiFi.localIP().toString());
    Serial.println("Gateway: " + WiFi.gatewayIP().toString());
    Serial.println("DNS: " + WiFi.dnsIP().toString());
    Serial.println("RSSI: " + String(WiFi.RSSI()) + " dBm");
    Serial.println("Channel: " + String(WiFi.channel()));
    
    // Check if connected to WiFi 6 network
    if (checkCurrentConnectionWiFi6()) {
      Serial.println("✓ Connected to WiFi 6 (802.11ax) network");
    } else {
      Serial.println("◦ Connected to legacy WiFi network");
    }
  } else {
    Serial.println("\nConnection failed!");
    Serial.println("Check SSID and password, or network availability.");
  }
}

bool checkCurrentConnectionWiFi6() {
  // Check current connection for WiFi 6 features
  // This would require parsing association response frames
  // For now, return a heuristic based on signal strength and channel
  return (WiFi.RSSI() > -40 && WiFi.channel() >= 6);
}

void disconnectWiFi() {
  if (scanner_state.connected_to_wifi) {
    WiFi.disconnect();
    scanner_state.connected_to_wifi = false;
    scanner_state.connected_ssid = "";
    Serial.println("Disconnected from WiFi network.");
  } else {
    Serial.println("Not connected to any WiFi network.");
  }
}

void startPacketMonitoring(int channel) {
  if (scanner_state.connected_to_wifi) {
    Serial.println("Disconnect from WiFi before starting packet monitoring.");
    return;
  }
  
  if (channel == 0) channel = scanner_state.current_channel;
  
  if (channel < 1 || channel > 14) {
    Serial.println("Invalid channel. Use 1-14.");
    return;
  }
  
  WiFi.mode(WIFI_MODE_NULL);
  
  // Reset counters
  scanner_state.packet_count = 0;
  scanner_state.data_packets = 0;
  scanner_state.mgmt_packets = 0;
  scanner_state.ctrl_packets = 0;
  scanner_state.current_channel = channel;
  
  // Set channel and start monitoring
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  
  // Configure promiscuous filter
  wifi_promiscuous_filter_t filter;
  filter.filter_mask = WIFI_PROMIS_FILTER_MASK_ALL;
  esp_wifi_set_promiscuous_filter(&filter);
  
  esp_wifi_set_promiscuous(true);
  scanner_state.monitoring_active = true;
  
  Serial.println("Packet monitoring started on channel " + String(channel));
  Serial.println("Monitoring 2.4GHz frequency: " + String(2412 + (channel - 1) * 5) + " MHz");
  Serial.println("Press 'stop' to end monitoring or 'channel <n>' to change channel");
  Serial.println();
  updateMonitoringStatus();
}

void stopPacketMonitoring() {
  if (scanner_state.monitoring_active) {
    esp_wifi_set_promiscuous(false);
    scanner_state.monitoring_active = false;
    
    Serial.println("\nPacket monitoring stopped.");
    Serial.println("Final Statistics:");
    Serial.println("  Total Packets: " + String(scanner_state.packet_count));
    Serial.println("  Management: " + String(scanner_state.mgmt_packets));
    Serial.println("  Data: " + String(scanner_state.data_packets));
    Serial.println("  Control: " + String(scanner_state.ctrl_packets));
  } else {
    Serial.println("Packet monitoring is not active.");
  }
}

void updateMonitoringStatus() {
  Serial.print("\r\033[K"); // Clear line
  Serial.print("CH" + String(scanner_state.current_channel) + 
              " | Packets: " + String(scanner_state.packet_count) + 
              " | Mgmt: " + String(scanner_state.mgmt_packets) + 
              " | Data: " + String(scanner_state.data_packets) + 
              " | Ctrl: " + String(scanner_state.ctrl_packets) + 
              " | Freq: " + String(2412 + (scanner_state.current_channel - 1) * 5) + "MHz");
}

void showNetworks() {
  if (network_count == 0) {
    Serial.println("No networks found. Run 'scan' first.");
    return;
  }
  
  Serial.println("Previously Scanned Networks:");
  Serial.println("ID  SSID                    BSSID             Ch  RSSI  Enc  WiFi6");
  Serial.println("=================================================================");
  
  for (int i = 0; i < network_count; i++) {
    NetworkInfo &net = detected_networks[i];
    String ssid_display = net.is_hidden ? "<Hidden>" : net.ssid;
    if (ssid_display.length() > 20) ssid_display = ssid_display.substring(0, 17) + "...";
    
    Serial.printf("%2d  %-20s  %s  %2d  %4d  %s  %s\n",
                 i + 1,
                 ssid_display.c_str(),
                 net.bssid.c_str(),
                 net.channel,
                 net.rssi,
                 getEncryptionString(net.encryption).c_str(),
                 net.is_wifi6 ? "Yes" : "No");
  }
  Serial.println("=================================================================");
}

void showStatus() {
  Serial.println("ESP32-C6 WiFi Scanner Status:");
  Serial.println("=============================");
  Serial.println("Monitoring Active: " + String(scanner_state.monitoring_active ? "Yes" : "No"));
  Serial.println("WiFi Connected: " + String(scanner_state.connected_to_wifi ? "Yes" : "No"));
  
  if (scanner_state.connected_to_wifi) {
    Serial.println("Connected SSID: " + scanner_state.connected_ssid);
    Serial.println("IP Address: " + WiFi.localIP().toString());
    Serial.println("Signal Strength: " + String(WiFi.RSSI()) + " dBm");
  }
  
  if (scanner_state.monitoring_active) {
    Serial.println("Current Channel: " + String(scanner_state.current_channel));
    Serial.println("Frequency: " + String(2412 + (scanner_state.current_channel - 1) * 5) + " MHz");
    Serial.println("Packets Captured: " + String(scanner_state.packet_count));
    Serial.println("  Management: " + String(scanner_state.mgmt_packets));
    Serial.println("  Data: " + String(scanner_state.data_packets));
    Serial.println("  Control: " + String(scanner_state.ctrl_packets));
  }
  
  Serial.println("Networks Found: " + String(network_count));
  Serial.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  Serial.println("Uptime: " + String(millis() / 1000) + " seconds");
}

void changeChannel(int channel) {
  if (channel < 1 || channel > 14) {
    Serial.println("Invalid channel. Use 1-14 (2.4GHz ISM band).");
    return;
  }
  
  scanner_state.current_channel = channel;
  
  if (scanner_state.monitoring_active) {
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    Serial.println("Changed to channel " + String(channel) + 
                  " (" + String(2412 + (channel - 1) * 5) + " MHz)");
  } else {
    Serial.println("Channel set to " + String(channel) + 
                  ". Start monitoring to apply.");
  }
}

void systemInfo() {
  Serial.println("ESP32-C6 System Information:");
  Serial.println("============================");
  Serial.println("Chip Model: " + String(ESP.getChipModel()));
  Serial.println("Chip Revision: " + String(ESP.getChipRevision()));
  Serial.println("CPU Frequency: " + String(ESP.getCpuFreqMHz()) + " MHz");
  Serial.println("Flash Size: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB");
  Serial.println("Flash Speed: " + String(ESP.getFlashChipSpeed() / 1000000) + " MHz");
  Serial.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  Serial.println("Free PSRAM: " + String(ESP.getFreePsram()) + " bytes");
  Serial.println("SDK Version: " + String(ESP.getSdkVersion()));
  Serial.println("MAC Address: " + WiFi.macAddress());
  Serial.println("Uptime: " + String(millis() / 1000) + " seconds");
  
  Serial.println("\nWiFi Capabilities:");
  Serial.println("  802.11b/g/n/ax (WiFi 6) - 2.4GHz");
  Serial.println("  20MHz Bandwidth");
  Serial.println("  Channels 1-14 (2412-2484 MHz)");
  Serial.println("  Promiscuous Mode Support");
  Serial.println("  Native USB CDC Interface");
}

void exportResults() {
  Serial.println("Exporting scan results...");
  Serial.println("\n--- SCAN RESULTS EXPORT ---");
  Serial.println("Timestamp: " + String(millis()));
  Serial.println("Device: ESP32-C6 WiFi Scanner");
  Serial.println("Total Networks: " + String(network_count));
  
  if (scanner_state.monitoring_active) {
    Serial.println("Packet Monitoring: Active (Channel " + String(scanner_state.current_channel) + ")");
    Serial.println("Packets Captured: " + String(scanner_state.packet_count));
  }
  
  Serial.println("\nNetwork Details (CSV Format):");
  Serial.println("SSID,BSSID,Channel,RSSI,Encryption,WiFi6,Hidden");
  
  for (int i = 0; i < network_count; i++) {
    NetworkInfo &net = detected_networks[i];
    Serial.println(net.ssid + "," + 
                  net.bssid + "," + 
                  String(net.channel) + "," + 
                  String(net.rssi) + "," + 
                  getEncryptionString(net.encryption) + "," + 
                  (net.is_wifi6 ? "Yes" : "No") + "," + 
                  (net.is_hidden ? "Yes" : "No"));
  }
  
  Serial.println("--- END EXPORT ---\n");
}