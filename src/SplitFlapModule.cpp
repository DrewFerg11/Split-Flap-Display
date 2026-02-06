#include "SplitFlapModule.h"

// Array of characters, in order, the first item is located on the magnet on the
// character drum
const char SplitFlapModule::StandardChars[37] = {' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
                                                 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y',
                                                 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

const char SplitFlapModule::ExtendedChars[48] = {
    ' ', 'A', 'B', 'C', 'D', 'E',  'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
    'P', 'Q', 'R', 'S', 'T', 'U',  'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4',
    '5', '6', '7', '8', '9', '\'', ':', '?', '!', '.', '-', '/', '$', '@', '#', '%',
};

bool hasErrored = false;

// Default Constructor
SplitFlapModule::SplitFlapModule()
    : address(0), position(0), stepNumber(0), stepsPerRot(0), chars(StandardChars), numChars(37), charSetSize(37), wire(&Wire) {
    magnetPosition = 710;
}

// Constructor implementation
SplitFlapModule::SplitFlapModule(
    uint8_t I2Caddress, int stepsPerFullRotation, int stepOffset, int magnetPos, int charsetSize, TwoWire &wireInstance
)
    : address(I2Caddress), position(0), stepNumber(0), stepsPerRot(stepsPerFullRotation), charSetSize(charsetSize), wire(&wireInstance) {
    magnetPosition = magnetPos + stepOffset;

    chars = (charsetSize == 48) ? ExtendedChars : StandardChars;
    numChars = (charsetSize == 48) ? 48 : 37;
}

void SplitFlapModule::writeIO(uint16_t data) {
    wire->beginTransmission(address);
    wire->write(data & 0xFF);        // Send lower byte
    wire->write((data >> 8) & 0xFF); // Send upper byte

    byte error = wire->endTransmission();

    if (error > 0 && ! hasErrored) {
        hasErrored = true; // Set the error flag
        Serial.print("Error writing data to module ");
        Serial.print(address);
        Serial.print(", error code: ");
        Serial.println(error); // Error codes:
        // 0 = success
        // 1 = data too long to fit in transmit buffer
        // 2 = received NACK on transmit of address
        // 3 = received NACK on transmit of data
        // 4 = other error
    }
}

bool SplitFlapModule::writeIOWithRetry(uint16_t data, int retryCount) {
    int attempts = 0;
    int maxAttempts = (retryCount > 0) ? (retryCount + 1) : 1;
    
    while (attempts < maxAttempts) {
        wire->beginTransmission(address);
        wire->write(data & 0xFF);
        wire->write((data >> 8) & 0xFF);
        byte error = wire->endTransmission();
        
        if (error == 0) {
            return true;  // Success
        }
        
        attempts++;
        if (attempts < maxAttempts) {
            delayMicroseconds(50);  // Brief delay before retry
        }
    }
    
    // All attempts failed
    if (!hasErrored) {
        hasErrored = true;
        Serial.printf("[ACC] Module 0x%02X: I2C write failed after %d attempts\n", address, maxAttempts);
    }
    return false;
}

// Init Module, Setup IO Board
void SplitFlapModule::init() {
    float stepSize = (float) stepsPerRot / (float) numChars;
    float currentPosition = 0;
    for (int i = 0; i < numChars; i++) {
        charPositions[i] = (int) currentPosition;
        currentPosition += stepSize;
    }

    uint16_t initState = 0b1111111111100001; // Pin 15 (17) as INPUT, Pins 1-4 as OUTPUT
    writeIO(initState);

    stop();                                  // Write all motor coil inputs LOW

    int initDelay = 100;

    delay(initDelay);
    step();
    delay(initDelay);
    step();
    delay(initDelay);
    step();
    delay(initDelay);
    step();
    delay(initDelay);

    stop();
}

int SplitFlapModule::getCharPosition(char inputChar) {
    inputChar = toupper(inputChar);
    for (int i = 0; i < charSetSize; i++) {
        if (chars[i] == inputChar) {
            return charPositions[i];
        }
    }
    return 0; // Character not found, return blank
}

void SplitFlapModule::stop() {
    uint16_t stepState = 0b1111111111100001;
    writeIO(stepState);
}

void SplitFlapModule::start() {
    stepNumber = (stepNumber + 3) % 4; // effectively take one off stepNumber
    step(false);                       // write the "previous" step high again, in case turned off
}

void SplitFlapModule::step(bool updatePosition) {
    // Basic step without accuracy features - delegate to full method
    step(0, 0, updatePosition);
}

void SplitFlapModule::step(int settleUs, int retryCount, bool updatePosition) {
    uint16_t stepState;
    switch (stepNumber) {
        case 0:
            stepState = 0b1111111111100111;
            break;
        case 1:
            stepState = 0b1111111111110011;
            break;
        case 2:
            stepState = 0b1111111111111001;
            break;
        case 3:
            stepState = 0b1111111111101101;
            break;
    }
    
    // Use retry logic if enabled, otherwise basic write
    if (retryCount > 0) {
        lastStepSuccess = writeIOWithRetry(stepState, retryCount);
    } else {
        writeIO(stepState);
        lastStepSuccess = true;  // Basic write doesn't track success
    }
    
    // Apply settle delay if configured (allows motor coils to stabilize)
    if (settleUs > 0) {
        delayMicroseconds(settleUs);
    }
    
    if (updatePosition) {
        position = (position + 1) % stepsPerRot;
        stepNumber = (stepNumber + 1) % 4;
    }
}

bool SplitFlapModule::readHallEffectSensor() {
    if (hasErrored) {
        return false;
    }

    uint8_t requestBytes = 2;
    wire->requestFrom(address, requestBytes);
    // Make sure the data is available
    if (wire->available() == 2) {
        uint16_t inputState = 0;

        // Read the two bytes and combine them into a 16-bit value
        inputState = wire->read();             // Read the lower byte
        inputState |= (wire->read() << 8);     // Read the upper byte and shift it left

        return (inputState & (1 << 15)) != 0; // If bit is 15, return HIGH, else LOW
    }
    return false;
}

// Priority 2: Position Error Statistics - Record correction for diagnostics
void SplitFlapModule::recordPositionError(int error) {
    accuracyStats.totalCorrections++;
    accuracyStats.maxError = max(accuracyStats.maxError, abs(error));
    
    // Update rolling average (exponential smoothing, alpha = 0.1)
    float alpha = 0.1f;
    accuracyStats.avgError = alpha * abs(error) + (1.0f - alpha) * accuracyStats.avgError;
    
    accuracyStats.lastCorrectionTime = millis();
}
