#include <string.h> // For strcmp()
// #include <arduino.h>  // For delay()
#include <esp-rix.h>

#ifndef RIX_DISABLE

static const char   *g_aszLevelNames[] = {"CRITICAL", "ERROR", "WARNING", "INFORMATION", "DEBUG", "TRACE"};
static unsigned long g_nPrevTime = 0;             // Millis when the last log entry was sent
static RixLevels     g_eLevel = RixLevels::Trace; // Max out log level
static bool          g_xColorEnable = true;       // Color enabled/disabled
static int           g_nTcpPort = 23;
static WiFiClient    g_client;

// Send the Help
static void show_help() {
    g_client.println("c) Toggle color on or off");
    g_client.println("m) Show free memory");
    g_client.println("q) Quit and logout of ESP");
    g_client.println("r) Reboot MCU");
    g_client.println("u) Print MCU uptime");
    g_client.println("?) Show help screen");

    g_client.println("");

    g_client.println("1) Set logging level to CRITICAL");
    g_client.println("2) Set logging level to ERROR and above");
    g_client.println("3) Set logging level to WARNING and above");
    g_client.println("4) Set logging level to INFORMATION and above");
    g_client.println("5) Set logging level to DEBUG and above");
    g_client.println("6) Set logging level to TRACE and above");

    g_client.println("");
}

// Send the Telnet banner
static void send_banner() {
    String ipaddr = rix_ip2string(WiFi.localIP());
    String mac = WiFi.macAddress();
    String sdk_ver = ESP.getSdkVersion();
    int    free_mem = ESP.getFreeHeap();

#if defined(ESP32)
    const char *board_type = "ESP32";
#elif defined(ESP8266)
    const char *board_type = "ESP8266";
#else
    const char *board_type = "Unknown";
#endif

    // Make the header WHITE
    if (g_xColorEnable) {
        g_client.print("\x1B[1m");       // Bold
        g_client.print("\x1B[38;5;15m"); // White
    }

    g_client.printf("Welcome to Remote Information eXchange - version %s\r\n", RIX_VERSION);
    g_client.printf("IP: %s / MAC: %s\r\n", ipaddr.c_str(), mac.c_str());
    g_client.printf("Free Mem: %d / ESP SDK: %s / %s\r\n", free_mem, sdk_ver.c_str(), board_type);
    // g_client.printf("MCU Uptime: %d minutes\r\n", millis() / 1000 / 60);
    g_client.println("=======================================================\r\n");

    // Reset the color
    if (g_xColorEnable) {
        g_client.print("\x1B[0m");
    }

    show_help();
}

// Get color str for spicify log level
static const char *level_to_color(RixLevels eLevel) {
    switch (eLevel) {
    case RixLevels::Critical:
        return "\x1B[38;5;201m";
    case RixLevels::Error:
        return "\x1B[38;5;196m";
    case RixLevels::Warning:
        return "\x1B[38;5;226m";
    case RixLevels::Information:
        return "\x1B[38;5;27m";
    case RixLevels::Debug:
        return "\x1B[38;5;3m";
    case RixLevels::Trace:
        return "\x1B[38;5;241m";
    default:
        return "\x1B[38;5;0m";
    }
}

// This is underlying function that sends the log lines to telnet clients
void __debug_print2(RixLevels eLevel, const char *format, ...) {
    // Check if there are any pening input commands (quit, change level) before we print
    //rix_handle();

    // If there are no connected clients, don't print anything
    if (!g_client || !g_client.connected())
        return;

    // If this log element is above our threshold we don't display it
    if (eLevel > g_eLevel)
        return;

    static int ln = 1;
    g_client.printf("line %i\r\n", ln++);

    return;

    // Get the variadic params from this function
    va_list args;
    va_start(args, format);
    char msg[300];
    vsnprintf(msg, 300, format, args);
    va_end(args);

    // Elapsed time, format HHH:MM:SS.FFF
    unsigned long  time_ms = millis();
    unsigned short h = time_ms / 3600000;
    unsigned char  m = (time_ms - h * 3600000) / 60000;
    unsigned char  s = (time_ms - (h * 60 + m) * 60000) / 1000;
    unsigned short f = time_ms - ((h * 60 + m) * 60 + s) * 1000;
    char           pzTime[16];

// suppress warning
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    snprintf(pzTime, 16, "%03i:%02i:%02i.%03i", _min(h, 999), m, s, f);
#pragma GCC diagnostic pop

    // Send the color and text data to the telnet client
    const char *pszReset = "\x1B[0m";
    const char *pszColor = level_to_color(eLevel);
    if (g_xColorEnable)
        g_client.print(pszColor);

    //int len = strlen(pzTime);
    //Serial.printf("-- '%s' len %i\n", pzTime, len);
    //Serial.printf("-- '%s'\n", format);

    //g_client.print(pzTime);
    //g_client.print("  ");

    static const char *aszLevelNames[] = {
        "CRIT ",
        "ERROR",
        "WARN ",
        "INFO ",
        "DEBUG",
        "TRACE"};
    g_client.print(aszLevelNames[(int)eLevel]);
    g_client.print("  ");
    g_client.print(msg);

    if (g_xColorEnable)
        g_client.print(pszReset);

    g_client.print("\r\n");
}

// This is underlying function that sends the log lines to telnet clients
void __debug_print(const char *function_name, RixLevels eLevel, const char *format, ...) {
    // Check if there are any pening input commands (quit, change level) before we print
    rix_handle();

    // If there are no connected clients, don't print anything
    if (!g_client || !g_client.connected())
        return;

    // If this log element is above our threshold we don't display it
    if (eLevel > g_eLevel)
        return;

    // Get the variadic params from this function
    va_list args;
    va_start(args, format);
    char buf[300];
    vsnprintf(buf, 300, format, args);
    va_end(args);

    // Calculate how long between the last call, and this one
    unsigned long diff = millis() - g_nPrevTime;
    if (!g_nPrevTime) {
        diff = 0;
    }

    // Milli-second string
    char ms_str[20] = "";
    snprintf(ms_str, 20, "(%04lums) ", diff);

    // Function name string
    char func_str[100] = "";
    snprintf(func_str, 100, "(F: %s) ", function_name);

    // Send the color and text data to the telnet client
    const char *reset = "\x1B[0m";
    const char *pszColor = level_to_color(eLevel);
    if (g_xColorEnable) {
        g_client.print(pszColor);
    }

    g_client.print(ms_str);
    g_client.print(func_str);
    g_client.print(buf);

    if (g_xColorEnable) {
        g_client.print(reset);
    }

    g_client.print("\r\n");

    // Save this for the next time
    g_nPrevTime = millis();
}

// On/off color
void rix_color(bool newState) {
    g_xColorEnable = newState;
}

// Goes in loop() and listens for telnet connections
// Checks for input for commands
// Outputs log lines to connected clients
void rix_handle() {
    static bool       is_first = true;
    static WiFiServer TelnetServer(g_nTcpPort);

    if (is_first) {
        TelnetServer.begin();
        TelnetServer.setNoDelay(true);
        is_first = false;
    }

    // Already connected telnet session
    if (g_client && g_client.connected()) {

        // Read chars into a string until we get to a \n
        char str[50] = "";
        int  i = 0;

        // Read all the available chars up to a \n
        while (g_client.available()) {
            char c = g_client.read();
            if (c == 10) {
                break;
            } // If it's a \n it's the end
            if (c > 127 || c < 32) {
                continue;
            } // Only add ASCII chars

            // Build the string one char at a time
            str[i] = c;
            i++;

            // Don't go beyong end of string (wrap around and start over)
            if (i >= 49) {
                i = 0;
            }
        }

        // If the string has no chars bail out
        if (i == 0) {
            return;
            // If the string has SOME length
        } else if (i > 0) {
            str[i] = 0; // Terminate the string
            // g_client.printf("Got: '%s'\n", str);
        }

        // It's a "number string" if it's one character long and between '1' and '6'
        bool is_level_number = (i == 1 && (str[0] >= '1' && str[0] <= '6'));

        // If we got the quit command
        if (strcmp(str, "q") == 0) {
            g_client.stop();
            // If we got the help command
        } else if (strcmp(str, "?") == 0) {
            show_help();
            delay(2000);
            // If we got the color command
        } else if (strcmp(str, "c") == 0) {
            if (g_xColorEnable) {
                g_client.printf("Color disabled\r\n");
                rix_color(false);
            } else {
                g_client.printf("Color enbabled\r\n");
                rix_color(true);
            }
            // Free memory
        } else if (strcmp(str, "m") == 0) {
            int free_mem = ESP.getFreeHeap();
            g_client.printf("Free Memory: %d\r\n", free_mem);
            // Uptime
        } else if (strcmp(str, "u") == 0) {
            g_client.printf("Uptime: %0.1f minutes\r\n", millis() / float(60000));
            // Reboot/reset
        } else if (strcmp(str, "r") == 0) {
            g_client.printf("Rebooting...\r\n");
            g_client.stop(); // Disconnect the telnet session
            ESP.restart();
            // It's a number 1 - 7 we set the log level
        } else if (is_level_number) {
            // Convert char to int: https://stackoverflow.com/questions/5029840/convert-char-to-int-in-c-and-c
            RixLevels level = (RixLevels)(str[0] - '1');
            rix_log_level(level);
        }
        // New telnet session
    } else {
        // If there is a client connected we grab it
        g_client = TelnetServer.accept();

        // Reset the logging time in case of disconnect/reconnect
        g_nPrevTime = 0;

        if (g_client) {
            // Disable local echo
            // https://stackoverflow.com/questions/1098503/how-to-unsupress-local-echo
            // client.write(255);
            // client.write(251);
            // client.write(1);
            // local_echo(client, 0);

            send_banner();

            // Throw away the telnet handshaking data
            while (g_client.available()) {
                g_client.read();
            }
        }
    }
}

// Set the TCP port to use
void rix_tcp_port(int port) {
    g_nTcpPort = port;
}

// Change the logging level for a connected client
void rix_log_level(RixLevels eLevel) {
    if (eLevel < RixLevels::Critical || eLevel > RixLevels::Trace) {
        return;
    }

    const char *level_name = g_aszLevelNames[(int)eLevel];

    g_client.printf("Setting log level to %i (%s) and above\r\n", (int)eLevel + 1, level_name);

    g_eLevel = eLevel;
}

// If the MCU is in a delay() it cannot respond to HTTP OTA requests
// We do a "fake" looping delay and listen for incoming HTTP requests while waiting
void rix_delay(unsigned int ms) {
    // Borrowed from mshoe007 @ https://github.com/scottchiefbaker/ESP-WebOTA/issues/8
    decltype(millis()) last = millis();

    while ((millis() - last) < ms) {
        rix_handle();
        ::delay(5);
    }
}

String rix_ip2string(IPAddress ip) {
    String ret = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);

    return ret;
}

int rix_init_wifi(const char *ssid, const char *password) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    Serial.println("");
    Serial.print("Connecting to Wifi");

    const unsigned int start = millis();
    const unsigned int max_wait_secs = 45;

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");

        const unsigned int uptime_secs = (millis() - start) / 1000;
        if (uptime_secs >= max_wait_secs) {
            Serial.println("\r\nWiFi taking too long, not waiting anymore");
            return 0;
        }
    }

    Serial.println("");
    Serial.printf("Connected to '%s'\r\n\r\n", ssid);

    String ipaddr = rix_ip2string(WiFi.localIP());
    Serial.printf("IP address   : %s\r\n", ipaddr.c_str());
    Serial.printf("MAC address  : %s \r\n", WiFi.macAddress().c_str());

    return 1;
}

#endif
