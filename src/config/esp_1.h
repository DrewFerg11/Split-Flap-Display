#pragma once

// =============================================================================
// ESP 1 - MAIN
// Handles: Web UI, MQTT, Home Assistant, UART coordination, local displays
// Logical displays: 0 to (CLUSTER_DISPLAY_COUNT - 1)
// =============================================================================

#define DISPLAY_NAME          "esp1"
#define CLUSTER_ROLE          "main"
#define CLUSTER_ID            "1"
#define CLUSTER_OFFSET        0    // First logical display index owned by this ESP
#define CLUSTER_DISPLAY_COUNT 1    // Number of displays on this ESP (adjust freely)

// --- I2C Bus 0 (Wire) ---
// Example: 3 displays on channels 0, 1, 2 — each with 5 modules at I2C addrs 32-36
#define WIRE0_MUX_ADDRS        "112"
#define WIRE0_CH_MOD_ADDRS_112 "32,33,34,35,36;;;;;;;;"

// --- I2C Bus 1 (Wire1) ---
// Example: 2 displays on channels 0-1, each with 5 modules at I2C addrs 32-36.
#define WIRE1_MUX_ADDRS        "112"
#define WIRE1_CH_MOD_ADDRS_112 ";;;;;;;;;"
