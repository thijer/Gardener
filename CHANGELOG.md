# Changelog

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