#include <esp-rix.h>

void setup() {
    Serial.begin(115200);

    int ok = rix_init_wifi("YourSSID", "SekritPass");
}

void loop() {
    rix_handle();

    float pi = 3.14159265;

    // RIX supports printf() style formats
    rix_trace("Uptime: %d ms", millis());
    rix_info("Hello %s", "world");
    rix_debug("Pi is: %1.5f", pi);

    Serial.println("Loooping");

    rix_delay(2000);
}
