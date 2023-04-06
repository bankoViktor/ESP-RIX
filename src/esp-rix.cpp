#include <string.h>   // For strcmp()
//#include <arduino.h>  // For delay()
#include <esp-rix.h>

#ifndef RIX_DISABLE

static const char *aszLevelNames[8] = {"NONE", "ALERT", "CRITICAL", "ERROR", "WARNING", "NOTICE", "INFORMATION", "DEBUG"};
static unsigned long nPrevTime 	= 0; 				// Millis when the last log entry was sent
static RixLevels eLevel     	= RixLevels::Debug; // Starting log level
static bool xColorEnable        = true; 			// Color enabled/disabled
static int nTcpPort = 23;
static WiFiClient client;

// Send the local echo
// static void local_echo(WiFiClient client, int num) {
// 	client.write(255);
// 	client.write(251);

// 	if (num) {
// 		//client.write(0);
// 		//??????
// 	} else {
// 		client.write(1);
// 	}
// }

// Send the Help
static void show_help(WiFiClient client) {
	client.println("c) Toggle color on or off");
	client.println("m) Show free memory");
	client.println("q) Quit and logout of ESP");
	client.println("r) Reboot MCU");
	client.println("u) Print MCU uptime");
	client.println("?) Show help screen");

	client.println("");

	client.println("0) Set logging level to NONE");
	client.println("1) Set logging level to ALERT");
	client.println("2) Set logging level to CRITCAL and above");
	client.println("3) Set logging level to ERROR and above");
	client.println("4) Set logging level to WARNING and above");
	client.println("5) Set logging level to NOTICE and above");
	client.println("6) Set logging level to INFORMATION and above");
	client.println("7) Set logging level to DEBUG and above");

	client.println("");
}

// Send the Telnet banner
static void send_banner(WiFiClient client) {
	String ipaddr  = rix_ip2string(WiFi.localIP());
	String mac     = WiFi.macAddress();
	String sdk_ver = ESP.getSdkVersion();
	int free_mem   = ESP.getFreeHeap();

	#if   defined(ESP32)
	const char* board_type = "ESP32";
	#elif defined(ESP8266)
	const char* board_type = "ESP8266";
	#else
	const char* board_type = "Unknown";
	#endif

	// Make the header WHITE
	if (xColorEnable) {
		client.print("\x1B[1m");       // Bold
		client.print("\x1B[38;5;15m"); // White
	}

	client.printf("Welcome to Remote Information eXchange - version %s\r\n", RIX_VERSION);
	client.printf("IP: %s / MAC: %s\r\n", ipaddr.c_str(), mac.c_str());
	client.printf("Free Mem: %d / ESP SDK: %s / %s\r\n", free_mem, sdk_ver.c_str(), board_type);
	//client.printf("MCU Uptime: %d minutes\r\n", millis() / 1000 / 60);
	client.println("=======================================================\r\n");

	// Reset the color
	if (xColorEnable) {
		client.print("\x1B[0m");
	}

	show_help(client);
}

void rix_color(bool newState) {
	xColorEnable = newState;
}

// Goes in loop() and listens for telnet connections
// Checks for input for commands
// Outputs log lines to connected clients
void rix_handle() {
	static bool first = 1;
	static WiFiServer TelnetServer(nTcpPort);

	if (first) {
		TelnetServer.begin();
		TelnetServer.setNoDelay(true);
		first = 0;
	}

	// Already connected telnet session
	if (client && client.connected()) {

		// Read chars into a string until we get to a \n
		char str[50] = "";
		int i        = 0;

		// Read all the available chars up to a \n
		while (client.available()) {
			char c = client.read();
			if (c == 10)           { break;    } // If it's a \n it's the end
			if (c > 127 || c < 32) { continue; } // Only add ASCII chars

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
			//client.printf("Got: '%s'\n", str);
		}

		// It's a "number string" if it's one character long and between 0 and 7
		int is_level_number = (i == 1 && (str[0] >= '0' && str[0] <= '7'));

		// If we got the quit command
		if (strcmp(str, "q") == 0) {
			client.stop();
		// If we got the help command
		} else if (strcmp(str, "?") == 0) {
			show_help(client);
			delay(2000);
		// If we got the color command
		} else if (strcmp(str, "c") == 0) {
			if (xColorEnable) {
				client.printf("Disabling color\r\n");
				rix_color(false);
			} else {
				client.printf("Enbabling color\r\n");
				rix_color(true);
			}
		// Free memory
		} else if (strcmp(str, "m") == 0) {
			int free_mem = ESP.getFreeHeap();
			client.printf("Free Memory: %d\r\n", free_mem);
		// Uptime
		} else if (strcmp(str, "u") == 0) {
			client.printf("Uptime: %0.1f minutes\r\n", millis() / float(60000));
		// Reboot/reset
		} else if (strcmp(str, "r") == 0) {
			client.printf("Rebooting...\r\n");
			client.stop(); // Disconnect the telnet session
			ESP.restart();
		// It's a number 1 - 7 we set the log level
		} else if (is_level_number) {
			// Convert char to int: https://stackoverflow.com/questions/5029840/convert-char-to-int-in-c-and-c
			RixLevels level = (RixLevels)(str[0] - '0');
			rix_log_level(level);
		}
	// New telnet session
	} else {
		// If there is a client connected we grab it
		client = TelnetServer.accept();

		// Reset the logging time in case of disconnect/reconnect
		nPrevTime = 0;

		if (client) {
			// Disable local echo
			// https://stackoverflow.com/questions/1098503/how-to-unsupress-local-echo
			//client.write(255);
			//client.write(251);
			//client.write(1);
			//local_echo(client, 0);

			send_banner(client);

			// Throw away the telnet handshaking data
			while (client.available()) {
				client.read();
			}
		}
	}
}

// This is underlying function that sends the log lines to telnet clients
void __debug_print(const char* function_name, RixLevels level, const char* format, ...) {
	// Check if there are any pening input commands (quit, change level) before we print
	rix_handle();

	// If there are no connected clients, don't print anything
	if (!client || !client.connected()) {
		return;
	}

	// If this log element is above our threshold we don't display it
	if (level > eLevel) {
		return;
	}

	const char* reset = "\x1B[0m";
	char color[25];

	// Alert - Dark Purple
	if (level == RixLevels::Alert) {
		//strncpy(color, "\x1B[48;5;90m\x1B[38;5;15m", 25);
		strncpy(color, "\x1B[38;5;90m", 12);
	// Critical - Red
	} else if (level == RixLevels::Critical) {
		strncpy(color, "\x1B[38;5;196m", 12);
	// Error - Orange
	} else if (level == RixLevels::Error) {
		strncpy(color, "\x1B[38;5;208m", 12);
	// Warning - Yellow
	} else if (level == RixLevels::Warning) {
		strncpy(color, "\x1B[38;5;226m", 12);
	// Notice - White
	} else if (level == RixLevels::Notice) {
		strncpy(color, "\x1B[38;5;15m", 12);
	// Informational - Light Green
	} else if (level == RixLevels::Information) {
		strncpy(color, "\x1B[38;5;156m", 12);
	// Debug - Light Blue
	} else if (level == RixLevels::Debug) {
		strncpy(color, "\x1B[38;5;123m", 12);
	}

	// Get the variadic params from this function
	va_list args;
	va_start(args, format);
	char buf[300];
	vsnprintf(buf, 300, format, args);
	va_end(args);

	// Calculate how long between the last call, and this one
	unsigned long diff = millis() - nPrevTime;
	if (!nPrevTime) {
		diff = 0;
	}

	// Milli-second string
	char ms_str[20] = "";
	snprintf(ms_str, 20, "(%04lums) ", diff);

	// Function name string
	char func_str[100] = "";
	snprintf(func_str, 100, "(F: %s) ", function_name);

	// Send the color and text data to the telnet client
	if (xColorEnable) {
		client.print(color);
	}

	client.print(ms_str);
	client.print(func_str);
	client.print(buf);

	if (xColorEnable) {
		client.print(reset);
	}

	client.print("\r\n");

	// Save this for the next time
	nPrevTime = millis();
}

// Set the TCP port to use
void rix_tcp_port(int port) {
	nTcpPort = port;
}

// Change the logging level for a connected client
void rix_log_level(RixLevels level) {
	// We only allow levels 0 - 7
	if ((int)level < 0 || (int)level > 7) {
		return;
	}

	char level_name[15] = "???";
	strcpy(level_name, aszLevelNames[(int)level]);

	if ((int)level > 1) {
		client.printf("Setting log level to %i (%s) and above\r\n", (int)level, level_name);
	} else {
		client.printf("Setting log level to %i (%s)\r\n", (int)level, level_name);
	}

	// Store the level so __debug_print() knows what to filter out
	eLevel = level;
}

// First pass at a wrapper function... this is not used anymore
void __xdebugN(const char* function_name, RixLevels level, const char* format, ...) {
	va_list args;
	va_start(args, format);
	char buf[300];
	vsnprintf(buf, 300, format, args);
	va_end(args);

	__debug_print("", RixLevels::Information, buf, RixLevels::Notice);
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
	String ret = String(ip[0]) + "." +  String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);

	return ret;
}

int rix_init_wifi(const char *ssid, const char *password) {
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);

	Serial.println("");
	Serial.print("Connecting to Wifi");

	const unsigned int start         = millis();
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
	Serial.printf("Connected to '%s'\r\n\r\n",ssid);

	String ipaddr = rix_ip2string(WiFi.localIP());
	Serial.printf("IP address   : %s\r\n", ipaddr.c_str());
	Serial.printf("MAC address  : %s \r\n", WiFi.macAddress().c_str());

	return 1;
}

#endif

// vim: tabstop=4 shiftwidth=4 noexpandtab autoindent softtabstop=4
