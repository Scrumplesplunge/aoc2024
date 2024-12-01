// Rename this file to `wifi.hpp` and set the appropriate settings.
#ifndef AOC2024_WIFI_HPP_
#define AOC2024_WIFI_HPP_

// WiFi network credentials.
#define WIFI_SSID "Example SSID"
#define WIFI_PASS "Example Password"

// Current country, which is needed to comply with local radio transmission
// regulations. This can be left as CYW43_COUNTRY_WORLDWIDE, but the performance
// will suffer, so it is better to specify a country explicitly. See
// third_party/pico-sdk/lib/cyw43-driver/src/cyw43_country.h for a list of
// possible options, or use CYW43_COUNTRY('A', 'B', x) where "AB" is the two
// letter code for your country and x is the revision number.
#define WIFI_COUNTRY CYW43_COUNTRY('A', 'B', 0)

#endif  // AOC2024_WIFI_HPP_
