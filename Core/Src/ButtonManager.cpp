#include "ButtonManager.hpp"

ButtonManager::ButtonManager(GPIO_TypeDef* port1, uint16_t pin1,
                             GPIO_TypeDef* port2, uint16_t pin2,
                             GPIO_TypeDef* port3, uint16_t pin3,
                             GPIO_TypeDef* port4, uint16_t pin4)
{
    mButtons[BTN1].port = port1;  mButtons[BTN1].pin = pin1;
    mButtons[BTN2].port = port2;  mButtons[BTN2].pin = pin2;
    mButtons[BTN3].port = port3;  mButtons[BTN3].pin = pin3;
    mButtons[BTN4].port = port4;  mButtons[BTN4].pin = pin4;

    for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
        mButtons[i].currentState = false;
        mButtons[i].rawState = false;
        mButtons[i].lastDebounceTime = 0;
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

        // === ШАГ 1: АНТИДРЕБЕЗГ ===
        // Если сырое состояние изменилось — обновляем таймер
        if (raw != btn.rawState) {
            btn.rawState = raw;
            btn.lastDebounceTime = now;
        }

        // Проверяем, что состояние стабильно
        uint32_t debounceElapsed = now - btn.lastDebounceTime;
        if (debounceElapsed < DEBOUNCE_MS) {
            // Ещё не стабилизировалось — пропускаем
            continue;
        }

        // === ШАГ 2: ОБРАБОТКА НОВОГО СТАБИЛЬНОГО СОСТОЯНИЯ ===
        if (raw != btn.currentState) {
            // Состояние изменилось и стабилизировалось
            btn.currentState = raw;

            if (raw) {
                // === КНОПКА НАЖАТА ===
                btn.pressStartTime = now;
                btn.longPressFired = false;
                // НЕ генерируем событие — ждём либо отпускания, либо долгого нажатия
            } else {
                // === КНОПКА ОТПУЩЕНА ===
                uint32_t holdTime = now - btn.pressStartTime;

                if (!btn.longPressFired && holdTime >= MIN_PRESS_MS) {
                    // Короткое нажатие (не было LONG_PRESS, держали > MIN_PRESS_MS)
                    btn.pendingEvent = PRESSED;
                }
                // Если был LONG_PRESS или держали < MIN_PRESS_MS (дребезг) — ничего не генерируем

                btn.longPressFired = false;
            }
        }

        // === ШАГ 3: ПРОВЕРКА ДОЛГОГО НАЖАТИЯ (ПОКА КНОПКА НАЖАТА) ===
        if (btn.currentState && !btn.longPressFired) {
            uint32_t holdTime = now - btn.pressStartTime;
            if (holdTime >= LONG_PRESS_MS) {
                btn.longPressFired = true;
                btn.pendingEvent = LONG_PRESS;
                // НЕ ждём отпускания — событие уже сгенерировано
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
