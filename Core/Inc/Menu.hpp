#ifndef MENU_HPP_
#define MENU_HPP_

#include "stm32h7xx_hal.h"
#include "Display.hpp"
#include "ButtonManager.hpp"
#include <stdint.h>

class Menu {
public:
    // Состояния меню
    enum State {
        STATE_HIDDEN,        // Меню скрыто
        STATE_MAIN,          // Главное меню
        STATE_SETTINGS,      // Список параметров
        STATE_EDIT           // Редактирование параметра
    };

    // Режимы работы
    enum Mode {
        MODE_OSCILLOSCOPE = 0,
        MODE_SCANNER = 1
    };

    // Параметры осциллографа
    enum OscParam {
        OSC_FREQ = 0,
        OSC_OFFSET,
        OSC_ATT1,
        OSC_ATT2,
        OSC_TIMEBASE,
        OSC_TRIGGER,
        OSC_PARAM_COUNT
    };

    // Параметры сканера
    enum ScanParam {
        SCAN_CENTER = 0,
        SCAN_SPAN,
        SCAN_OFFSET,
        SCAN_PARAM_COUNT
    };

    // Конструктор
    Menu(Display* display, ButtonManager* buttons);

    // === ГЛАВНЫЕ МЕТОДЫ ===
    void update(uint8_t currentMode);  // Вызывать в цикле
    void show();                       // Показать меню
    void hide();                       // Скрыть меню
    bool isVisible() const;

    // === ПОЛУЧЕНИЕ ЗНАЧЕНИЙ ===
    float getFrequency() const;
    uint8_t getOffset() const;
    bool getAttenuator1() const;
    bool getAttenuator2() const;
    uint8_t getTimebase() const;
    uint16_t getTrigger() const;
    float getScanCenter() const;
    float getScanSpan() const;

    // === ПРОВЕРКА ИЗМЕНЕНИЙ ===
    bool isFrequencyChanged() const;
    bool isOffsetChanged() const;
    bool isAtt1Changed() const;
    bool isAtt2Changed() const;
    bool isTimebaseChanged() const;
    bool isTriggerChanged() const;
    bool isScanCenterChanged() const;
    bool isScanSpanChanged() const;

    // === СБРОС ФЛАГОВ ===
    void resetFlags();

private:
    Display* mDisplay;
    ButtonManager* mButtons;

    State mState;
    uint8_t mSelectedItem;
    uint8_t mCurrentMode;

    // Значения параметров
    float mFrequency;
    uint8_t mOffset;
    bool mAtt1;
    bool mAtt2;
    uint8_t mTimebase;
    uint16_t mTrigger;
    float mScanCenter;
    float mScanSpan;

    // Флаги изменений
    bool mFreqChanged;
    bool mOffsetChanged;
    bool mAtt1Changed;
    bool mAtt2Changed;
    bool mTimebaseChanged;
    bool mTriggerChanged;
    bool mScanCenterChanged;
    bool mScanSpanChanged;

    // Долгое нажатие для вызова меню
    uint32_t mBtn3PressTime;
    bool mBtn3Pressed;

    // Оптимизация рендера
    bool mNeedRedraw;
    uint8_t mLastSelectedItem;
    State mLastState;

    // === РЕЗЕРВНЫЕ КОПИИ ДЛЯ ОТКАТА ===
    float mBackupFrequency;
    uint8_t mBackupOffset;
    bool mBackupAtt1;
    bool mBackupAtt2;
    uint8_t mBackupTimebase;
    uint16_t mBackupTrigger;
    float mBackupScanCenter;
    float mBackupScanSpan;

    // Внутренние методы
    void renderMain();
    void renderSettings();
    void renderEdit();
    void handleMain();
    void handleSettings();
    bool handleEdit();
    void adjustValue(int8_t delta);
};

#endif // MENU_HPP_
