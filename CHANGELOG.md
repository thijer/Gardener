# Changelog
## [2.11.2] - 1-4-2026
### Changed
- All files now use PascalCase and are contained in dedicated subdirs.
- `VectorBaseStore` is moved to its own file.

## [2.11.1] - 27-3-2026
### Fixed
- All thingsboard devices use a "Test-" prefix in their names when `GARDENER_TEST` is defined.

## [2.11.0] - 24-3-2026
### Changed
- `WateringRuleEngine` now works with externally defined `WateringRule`s of which the parameters can be modified in Thingsboard.
- The `Debug` instance is now passed to the constructor of `RuleEngine` and derivations instead of to `begin()`. 

### Fixed
- Rules are now requested from Thingsboard at startup.

## [2.10.1] - 19-3-2026
### Fixed
- Actually check if rule is enabled before processing them.

## [2.10.0] - 13-3-2026
### Added
- `PropertyRuleEngine`, a `RuleEngine` implementation that stores the results of evaluated expressions in a `Property`.

### Fixed
- Compile error when some arguments to `RuleEngine::set_variables` are disabled, and the number of comma's are incorrect.
- Missing N_DEV_* defines when their corresponding components were not enabled.
  
### Changed
- `Rule`s can now also be compiled after construction.

## [2.9.1] - 6-3-2026
### Fixed
- Create a semblance of activity from `ThingRuleEngine` to Thingsboard, to prevent Thingsboard from marking `ThingRuleEngine` as inactive and stop sending rules to it.
- Fixed parameter swap during construction of a `WateringRule`.
- `DebugSocket` will now only check for connections once every 100ms instead of every time `loop` is called.
- Removed duplicate preprocessor condition check.
- JSON deserialization errors are now introduced first.

### Added
- Rules can now be printed to debug.

## [2.9.0] - 3-3-2026
### Added
- `ThingRuleEngine`, a `ThingDevice` inheriting wrapper that allows rules to be set from Thingsboard.

## [2.8.1] - 1-3-2026
### Changed
- Moisture sensors are now also present in the `variables` `PropertyStore` when thingsboard is enabled.
### Fixed
- Moisture sensors now use unique names for their internal `IntegerProperty`s when Thingsboard is enabled, instead of "moisture".

## [2.8.0] - 1-3-2026
### Added
- Support for ArduinoOTA.

## [2.7.0] - 1-3-2026
### Added
- `RuleEngine` a base class implementation of a rule engine based on TinyExpr++.
- `WateringRuleEngine`, an implementation of `RuleEngine` that will water a specific area of the greenhouse based on math expressions that can be added at runtime.

## [2.6.0] - 20-2-2026
### Added
- `DebugWebsocket`, allows for printing debug messages over a TCP connection.

## [2.5.1] - 20-2-2026
### Fixed
- Moved barrel parameters to `config.h`.
- Removed unused barrel parameters from `config.h`

## [2.5.0] - 20-2-2026
### Changed
- Moved tank volume calculations to a dedicated class called `DoubleFrustrumBarrel`.

### Added
- Added a `SingleFrustrumBarrel` model that more closely approximates the shape of the greenhouse barrel.

## [2.4.0] - 20-2-2026
### Added
- Thingsboard connection can now be enabled with a switch.
- `WiFiManager`, that maintains the WiFi connection.
- WiFi status can now be printed to `debug` upon request.

## Fixed
- `ThingGateway` is now prevented from trying to connect when there is no WiFi.
- Compilation error when moisture sensors are not `ENABLE`d but Thingsboard is.

## [2.3.0] - 10-2-2026
### Added
- `TankLevelSensor`, a class that can operate a standard HC_SR04 ultrasonic sensor to measure the stored volume in the rain water barrel.

## [2.2.4] - 10-2-2026
### Changed
- Added support for Properties v3

## [2.2.3] - 1-2-2026
### Removed
- unused commented-out code.
- unused config definitions.
### Fixed
- defaults for interval properties are now also in seconds.

## [2.2.2] - 1-2-2026
### Changed
- `Feeder` and `Window` FSM states are now sourced from a single preprocessor definition.
- debugging is now implemented the same across all objects.
- All `Property`s assigned to an object are now passed as reference to the object's constructor.
- Changed `uint` and `ulong` datatypes to standard int types whereever they occured.
- All `IntegerProperty`s that were compared to unsigned datatypes are now cast as `uint32_t`.
- All `IntegerProperty`s that describe a time value (intervals) are now interpreted as seconds instead of milliseconds, and are called `update_interval`.

## [2.2.1] - 31-1-2026
### Fixed
- `Feeder` will now wait for 2s after sending a command before sending a follow-up command, to give the `Gardener-waterer` time to respond. 

## [2.2.0] - 13-1-2026
### Added
- The `WateringLogic` base class, which provides a standard framework from which classes can be derived that each implement custom logic for watering a specific area of the greenhouse connected to the `Feeder`. 
- The `FixedQuantity` class, that specializes `WateringLogic` to water an area with a fixed quantity at a fixed interval.
- The `MoistureLevels` class, that specializes `WateringLogic` to water an area based on the soil moisture levels of one or more connected moisture sensors.

## [2.1.1] - 10-1-2026
### Fixed
- ESP32 starting a generic accesspoint during `WebInterface::begin`.

## [2.1.0] - 10-1-2026
### Added
- A queue to store multiple feed commands, so that multiple commands can be given at once without having to wait for the feeder to finish.

## [2.0.2] - 9-1-2026
### Added
- Option to manually set the tracked position variable.

### Changed
- Moved all state change code to the `set_state` function.
- Restructured the command parsing code.

### Fixed
- Feeder does not remain in `WAITING` state indefinitely.

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