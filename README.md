# Remote Information eXchange

<!--
Icons from: https://shields.io/

https://img.shields.io/badge/ESP-32-success
https://img.shields.io/badge/ESP-8266-success
https://img.shields.io/badge/Arduino-AVR-success?logo=arduino&logoWidth=18
-->
![ARDUINO-AVR](https://user-images.githubusercontent.com/3429760/220426704-0a102a4f-f661-4fa8-a3cc-b37af02a35d4.svg)
![ESP8266](https://user-images.githubusercontent.com/3429760/220426614-77c8aa30-325e-4e14-8bb9-94daf03f68fc.svg)
![ESP 32](https://user-images.githubusercontent.com/3429760/220428060-b08bb5ee-3a5c-4061-97ab-2c8977045aa2.svg)

**R**emote **I**nformation e**X**change adds remote logging and debugging
capabilities to your ESP based Arduino projects. This can be useful if
your project is in an inaccessible location, and serial isn't available.

# Installation

### Arduino IDE

Clone this repo to your Arduino libraries directory. On Linux this is
`~/Arduino/libraries/`

### VSCode PlatformIO

Add the repo link to platformio ini-file:

```ini
[env]
platform = espressif8266
board = nodemcuv2
framework = arduino
monitor_speed = 74800
lib_deps = https://github.com/<user>/<repo>.git
```

More on [PlatformIO](https://docs.platformio.org/en/stable/projectconf/sections/env/options/library/lib_deps.html) documentation.

# Usage

Include the RIX library

```C
#include <esp-rix.h>
```

Listen for RIX calls at the end of your `loop()` function

```C
void loop() {
	// Other loop code here

	// Rix supports 7 levels of debug messages
	rix_info("This is a INFORMATION message");
	rix_error("This is a ERROR message");

	// Rix also supports printf style messages
	rix_trace("MCU Uptime: %d minutes", millis() / 1000 / 60);

	rix_handle();
}
```

On a machine that shares the same WiFi as your ESP you can `telnet` to your
ESP's IP address to view the messages.

# Library options

#### On/Off color

Enable/disable color in output

```C
rix_color(false); // Disable color, default: true
```

#### Max log level

Set the initial output logging level

```C
rix_log_level(RixLevels::Information); // Default: RixLevels::Trace (5)
```

#### Set TCP port

Change the TCP port that RIX listens on

```C
rix_tcp_port(2300); // Default: 23
```

#### Special delay function

Using `delay()` in your scripts may cause RIX to be less responsive. A
`rix_delay()` method has been added as a drop-in replacement to keep your
project responsive.

```C
rix_delay(500); // Wait 500 ms
```

#### Special Wi-Fi connection function

RIX has a function to make connecting to your WiFi simple:

```C
int ok = rix_init_wifi("MySSID", "SekritPassword");
```

# Disabling RIX

When you're done debugging you can disable RIX entirely by adding:

```C
# define RIX_DISABLE
```

before the include line. This will make all the `rix_*` calls no-ops, and
disable logging. This means you can leave all your setup and logging calls in
your code and simply disable the library at compile time.

# Enable alternative log format

To enable an alternate log format (like `"HHH:MM:SS:FFF  LEVEL  Message"`),  add:

```C
# define RIX_FORMAT_EX
```

before the include line.

# Backwards compatibility

On non-ESP boards RIX is **automatically** disabled. This allows you to test
and debug on an ESP board, and then compile on an Arduino Nano with no changes
to your code.

# Inspired by

RIX was inspired by Joao Lopes' [RemoteDebug](https://github.com/JoaoLopesF/RemoteDebug)
which appears to be abandonned.
