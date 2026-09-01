#include "ButtonManager.hpp"

ButtonManager::ButtonManager(GPIO_TypeDef* port1, uint16_t pin1,
                             GPIO_TypeDef* port2, uint16_t pin2,
                             GPIO_TypeDef* port3, uint16_t pin3,
                             GPIO_TypeDef* port4, uint16_t pin4)
{
    // Сохраняем порты и пины
    mButtons[BTN1].port = port1;  mButtons[BTN1].pin = pin1;
    mButtons[BTN2].port = port2;  mButtons[BTN2].pin = pin2;
    mButtons[BTN3].port = port3;  mButtons[BTN3].pin = pin3;
    mButtons[BTN4].port = port4;  mButtons[BTN4].pin = pin4;

    // Инициализация состояний
    for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
        mButtons[i].currentState = false;
        mButtons[i].lastState = false;
        mButtons[i].rawState = false;
        mButtons[i].lastChangeTime = 0;
        mButtons[i].pressStartTime = 0;
        mButtons[i].longPressFired = false;
        mButtons[i].pendingEvent = NONE;
    }
}

void ButtonManager::update()
{
    uint32_t now = HAL_GetTick();

    for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
        ButtonState& btn = mButtons[i];
        bool raw = readPin((ButtonID)i);

        // === АНТИДРЕБЕЗГ ===
        if (raw != btn.rawState) {
            btn.rawState = raw;
            btn.lastChangeTime = now;
        }

        // Если состояние стабильно более DEBOUNCE_MS
        if ((now - btn.lastChangeTime) >= DEBOUNCE_MS && raw != btn.currentState) {
            btn.currentState = raw;

            if (btn.currentState) {
                // Кнопка нажата
                btn.pressStartTime = now;
                btn.longPressFired = false;
                btn.pendingEvent = PRESSED;
            } else {
                // Кнопка отпущена
                btn.pendingEvent = RELEASED;
            }
        }

        // === ДОЛГОЕ НАЖАТИЕ ===
        if (btn.currentState && !btn.longPressFired) {
            if ((now - btn.pressStartTime) >= LONG_PRESS_MS) {
                btn.longPressFired = true;
                btn.pendingEvent = LONG_PRESS;
            }
        }
    }
}

ButtonManager::ButtonEvent ButtonManager::getEvent(ButtonID btn)
{
    if (btn >= BUTTON_COUNT) return NONE;
    ButtonEvent evt = mButtons[btn].pendingEvent;
    mButtons[btn].pendingEvent = NONE;  // Сбрасываем после чтения
    return evt;
}

bool ButtonManager::isPressed(ButtonID btn) const
{
    if (btn >= BUTTON_COUNT) return false;
    return mButtons[btn].currentState;
}

bool ButtonManager::readPin(ButtonID btn) const
{
    // Кнопка нажата = низкий уровень (замыкание на GND)
    return HAL_GPIO_ReadPin(mButtons[btn].port, mButtons[btn].pin) == GPIO_PIN_RESET;
}
