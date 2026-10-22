# Changelog

## 1.1.1 — 2026-10-22

### Fixed
- The web UI showed meter totals rounded through `float` after boot
  (e.g. electricity 16791.208 kWh as 16791.207); saving the form then wrote
  the wrong value back. Totals are converted for display as `double`.

## 1.1.0 — 2026-10-22

### Changed
- Settings and meter totals are stored with `config` 2.0.0 in a flash journal
  in the FS area instead of EEPROM. Only changed values are appended, so a
  flash sector is no longer erased on every counter update, and the erases are
  spread over the whole FS area.
- `config` is pinned to a commit in `platformio.ini`.

### Fixed
- Meter totals are kept as integers instead of `float`, which lost units above
  2^24 (electricity total in Wh).
- Totals entered in the web form are parsed as `double` and rounded, so saving
  the form doesn't change the counters.

### Upgrade
- Values stored by 1.0.x are not migrated. Save them from `/param` before
  flashing and submit the form with the same values after the first boot.
  The heat energy total comes back from the Pulsar meter on its next reading
  (within 30 minutes).

## 1.0.2 — 2026-10-22

### Fixed
- Timers fired on every loop iteration before the `millis()` rollover
  (~49.7 days of uptime).
- The Pulsar heat meter returned stale data for ~49 days after the `millis()`
  rollover.
- The water counters could stop counting after the `millis()` rollover.
- After a power outage the board could hang in the WiFi setup portal when the
  router came up later; the portal now times out after 180 s and the board
  restarts.

## 1.0.1 — 2026-08-19

### Fixed
- Dallas sensor lookup always matched the first sensor.
- Heat meter temperature delta was logged as the outlet temperature.
- Type mismatches in the water counter state.

## 1.0.0 — 2026-08-18

- Water (cold/hot) pulse counters, PZEM-004T power meter, Pulsar heat meter
  and DS18B20 temperatures reported to InfluxDB.
- WiFiManager web portal with meter totals and settings.
- Wall-clock time from the CoAP config server, shown in the web UI.
- Firmware version shown in the web UI.
