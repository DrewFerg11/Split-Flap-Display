# TCA9548A I2C Multiplexer Scanner

## Overview
Added I2C scanner that runs at startup to detect the TCA9548A multiplexer and scan all 8 channels for connected devices.

## Hardware Configuration

### TCA9548A Connections
- **VCC** → ESP32 3.3V
- **GND** → ESP32 GND  
- **SDA** → ESP32 GPIO21 (default)
- **SCL** → ESP32 GPIO22 (default)
- **A0/A1/A2** → GND (address = 0x70)
- **RST** → 3.3V or leave unconnected

### Display PCB Connections
- **Channel 0 (SD0/SC0)** → Display 1 (modules 0x20-0x24)
- **Channel 1 (SD1/SC1)** → Display 2 (modules 0x20-0x24)
- **Channel 2 (SD2/SC2)** → Display 3 (modules 0x20-0x24)
- Channels 3-7 reserved for future expansion

### Power
- PSU +5V → All display PCBs VCC
- PSU GND → Common ground (ESP32, TCA9548A, all PCBs)
- PSU +5V → ESP32 VIN (or USB for testing)

## Expected Serial Output

### Successful Detection
```
=== TCA9548A I2C Multiplexer Scanner ===
TCA9548A found at address 0x70

Scanning channel 0: [0x20, 0x21, 0x22, 0x23, 0x24]
Scanning channel 1: [0x20, 0x21, 0x22, 0x23, 0x24]
Scanning channel 2: [0x20, 0x21, 0x22, 0x23, 0x24]
Scanning channel 3: []
Scanning channel 4: []
Scanning channel 5: []
Scanning channel 6: []
Scanning channel 7: []
=== End I2C Scanner ===
```

### Mux Not Detected
```
=== TCA9548A I2C Multiplexer Scanner ===
WARNING: TCA9548A not detected at 0x70
Scanning main I2C bus...
=== End I2C Scanner ===
```

## Implementation Details

### Files Modified
- `src/SplitFlapDisplay.h` - Added mux helper methods and `muxAddress` constant
- `src/SplitFlapDisplay.cpp` - Implemented scanner and channel selection

### Key Functions

#### `selectMuxChannel(uint8_t channel)`
Selects active TCA9548A channel by writing bit mask to 0x70.
```cpp
void SplitFlapDisplay::selectMuxChannel(uint8_t channel) {
    if (channel > 7) return;
    Wire.beginTransmission(muxAddress);
    Wire.write(1 << channel);  // Activate specific channel
    Wire.endTransmission();
}
```

#### `scanMuxChannels()`
Full I2C address sweep (0x08-0x77) on all 8 mux channels, logging detected devices per channel.

### Scanner Timing
- Runs once during `SplitFlapDisplay::init()` after `Wire.begin()`
- Occurs before module initialization
- Full scan takes ~1-2 seconds
- Uses 400kHz I2C clock speed

## Troubleshooting

### No Devices Detected on Any Channel
1. Check TCA9548A power (3.3V on VCC)
2. Verify A0/A1/A2 are grounded (address 0x70)
3. Check SDA/SCL connections to ESP32
4. Verify common ground between ESP32, mux, and displays

### Mux Not Found at 0x70
1. Confirm A0/A1/A2 address pins (all GND = 0x70)
2. Check 3.3V power to mux VCC
3. Verify I2C pullups (TCA9548A has internal pullups, but may need external 4.7kΩ on main bus)
4. Test with I2C scanner on main bus (disable mux detection temporarily)

### Devices Show on Wrong Channels
1. Verify SD0/SC0 through SD7/SC7 wiring
2. Check that displays are connected to intended channels
3. Confirm each display chain has proper I2C pullups (4.7kΩ on first PCB)

### Partial Detection (Missing Modules)
1. Check 5V power to all display PCBs
2. Verify I2C chaining between PCBs in same display
3. Ensure pullup resistors on first PCB of each display
4. Check module addresses in PCB firmware (should be 0x20-0x24)

## Next Steps

1. **Verify Hardware** - Confirm scanner output matches expected wiring
2. **Channel Mapping** - Add settings to map modules to mux channels
3. **Per-Channel Operations** - Modify `moveTo()`, `home()`, etc. to select channel before I2C access
4. **UI Integration** - Update web interface to control multiple displays
5. **MQTT Support** - Extend MQTT commands for multi-display control

## Build & Upload

```bash
# Clean build
pio run -e esp32_wroom -t clean

# Compile
pio run -e esp32_wroom

# Upload (via USB)
pio run -e esp32_wroom -t upload

# Monitor serial output
pio device monitor -b 115200
```

## Scanner Address Range

Full sweep covers standard I2C 7-bit addresses:
- **0x08-0x77** (excluding reserved 0x00-0x07 and 0x78-0x7F)
- **Display modules**: 0x20-0x24 (PCF8575 GPIO expanders)
- **TCA9548A mux**: 0x70 (on main bus)
- **Other common**: 0x68 (IMU), 0x76/0x77 (BME280), etc.

## Performance Notes

- Scanner adds ~1-2s to boot time
- Runs once at startup only
- Does not interfere with motor control timing
- Safe to run with displays powered on
- Full sweep helps catch wiring issues early
