# Changelog

## [0.3.0](https://github.com/OpenSkyHawk/OpenSkyhawk/compare/firmware-v0.2.0...firmware-v0.3.0) (2026-10-27)


### ⚠ BREAKING CHANGES

* **firmware:** sim output yields to every input on CAN — CTRL_BCAST moves from 0x010 to 0x7E0 ([#389](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/389))
* **bootloader:** BootloaderInfo's first reserved byte is now `transports`, and STM32Board::enterBootloader() takes the transport the request came on.
* **firmware:** AnalogInput and AngleSensorInput take a config struct; hysteresis defaults by route ([#295](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/295))
* **firmware:** StepperConfig carries the X27 defaults; makeX27Config takes 5 arguments ([#295](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/295))
* **firmware:** move the node-roster request to export address 0xFF00 ([#295](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/295))

### Features

* **bootloader:** update the Gateway's PanelBridge over USART2 and freeze the bootloader ([#367](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/367)) ([0ca6987](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/0ca6987fff08282b9dc0d0dd252233db359a1b28))
* **firmware:** AnalogInput and AngleSensorInput take a config struct; hysteresis defaults by route ([#295](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/295)) ([5771d0d](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/5771d0d2c1a3df68df552612c39ccf3f0b1abf13))
* **firmware:** AnalogMultiPos::NO_DEADBAND ([#295](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/295)) ([5771d0d](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/5771d0d2c1a3df68df552612c39ccf3f0b1abf13))
* **firmware:** bootloader self-protection, NODE_ID rule, crash-loop counter, catch at boot ([#364](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/364)) ([#377](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/377)) ([8f66939](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/8f669394edbcdedb470c9ea31bc50576230b4b9d))
* **firmware:** CAN bootloader core and the 4 KB app layout ([#363](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/363)) ([#376](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/376)) ([bfbffa1](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/bfbffa1b54b84c5acd6dcf794bb41e9d4e5a1a64))
* **firmware:** image identity block on every production STM32 image ([#362](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/362)) ([#375](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/375)) ([2be6db6](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/2be6db6207fde6cb263c01912b46ca948b42f231))
* **firmware:** move the node-roster request to export address 0xFF00 ([#295](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/295)) ([5771d0d](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/5771d0d2c1a3df68df552612c39ccf3f0b1abf13))
* **firmware:** nodes announce their firmware version in READY_n, _NODE_STATUS v4 ([#295](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/295)) ([5771d0d](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/5771d0d2c1a3df68df552612c39ccf3f0b1abf13))
* **firmware:** nodes enter the bootloader on request; the bridge relays update traffic and reports it ([#365](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/365)) ([#378](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/378)) ([d7ac1ac](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/d7ac1aced64e8a708d45d5f57aada5a5116527ae))
* **firmware:** PanelBridge forwards the whole DCS-BIOS export stream ([#295](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/295)) ([5771d0d](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/5771d0d2c1a3df68df552612c39ccf3f0b1abf13))
* **firmware:** PanelBridge node-status reporting on by default ([#295](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/295)) ([5771d0d](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/5771d0d2c1a3df68df552612c39ccf3f0b1abf13))
* **firmware:** reserve room in DIAG for future link-health counters ([#295](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/295)) ([5771d0d](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/5771d0d2c1a3df68df552612c39ccf3f0b1abf13))
* **firmware:** sim output yields to every input on CAN — CTRL_BCAST moves from 0x010 to 0x7E0 ([#389](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/389)) ([c810e13](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/c810e139b1f162451c6687ff17b9bf700f652d45))
* **firmware:** StepperConfig carries the X27 defaults; makeX27Config takes 5 arguments ([#295](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/295)) ([5771d0d](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/5771d0d2c1a3df68df552612c39ccf3f0b1abf13))
* **firmware:** streaming bootloader transfer and the Gateway uploader ([#366](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/366)) ([#379](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/379)) ([c365e09](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/c365e096b1b72fc2c1ac21ef4f5af771d85f0d0d))
* **panelbridge:** forget a node that's gone on purpose ([#384](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/384)) ([#386](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/386)) ([50a1c0d](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/50a1c0d6661c0b0666f0c3c240bc3b90dd425bb7))


### Bug Fixes

* **bootloader:** guard the bridge's DMA write position at the wrap ([#367](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/367)) ([0ca6987](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/0ca6987fff08282b9dc0d0dd252233db359a1b28))
* **firmware:** bridge RX ring to 128 frames ([#387](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/387)) ([e82ac6e](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/e82ac6eb7c6ada62135e39a065ddf652eaf35eaf))
* **firmware:** calibration blocks every firmware upload, not only UPL_BEGIN ([#402](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/402)) ([e8eec29](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/e8eec2966ffc96424a34930025a32cd95b8e3cf4))
* **firmware:** nodes re-announce their version on every SYNC_REQ ([#383](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/383)) ([#385](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/385)) ([692a5a5](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/692a5a55ce1450c7bb1f96abe1f828bad5d26751))
* **firmware:** resend CAN frames that lose arbitration; RX and TX run from their interrupts ([#389](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/389)) ([c810e13](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/c810e139b1f162451c6687ff17b9bf700f652d45))
* **firmware:** sync nodes one at a time so SYNC_REQ answers can't overflow the bridge ([#387](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/387)) ([e82ac6e](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/e82ac6eb7c6ada62135e39a065ddf652eaf35eaf))
* **firmware:** templates and panel sketches ship with DiagSerial off ([#389](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/389)) ([c810e13](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/c810e139b1f162451c6687ff17b9bf700f652d45))
* **panelbridge:** forward the whole DCS-BIOS export stream to CAN ([#389](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/389)) ([c810e13](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/c810e139b1f162451c6687ff17b9bf700f652d45))
* **panelbridge:** never block loop() on a roster reply — write it a row at a time ([#388](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/388)) ([c810e13](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/c810e139b1f162451c6687ff17b9bf700f652d45))
* **panelbridge:** the boot roster survives every bridge reset ([#367](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/367)) ([0ca6987](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/0ca6987fff08282b9dc0d0dd252233db359a1b28))
* **simgateway:** service USB from the main loop only — no deadlock in TinyUSB's task IRQ ([#389](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/389)) ([c810e13](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/c810e139b1f162451c6687ff17b9bf700f652d45))


### Performance Improvements

* **firmware:** the bridge pairs CTRL_BCAST packets instead of sending them half empty ([#389](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/389)) ([c810e13](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/c810e139b1f162451c6687ff17b9bf700f652d45))

## [0.2.0](https://github.com/OpenSkyHawk/OpenSkyhawk/compare/firmware-v0.1.0...firmware-v0.2.0) (2026-10-25)


### Features

* **firmware:** DrumDisplay takes a bus or a mux ([#318](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/318)) ([c37f475](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/c37f475b9f1885bef96f5ca9d15f4e4da00075cf))
* **firmware:** nodes announce their board kind in HEALTH_n and _NODE_STATUS v3 ([#344](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/344)) ([#345](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/345)) ([d29433a](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/d29433ab149fcff1695e3d5eb07e2fab4211a603))
* **firmware:** PDU monitoring node and TELEM_n telemetry frame ([#216](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/216)) ([#343](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/343)) ([22b6a87](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/22b6a87e73068ca2bfd964d067af1247771d5854))


### Bug Fixes

* **firmware:** ADS1115 analog reads span the full 16-bit range ([#325](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/325)) ([#330](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/330)) ([0b325fd](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/0b325fd0d03175b21ca6339e5feaaf80140ec8fb))
* **firmware:** PanelBridge template builds for the STM32F103C8 on the Gateway ([#360](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/360)) ([b8ee30a](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/b8ee30a48d1b615d1cc91475bdfe22a990e5af63))
* **firmware:** SimGateway never blocks on the host and recovers its own USB link ([#355](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/355)) ([#358](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/358)) ([9e17fa2](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/9e17fa28f35e554b2e852cc1d79342e2875cf17d))

## [0.1.0](https://github.com/OpenSkyHawk/OpenSkyhawk/compare/625e581f...firmware-v0.1.0) (2026-10-01)


### Features

* **firmware:** AnalogOutput family — abstract base, Dimmer and IntegerOutput ([#288](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/288)) ([#301](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/301)) ([b49fb91](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/b49fb9157424a8779acf31b370d682cba13b8b2c))
* **firmware:** AngleSensorInput — absolute angle sensor on a PinRef ([#294](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/294)) ([#303](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/303)) ([11c31c0](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/11c31c065be5a7dc4cb03a1dc76206b16153fc01))
* **firmware:** RotaryAcceleratedEncoder — DCS-BIOS-parity subclass of RotaryEncoder ([#287](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/287)) ([#296](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/296)) ([ce38c07](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/ce38c076a12549ebbc97d3b7dba66be6b7df1e88))
* **firmware:** STM32Board releases JTAG at boot, keeping SWD ([#299](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/299)) ([#300](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/300)) ([9b4673f](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/9b4673fe9cf3bce81fb120d8c3646edbc2be383d))
* **firmware:** SwitchWithCover2Pos — cover and switch from one pin ([#293](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/293)) ([#311](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/311)) ([9cad7d7](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/9cad7d7ce5bd0d1fa166fb1cd1abb8a8fb6ae331))


### Bug Fixes

* **firmware:** harden StepperMotor recal and fix DrumDisplay decode ([#137](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/137)) ([#313](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/313)) ([f5930b5](https://github.com/OpenSkyHawk/OpenSkyhawk/commit/f5930b5cf8a3ec305681ac6eea4c4822727ca415))
