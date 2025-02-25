
#include "Brewpi.h"
#include "Pins.h"
#include "ActuatorArduinoPin.h"
#include "Display.h"


void DigitalPinActuator::setActive(bool active_setting) {

    bool oldActive = active;
    this->active = active_setting;
    digitalWrite(pin, active_setting^invert ? HIGH : LOW);

    if (oldActive != active) {
        // We toggled one of the pins, which can cause issues for the display. Delay slightly to let everything settle down, then reinit the display
        delay(100);
        display.init();
        display.printAll();
    }

}

