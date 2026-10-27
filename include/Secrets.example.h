/**
 * @file Secrets.example.h
 * @brief Mall för WiFi-uppgifterna.
 *
 * Kopiera filen till include/Secrets.h och fyll i nätverkets namn och
 * lösenord. Secrets.h checkas inte in (den står i .gitignore).
 *
 * Uno R4 WiFi klarar bara 2,4 GHz-nätverk.
 */
#pragma once

namespace secrets {

/// Nätverkets namn (SSID).
constexpr char kWifiSsid[] = "mitt-natverk";

/// Nätverkets lösenord.
constexpr char kWifiPassword[] = "mitt-losenord";

}  // namespace secrets
