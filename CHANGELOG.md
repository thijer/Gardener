# Changelog
## [2.0.1] - 5-1-2026
### Fixed
- Support for Properties v2.2

## [2.0.0] - 8-11-2025
### Added
- `ServoValve`, a class that uses Arduino's `Servo` implementation to control a servo attached to a ball valve.

### Changed
- `Gardener-waterer` now used a `ServoValve` to control water supply.

### Removed
- All pump functionality used by `Feeder`.

## [1.3.0] - 13-8-2025
### Added
- Feeder can now be given a start and abort command directly from an HTML form in the webgui.

## [1.2.0] - 12-8-2025
### Added
- Nozzle retraction and extrusion position properties.
- `Gardener-waterer` can now be configured with the desired nozzle retraction and extrusion positions.

## [1.1.1] - 11-8-2025
### Fixed
- Feeder now moves to correct position before feeding when given a new command when it is in WAITING.

## [1.1.0] - 10-8-2025
### Added
- `MoistureSensorArray`: A class that can measure the resistance of a moisture sensor and store the measurement in the assigned `IntProperty`.

### Changed
- switched window actuator and moisture analog pins because ESP32 wifi interferes with `analogRead` using ADC2.

## [1.0.2] - 6-8-2025
### Fixed
- WebGUI enable switch now behaves correctly after startup/reset.

## [1.0.1] - 5-8-2025
### Fixed
- "[Feeder] abort" message spamming by `Feeder`.
- Changed CRLF to LF to prevent double newline for messages from `Gardener-waterer` to `Gardener-main`.

### Changed
- `TempHumSensor` now reports if there is an error, and `Gardener-main` reverts to manual window control if that is the case.

## [1.0.0] - 3-8-2025
### Added
- Gardener-main: Main control system.
- Gardener-waterer: Subsystem for controlling the water feed carriage.

## [0.0.1] - 3-8-2025
### Added
- Changelog
- Readme
- .gitignore