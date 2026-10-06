# Changelog

## [0.2.0](https://github.com/OpenSkyHawk/OpenSkyhawk/compare/firmware-v0.1.0...firmware-v0.2.0) (2026-10-06)


### Features

* **firmware:** DrumDisplay takes a bus or a mux ([#318](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/318)) ([c37f475](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/c37f475b9f1885bef96f5ca9d15f4e4da00075cf))


### Bug Fixes

* **firmware:** ADS1115 analog reads span the full 16-bit range ([#325](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/325)) ([#330](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/330)) ([0b325fd](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/0b325fd0d03175b21ca6339e5feaaf80140ec8fb))

## [0.1.0](https://github.com/OpenSkyHawk/OpenSkyhawk/compare/625e581f...firmware-v0.1.0) (2026-10-01)


### Features

* **firmware:** AnalogOutput family — abstract base, Dimmer and IntegerOutput ([#288](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/288)) ([#301](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/301)) ([b49fb91](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/b49fb9157424a8779acf31b370d682cba13b8b2c))
* **firmware:** AngleSensorInput — absolute angle sensor on a PinRef ([#294](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/294)) ([#303](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/303)) ([11c31c0](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/11c31c065be5a7dc4cb03a1dc76206b16153fc01))
* **firmware:** RotaryAcceleratedEncoder — DCS-BIOS-parity subclass of RotaryEncoder ([#287](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/287)) ([#296](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/296)) ([ce38c07](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/ce38c076a12549ebbc97d3b7dba66be6b7df1e88))
* **firmware:** STM32Board releases JTAG at boot, keeping SWD ([#299](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/299)) ([#300](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/300)) ([9b4673f](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/9b4673fe9cf3bce81fb120d8c3646edbc2be383d))
* **firmware:** SwitchWithCover2Pos — cover and switch from one pin ([#293](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/293)) ([#311](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/311)) ([9cad7d7](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/9cad7d7ce5bd0d1fa166fb1cd1abb8a8fb6ae331))


### Bug Fixes

* **firmware:** harden StepperMotor recal and fix DrumDisplay decode ([#137](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/137)) ([#313](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/313)) ([f5930b5](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/f5930b5cf8a3ec305681ac6eea4c4822727ca415))
