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
#define CLUSTER_DISPLAY_COUNT 2    // Number of displays on this ESP (adjust freely)

// --- I2C Bus 0 (Wire) ---
#define WIRE0_MUX_ADDRS        "112"
#define WIRE0_CH_MOD_ADDRS_112 "32,33,34,35,36,37,38,39;;;;;;;;"

// --- I2C Bus 1 (Wire1) ---
#define WIRE1_MUX_ADDRS        "112"
#define WIRE1_CH_MOD_ADDRS_112 "32,33,34,35,36,37,38,39;;;;;;;;"
