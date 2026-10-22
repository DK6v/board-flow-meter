# Changelog

## 1.1.8 — 2026-10-23

### Changed
- ~10 KB more free heap (~25 KB instead of ~15 KB) and a 156 KB smaller
  firmware: DS18B20 addresses are formatted with `snprintf` instead of
  `std::stringstream`, which pulled in the C++ locale machinery.

### Added
- Free heap, the largest free block and heap fragmentation are logged at boot
  and every 15 minutes.

## 1.1.7 — 2026-10-23

### Fixed
- Since 1.1.3 the `device_id` tag was added again with every report
  (`reporter` 1.0.0 didn't clear headers): metrics got duplicate tags, then
  stopped fitting the buffer and weren't sent, and memory leaked until the
  board would run out of heap. Fixed by `reporter` 1.0.1.
- Fractional values were sent to InfluxDB with 6 significant digits
  (e.g. 16791853 Wh as `1.67919e+07`); `reporter` 1.0.1 sends them with
  6 decimals, trailing zeros trimmed.
- A `%` in the measurement name could crash the board when sending.

## 1.1.6 — 2026-10-23

### Changed
- A Pulsar request is repeated after 300 ms without an answer instead of 1 s;
  the meter answers in ~65 ms.

### Fixed
- The boot log showed the Pulsar heat total as 0 before the first reading;
  it shows the stored total now.

## 1.1.5 — 2026-10-23

### Fixed
- A Pulsar reading blocked the main loop for ~4 s, or ~7 s when the meter
  missed the first request after power-on: every attempt waited 3 s for a
  64-byte buffer although the response is 38 bytes. The response is now read
  frame by frame and done as soon as it's complete, attempts wait up to 1 s
  and are repeated for up to 6 s.
- Noise bytes before a Pulsar response no longer make it fail.

### Added
- The attempt number and the time after the first request are logged for a
  successful Pulsar reading, to tell whether the meter needs more time after
  power-on.

## 1.1.4 — 2026-10-22

### Added
- The firmware version and the reason of the last reset (watchdog, exception
  with its address) are sent to syslog at boot.

### Fixed
- Power correction is the number of Wh added per counted kWh, as labeled in
  the web form; it was applied per Wh, i.e. 1000 times too strong. The stored
  correction is now applied at boot, before it was used only after the form
  was saved.
- The heat energy total was overwritten with 0 when the first Pulsar reading
  after boot failed; it's now stored only after a successful reading.
- DS18B20 readings of -127 °C (sensor didn't answer) and 85 °C (power-on value)
  are no longer reported.

## 1.1.3 — 2026-10-22

### Changed
- All dependencies are pinned to exact versions: the platform
  (`espressif8266@4.2.1`), third-party libraries with their owners and the
  `piolibs` libraries by tag, so a clean build is reproducible.

## 1.1.2 — 2026-10-22

### Fixed
- Electricity used since the last PZEM reading (up to 30 minutes) was lost on
  every reboot: the PZEM energy counter was reset at boot. It's now reset only
  when a new total is entered in the web form; after a reboot the first reading
  still counts the energy accumulated in the PZEM.
- A failed PZEM energy reset made the next reading count the same energy
  again; the counted energy is now remembered and subtracted.

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
