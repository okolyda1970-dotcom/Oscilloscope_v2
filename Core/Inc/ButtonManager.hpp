#ifndef BUTTONMANAGER_HPP_
#define BUTTONMANAGER_HPP_

#include "stm32h7xx_hal.h"
#include <stdint.h>

class ButtonManager {
public:
    // Идентификаторы кнопок
    enum ButtonID { BTN1, BTN2, BTN3, BTN4, BUTTON_COUNT };

    // События кнопок
    enum ButtonEvent { NONE, PRESSED, RELEASED, LONG_PRESS };

    // Конструктор
    ButtonManager(GPIO_TypeDef* port1, uint16_t pin1,
                  GPIO_TypeDef* port2, uint16_t pin2,
                  GPIO_TypeDef* port3, uint16_t pin3,
                  GPIO_TypeDef* port4, uint16_t pin4);

    // Обновление состояния (вызывать каждые 10-20 мс)
    void update();

    // Получить последнее событие для кнопки (и сбросить его)
    ButtonEvent getEvent(ButtonID btn);

    // Проверить, нажата ли кнопка сейчас
    bool isPressed(ButtonID btn) const;

private:
    struct ButtonState {
        GPIO_TypeDef* port;
        uint16_t pin;
        bool currentState;       // Стабильное состояние (после антидребезга)
        bool rawState;           // Сырое состояние пина
        uint32_t lastDebounceTime; // Время последнего изменения
        uint32_t pressStartTime; // Время начала стабильного нажатия
        bool longPressFired;     // Долгое нажатие уже сгенерировано
        ButtonEvent pendingEvent;// Ожидающее событие
    };

    ButtonState mButtons[BUTTON_COUNT];

    // Настройки
    static const uint32_t DEBOUNCE_MS = 15;        // Антидребезг
    static const uint32_t MIN_PRESS_MS = 20;       // Мин. время нажатия (фильтр дребезга)
    static const uint32_t LONG_PRESS_MS = 2000;    // Долгое нажатие (2 секунды)

    // Внутренний метод чтения пина
    bool readPin(ButtonID btn) const;
};

#endif // BUTTONMANAGER_HPP_
