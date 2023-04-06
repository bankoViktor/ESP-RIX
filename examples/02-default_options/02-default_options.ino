#include <esp-rix.h>

void setup() {
	Serial.begin(115200);

	int ok = rix_init_wifi("YourSSID", "SekritPass");

	rix_color(false);     // Disable color
	rix_log_level(RixLevels::Warning); // Starting log level is 4
}

void loop() {
	rix_handle();

	rix_1("This is an ALERT");
	rix_3("This is an error %d", 37);
	rix_4("This is a warning %s", "BUB!");
	rix_5("This is a %s", "NOTICE");
	rix_7("This is just debug %s", "Scott");

	rix_delay(2000);
}
