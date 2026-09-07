#include <Arduino.h>
#include "config.h"

/*
 * Firmware skeleton for:
 *   ATtiny85 + single push button + keep-alive + LED strip
 *
 * The hardware is designed so that the button can wake the power module
 * while the MCU is OFF, while the MCU controls the keep-alive after boot.
 *
 * The exact LED library is intentionally not hard-coded here.
 * Replace setLedPattern() with the implementation for the chosen strip.
 */

enum class PowerState : uint8_t {
    RUNNING,
    SHUTDOWN_REQUESTED
};

static PowerState powerState = PowerState::RUNNING;
static uint8_t patternIndex = 0;

static bool buttonPressed()
{
    const bool raw = digitalRead(BUTTON_PIN);
    return BUTTON_ACTIVE_LOW ? (raw == LOW) : (raw == HIGH);
}

static void setKeepAlive(bool enabled)
{
    const bool level = KEEPALIVE_ACTIVE_HIGH ? enabled : !enabled;
    digitalWrite(KEEPALIVE_PIN, level ? HIGH : LOW);
}

static void setLedPattern(uint8_t index)
{
    // Placeholder for the LED-strip driver.
    // Example future implementation with Adafruit_NeoPixel:
    // strip.setPixelColor(...);
    // strip.show();
    (void)index;
}

static void shutdownSystem()
{
    // Turn LEDs off before power is removed.
    setLedPattern(0);

    // Stop actively keeping the converter alive.
    // The power module is then expected to enter its own idle/cutoff state.
    setKeepAlive(false);

    powerState = PowerState::SHUTDOWN_REQUESTED;

    // Do not immediately write more outputs here. If the hardware is correct,
    // the loss of keep-alive removes power from the MCU naturally.
}

static void handleButton()
{
    static bool previousPressed = false;
    static unsigned long pressStart = 0;
    static uint8_t clickCount = 0;
    static unsigned long lastRelease = 0;
    static bool longPressHandled = false;

    const unsigned long now = millis();
    const bool pressed = buttonPressed();

    if (pressed && !previousPressed) {
        pressStart = now;
        longPressHandled = false;
    }

    if (pressed && !longPressHandled) {
        if ((now - pressStart) >= LONG_PRESS_MS) {
            shutdownSystem();
            longPressHandled = true;
            clickCount = 0;
            previousPressed = pressed;
            return;
        }
    }

    if (!pressed && previousPressed) {
        const unsigned long pressDuration = now - pressStart;

        if (pressDuration < LONG_PRESS_MS) {
            clickCount++;
            lastRelease = now;

            // Single short click: change LED pattern.
            if (clickCount < SHUTDOWN_CLICK_COUNT) {
                patternIndex++;
                if (patternIndex >= 4) {
                    patternIndex = 0;
                }
                setLedPattern(patternIndex);
            }
        }
    }

    // Evaluate click sequence after the user stops clicking.
    if (clickCount > 0 && !pressed && (now - lastRelease) > CLICK_TIMEOUT_MS) {
        if (clickCount >= SHUTDOWN_CLICK_COUNT) {
            shutdownSystem();
        }
        clickCount = 0;
    }

    previousPressed = pressed;
}

void setup()
{
    pinMode(BUTTON_PIN, BUTTON_ACTIVE_LOW ? INPUT_PULLUP : INPUT);
    pinMode(KEEPALIVE_PIN, OUTPUT);
    pinMode(LED_DATA_PIN, OUTPUT);

    // Once the MCU is alive, keep the power stage alive.
    setKeepAlive(true);

    patternIndex = 0;
    setLedPattern(patternIndex);
}

void loop()
{
    if (powerState == PowerState::RUNNING) {
        handleButton();
    } else {
        // Waiting for power to disappear.
        // If needed, this section can be replaced with a deep-sleep routine.
        delay(50);
    }
}
