#include <esp-rix.h>

void setup() {
    Serial.begin(115200);

    int ok = rix_init_wifi("YourSSID", "SekritPass");

    rix_color(false);                  // Disable color
    rix_log_level(RixLevels::Warning); // Starting log level: Critical + Error + Warning messages
}

void loop() {
    rix_handle();

    rix_critical("This is an ALERT");
    rix_error("This is an error %d", 37);
    rix_warn("This is a warning %s", "BUB!");
    rix_info("This is a %s", "INFORMATION");
    rix_debug("This is just debug %s", "Scott");
    rix_trace("This is just trace %s", "Foo");

    rix_delay(2000);
}
