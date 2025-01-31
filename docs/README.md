# Wokwi 4-Pin FAN Chip

Simulates a PWM controlled FAN with 4 pins: PWM, TACHO, VCC, and GND.

Currently uses fixed parameters for the fan, but can be extended to support more features.
* Fan starts rotating above 25% duty cycle
* Fan speed is proportional to duty cycle (25%:2500..100%:6500 RPM)
* Fan outputs tacho signal: 50% duty cylce, 2 pulses per revolution
* Has a "break force" control to simulate broken or aged fan


## Pin names

| Name | Description              |
| ---- | ------------------------ |
| VCC | Power (not used)|
| GND | Ground (not used)|
| PWM | PWM input to control FAN speed (frequency is arbitrary)|
| TACHO | Tachometer output |

## Usage

To use this chip in your project, include it as a dependency in your `diagram.json` file:

```json
  "dependencies": {
    "chip-hdc2010": "github:ci4rail/wokwi-4pin-fan-chip@1.0.0"
  }
```

Then, add the chip to your circuit by adding a `chip-4pin-fan` item to the `parts` section of diagram.json:

```json
  "parts": {
    ...,
    { "type": "chip-4pin-fan", "id": "fan1", "attrs": { "channels": 1 } }
  },
```


