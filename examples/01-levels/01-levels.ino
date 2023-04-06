#include <esp-rix.h>

void setup() {
    Serial.begin(115200);

    int ok = rix_init_wifi("YourSSID", "SekritPass");
}

void loop() {
    rix_handle();

    rix_critical("CRITICAL message");
    rix_error("ERROR message");
    rix_warn("WARNING message");
    rix_info("INFORMATION message");
    rix_debug("DEBUG message");
    rix_trace("TRACE message");

    Serial.println("Loooping");

    rix_delay(2000);
}
