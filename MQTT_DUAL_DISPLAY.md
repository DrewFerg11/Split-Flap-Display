# MQTT Dual Display Control

## Overview
Added support for controlling both displays simultaneously via MQTT, matching the "Update Both Displays" functionality in the web UI.

## New MQTT Topic

**Topic:** `splitflap/{mdns}/displays/set`

Where `{mdns}` is your device's mDNS name (e.g., `splitflap`)

## Payload Format

```json
{
  "displays": [
    {
      "num": 1,
      "mode": 6,
      "text": "HELLO"
    },
    {
      "num": 2,
      "mode": 6,
      "text": "WORLD"
    }
  ]
}
```

### Fields

- `displays` (array): Array of display updates to apply
  - `num` (int): Display number (1 or 2)
  - `mode` (int): Display mode to set (0-12, typically 6 for custom text)
  - `text` (string, optional): Text to display (required when mode = 6)

## Example Commands

### Update Both Displays with Custom Text

```bash
mosquitto_pub -h <mqtt_broker_ip> -t "splitflap/splitflap/displays/set" -m '{"displays":[{"num":1,"mode":6,"text":"HELLO"},{"num":2,"mode":6,"text":"WORLD"}]}'
```

### Update Single Display via Dual Topic

```bash
mosquitto_pub -h <mqtt_broker_ip> -t "splitflap/splitflap/displays/set" -m '{"displays":[{"num":1,"mode":6,"text":"TEST"}]}'
```

### Change Display Mode Only

```bash
mosquitto_pub -h <mqtt_broker_ip> -t "splitflap/splitflap/displays/set" -m '{"displays":[{"num":1,"mode":2}]}'
```

Replace `<mqtt_broker_ip>` with your MQTT broker's IP address.

## Display Modes

- 0: Off
- 1: Clock (HH:MM)
- 2: Date (MM/DD)
- 3: Temperature
- 4: Humidity
- 5: Scrolling Text
- 6: Custom Text (requires `text` field)
- 7-12: Other modes

## Legacy Topic (Still Supported)

The original single-display command topic still works:

**Topic:** `splitflap/{mdns}/set`
**Payload:** Plain text string (no JSON)

This will update Display 1 only using the legacy blocking method.

## Implementation Details

- Commands are queued to the appropriate display queue for non-blocking execution
- Mode changes are persisted to settings (`d1_mode`, `d2_mode`)
- Text updates (mode 6) create DisplayCommand structs and queue them for parallel processing
- Both displays update simultaneously via FreeRTOS tasks
- If a queue is full, the command will retry for 100ms before timing out

## Troubleshooting

Enable serial monitoring to see MQTT debug messages:

```
[MQTT] Message received on topic splitflap/splitflap/displays/set: {...}
[MQTT] Display 1: mode=6
[MQTT] Queued text 'HELLO' for display 1
[MQTT] Display 2: mode=6
[MQTT] Queued text 'WORLD' for display 2
```

If you see "Queue not available" errors, ensure:
1. Display queues are initialized
2. `splitflapMqtt.setDisplayQueues()` was called in setup
3. The display number (1 or 2) is valid
