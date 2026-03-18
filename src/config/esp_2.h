#pragma once

// =============================================================================
// ESP 2 - WORKER
// Logical displays: CLUSTER_OFFSET to (CLUSTER_OFFSET + CLUSTER_DISPLAY_COUNT - 1)
// =============================================================================

#define DISPLAY_NAME          "esp2"
#define CLUSTER_ROLE          "worker"
#define CLUSTER_ID            "2"
#define CLUSTER_OFFSET        2    // First logical display index owned by this ESP
#define CLUSTER_DISPLAY_COUNT 1    // Number of displays on this ESP (adjust freely)

// --- I2C Bus 0 (Wire) ---
#define WIRE0_MUX_ADDRS        "112"
#define WIRE0_CH_MOD_ADDRS_112 "32,33,34,35,36,37,38,39;;;;;;;;"

// --- I2C Bus 1 (Wire1) ---
#define WIRE1_MUX_ADDRS        "112"
#define WIRE1_CH_MOD_ADDRS_112 ";;;;;;;;"
