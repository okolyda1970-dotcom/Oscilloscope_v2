#include "Menu.hpp"
#include <cstdio>
#include <cstring>

Menu::Menu(Display* display, ButtonManager* buttons)
    : mDisplay(display), mButtons(buttons),
      mState(STATE_HIDDEN), mSelectedItem(0), mCurrentMode(0),
      mFrequency(900.0f), mOffset(128), mAtt1(false), mAtt2(false),
      mTimebase(0), mTrigger(2048),
      mScanCenter(900.0f), mScanSpan(320.0f),
      mFreqChanged(false), mOffsetChanged(false),
      mAtt1Changed(false), mAtt2Changed(false),
      mTimebaseChanged(false), mTriggerChanged(false),
      mScanCenterChanged(false), mScanSpanChanged(false),
      mBtn3PressTime(0), mBtn3Pressed(false),
      mNeedRedraw(true), mLastSelectedItem(0), mLastState(STATE_HIDDEN),
      mBackupFrequency(900.0f), mBackupOffset(128),
      mBackupAtt1(false), mBackupAtt2(false),
      mBackupTimebase(0), mBackupTrigger(2048),
      mBackupScanCenter(900.0f), mBackupScanSpan(320.0f)
{
}

void Menu::update(uint8_t currentMode)
{
    mCurrentMode = currentMode;

    // === ЕСЛИ МЕНЮ СКРЫТО: ждём нажатие кнопки 3 для открытия ===
    if (mState == STATE_HIDDEN) {
        ButtonManager::ButtonEvent evt3 = mButtons->getEvent(ButtonManager::BTN3);
        if (evt3 == ButtonManager::PRESSED || evt3 == ButtonManager::LONG_PRESS) {
            mButtons->getEvent(ButtonManager::BTN1);
            mButtons->getEvent(ButtonManager::BTN2);
            mButtons->getEvent(ButtonManager::BTN4);
            show();
        }
        return;
    }

    // === ОТЛАЖИВАЕМ ИЗМЕНЕНИЯ ДЛЯ РЕНДЕРА ===
    bool stateChanged = (mState != mLastState);
    bool selectionChanged = (mSelectedItem != mLastSelectedItem);

    if (stateChanged || selectionChanged) {
        mNeedRedraw = true;
        mLastState = mState;
        mLastSelectedItem = mSelectedItem;
    }

    // === ПРИНУДИТЕЛЬНЫЙ СБРОС ПРИ СМЕНЕ СОСТОЯНИЯ ===
    if (stateChanged) {
        mDisplay->clearArea(0, 0, 160, 128, COLOR_WHITE);
    }

    // === ОБРАБОТКА СОСТОЯНИЙ ===
    bool valueChanged = false;

    switch (mState) {
        case STATE_MAIN:
            handleMain();
            break;
        case STATE_SETTINGS:
            handleSettings();
            break;
        case STATE_EDIT:
            valueChanged = handleEdit();
            break;
        default:
            break;
    }

    if (valueChanged) {
        mNeedRedraw = true;
    }

    // === РЕНДЕР ТОЛЬКО ПРИ ИЗМЕНЕНИЯХ ===
    if (mNeedRedraw) {
        switch (mState) {
            case STATE_MAIN:
                renderMain();
                break;
            case STATE_SETTINGS:
                renderSettings();
                break;
            case STATE_EDIT:
                renderEdit();
                break;
            default:
                break;
        }
        mNeedRedraw = false;
    }
}

void Menu::show()
{
    mState = STATE_MAIN;
    mSelectedItem = 0;
    mNeedRedraw = true;
    mLastState = STATE_HIDDEN;
    mLastSelectedItem = 255;
    mDisplay->clearArea(0, 0, 160, 128, COLOR_WHITE);
}

void Menu::hide()
{
    mState = STATE_HIDDEN;
    mNeedRedraw = true;
    mDisplay->clearArea(0, 0, 160, 128, COLOR_WHITE);
}

bool Menu::isVisible() const
{
    return mState != STATE_HIDDEN;
}

// === ГЛАВНОЕ МЕНЮ ===
void Menu::renderMain()
{
    const char* items[] = {"Oscilloscope", "Scanner", "Settings"};

    // Стираем ВЕСЬ экран (надёжно, без наложений)
    mDisplay->clearArea(0, 0, 160, 128, COLOR_WHITE);

    mDisplay->drawString(0, 0, "MAIN MENU", COLOR_BLACK, COLOR_WHITE);
    mDisplay->drawString(0, 10, "------------------", COLOR_BLACK, COLOR_WHITE);

    for (uint8_t i = 0; i < 3; i++) {
        uint16_t color = (i == mSelectedItem) ? COLOR_BLUE : COLOR_BLACK;
        uint16_t y = 20 + i * 15;

        if (i == mSelectedItem) {
            mDisplay->drawString(0, y, ">", COLOR_RED, COLOR_WHITE);
        } else {
            mDisplay->drawString(0, y, " ", COLOR_BLACK, COLOR_WHITE);
        }
        mDisplay->drawString(10, y, items[i], color, COLOR_WHITE);
    }

    mDisplay->drawString(0, 75, "BTN4:OK BTN3:Back", COLOR_BLACK, COLOR_WHITE);
}

void Menu::handleMain()
{
    ButtonManager::ButtonEvent evt1 = mButtons->getEvent(ButtonManager::BTN1);
    ButtonManager::ButtonEvent evt2 = mButtons->getEvent(ButtonManager::BTN2);
    ButtonManager::ButtonEvent evt3 = mButtons->getEvent(ButtonManager::BTN3);
    ButtonManager::ButtonEvent evt4 = mButtons->getEvent(ButtonManager::BTN4);

    // BTN1 = ВВЕРХ
    if (evt1 == ButtonManager::PRESSED) {
        mSelectedItem = (mSelectedItem + 2) % 3;
    }
    // BTN2 = ВНИЗ
    if (evt2 == ButtonManager::PRESSED) {
        mSelectedItem = (mSelectedItem + 1) % 3;
    }
    // BTN3 = НАЗАД (выход из меню)
    if (evt3 == ButtonManager::PRESSED || evt3 == ButtonManager::LONG_PRESS) {
        hide();
        return;
    }
    // BTN4 = ВЫБРАТЬ
    if (evt4 == ButtonManager::PRESSED) {
        if (mSelectedItem == 0) {
            mCurrentMode = MODE_OSCILLOSCOPE;
            hide();
        } else if (mSelectedItem == 1) {
            mCurrentMode = MODE_SCANNER;
            hide();
        } else if (mSelectedItem == 2) {
            mState = STATE_SETTINGS;
            mSelectedItem = 0;
        }
    }
}

// === НАСТРОЙКИ ===
void Menu::renderSettings()
{
    // Стираем ВЕСЬ экран (надёжно, без наложений)
    mDisplay->clearArea(0, 0, 160, 128, COLOR_WHITE);

    mDisplay->drawString(0, 0, "SETTINGS", COLOR_BLACK, COLOR_WHITE);
    mDisplay->drawString(0, 10, "------------------", COLOR_BLACK, COLOR_WHITE);

    uint8_t paramCount = 0;
    const char* items[10];

    if (mCurrentMode == MODE_OSCILLOSCOPE) {
        paramCount = OSC_PARAM_COUNT;
        items[0] = "Frequency";
        items[1] = "Offset";
        items[2] = "Attenuator 1";
        items[3] = "Attenuator 2";
        items[4] = "Timebase";
        items[5] = "Trigger";
    } else {
        paramCount = SCAN_PARAM_COUNT;
        items[0] = "Center";
        items[1] = "Span";
        items[2] = "Offset";
    }

    for (uint8_t i = 0; i < paramCount; i++) {
        uint16_t color = (i == mSelectedItem) ? COLOR_BLUE : COLOR_BLACK;
        uint16_t y = 20 + i * 12;

        if (i == mSelectedItem) {
            mDisplay->drawString(0, y, ">", COLOR_RED, COLOR_WHITE);
        } else {
            mDisplay->drawString(0, y, " ", COLOR_BLACK, COLOR_WHITE);
        }
        mDisplay->drawString(10, y, items[i], color, COLOR_WHITE);
    }

    mDisplay->drawString(0, 100, "BTN4:Edit BTN3:Back", COLOR_BLACK, COLOR_WHITE);
}

void Menu::handleSettings()
{
    uint8_t paramCount = (mCurrentMode == MODE_OSCILLOSCOPE) ?
                         OSC_PARAM_COUNT : SCAN_PARAM_COUNT;

    ButtonManager::ButtonEvent evt1 = mButtons->getEvent(ButtonManager::BTN1);
    ButtonManager::ButtonEvent evt2 = mButtons->getEvent(ButtonManager::BTN2);
    ButtonManager::ButtonEvent evt3 = mButtons->getEvent(ButtonManager::BTN3);
    ButtonManager::ButtonEvent evt4 = mButtons->getEvent(ButtonManager::BTN4);

    // BTN1 = ВВЕРХ
    if (evt1 == ButtonManager::PRESSED) {
        mSelectedItem = (mSelectedItem + paramCount - 1) % paramCount;
    }
    // BTN2 = ВНИЗ
    if (evt2 == ButtonManager::PRESSED) {
        mSelectedItem = (mSelectedItem + 1) % paramCount;
    }
    // BTN3 = НАЗАД
    if (evt3 == ButtonManager::PRESSED || evt3 == ButtonManager::LONG_PRESS) {
        mState = STATE_MAIN;
        mSelectedItem = 0;
        return;
    }
    // BTN4 = РЕДАКТИРОВАТЬ
    if (evt4 == ButtonManager::PRESSED) {
        // === СОХРАНЯЕМ ТЕКУЩИЕ ЗНАЧЕНИЯ ДЛЯ ОТКАТА ===
        mBackupFrequency = mFrequency;
        mBackupOffset = mOffset;
        mBackupAtt1 = mAtt1;
        mBackupAtt2 = mAtt2;
        mBackupTimebase = mTimebase;
        mBackupTrigger = mTrigger;
        mBackupScanCenter = mScanCenter;
        mBackupScanSpan = mScanSpan;

        mState = STATE_EDIT;
    }
}

// === РЕДАКТИРОВАНИЕ ===
void Menu::renderEdit()
{
    const char* title = "";
    char value[32];
    value[0] = '\0';

    if (mCurrentMode == MODE_OSCILLOSCOPE) {
        switch (mSelectedItem) {
            case OSC_FREQ:
                title = "Frequency";
                sprintf(value, "%.1f MHz", mFrequency);
                break;
            case OSC_OFFSET:
                title = "Offset";
                sprintf(value, "%d", mOffset);
                break;
            case OSC_ATT1:
                title = "Attenuator 1";
                sprintf(value, "%s", mAtt1 ? "ON" : "OFF");
                break;
            case OSC_ATT2:
                title = "Attenuator 2";
                sprintf(value, "%s", mAtt2 ? "ON" : "OFF");
                break;
            case OSC_TIMEBASE:
                title = "Timebase";
                sprintf(value, "%dx", 1 << mTimebase);
                break;
            case OSC_TRIGGER:
                title = "Trigger";
                sprintf(value, "%d", mTrigger);
                break;
            default:
                title = "?";
                break;
        }
    } else {
        switch (mSelectedItem) {
            case SCAN_CENTER:
                title = "Center";
                sprintf(value, "%.1f MHz", mScanCenter);
                break;
            case SCAN_SPAN:
                title = "Span";
                sprintf(value, "%.0f MHz", mScanSpan);
                break;
            case SCAN_OFFSET:
                title = "Offset";
                sprintf(value, "%d", mOffset);
                break;
            default:
                title = "?";
                break;
        }
    }

    // Стираем ВЕСЬ экран (надёжно, без наложений)
    mDisplay->clearArea(0, 0, 160, 128, COLOR_WHITE);

    mDisplay->drawString(0, 0, title, COLOR_BLACK, COLOR_WHITE);
    mDisplay->drawString(0, 10, "------------------", COLOR_BLACK, COLOR_WHITE);

    // Значение крупно
    mDisplay->drawString(0, 30, value, COLOR_BLUE, COLOR_WHITE);

    // Подсказки
    mDisplay->drawString(0, 55, "BTN1:- BTN2:+", COLOR_BLACK, COLOR_WHITE);
    mDisplay->drawString(0, 65, "BTN4:OK BTN3:Cancel", COLOR_BLACK, COLOR_WHITE);
}

bool Menu::handleEdit()
{
    ButtonManager::ButtonEvent evt1 = mButtons->getEvent(ButtonManager::BTN1);
    ButtonManager::ButtonEvent evt2 = mButtons->getEvent(ButtonManager::BTN2);
    ButtonManager::ButtonEvent evt3 = mButtons->getEvent(ButtonManager::BTN3);
    ButtonManager::ButtonEvent evt4 = mButtons->getEvent(ButtonManager::BTN4);

    bool changed = false;

    // BTN1 = МИНУС
    if (evt1 == ButtonManager::PRESSED) {
        adjustValue(-1);
        changed = true;
    }
    // BTN2 = ПЛЮС
    if (evt2 == ButtonManager::PRESSED) {
        adjustValue(1);
        changed = true;
    }
    // BTN3 = НАЗАД (ОТКАТ БЕЗ СОХРАНЕНИЯ)
    if (evt3 == ButtonManager::PRESSED || evt3 == ButtonManager::LONG_PRESS) {
        // === ВОССТАНАВЛИВАЕМ ЗНАЧЕНИЯ ===
        mFrequency = mBackupFrequency;
        mOffset = mBackupOffset;
        mAtt1 = mBackupAtt1;
        mAtt2 = mBackupAtt2;
        mTimebase = mBackupTimebase;
        mTrigger = mBackupTrigger;
        mScanCenter = mBackupScanCenter;
        mScanSpan = mBackupScanSpan;

        // === СБРАСЫВАЕМ ФЛАГИ ИЗМЕНЕНИЙ ===
        resetFlags();

        mState = STATE_SETTINGS;
        return false;
    }
    // BTN4 = ПРИМЕНИТЬ (СОХРАНИТЬ)
    if (evt4 == ButtonManager::PRESSED) {
        // Значения уже изменены — просто выходим
        mState = STATE_SETTINGS;
        return false;
    }

    return changed;
}

void Menu::adjustValue(int8_t delta)
{
    if (mCurrentMode == MODE_OSCILLOSCOPE) {
        switch (mSelectedItem) {
            case OSC_FREQ:
                mFrequency += delta * 0.5f;
                if (mFrequency < 50.0f) mFrequency = 50.0f;
                if (mFrequency > 4000.0f) mFrequency = 4000.0f;
                mFreqChanged = true;
                break;
            case OSC_OFFSET:
                if (delta > 0 && mOffset < 255) mOffset++;
                else if (delta < 0 && mOffset > 0) mOffset--;
                mOffsetChanged = true;
                break;
            case OSC_ATT1:
                mAtt1 = !mAtt1;
                mAtt1Changed = true;
                break;
            case OSC_ATT2:
                mAtt2 = !mAtt2;
                mAtt2Changed = true;
                break;
            case OSC_TIMEBASE:
                if (delta > 0 && mTimebase < 3) mTimebase++;
                else if (delta < 0 && mTimebase > 0) mTimebase--;
                mTimebaseChanged = true;
                break;
            case OSC_TRIGGER:
                mTrigger += delta * 10;
                if (mTrigger > 4095) mTrigger = 4095;
                mTriggerChanged = true;
                break;
        }
    } else {
        switch (mSelectedItem) {
            case SCAN_CENTER:
                mScanCenter += delta * 1.0f;
                if (mScanCenter < 50.0f) mScanCenter = 50.0f;
                if (mScanCenter > 4000.0f) mScanCenter = 4000.0f;
                mScanCenterChanged = true;
                break;
            case SCAN_SPAN:
                mScanSpan += delta * 10.0f;
                if (mScanSpan < 10.0f) mScanSpan = 10.0f;
                if (mScanSpan > 1000.0f) mScanSpan = 1000.0f;
                mScanSpanChanged = true;
                break;
            case SCAN_OFFSET:
                if (delta > 0 && mOffset < 255) mOffset++;
                else if (delta < 0 && mOffset > 0) mOffset--;
                mOffsetChanged = true;
                break;
        }
    }
}

// === ГЕТТЕРЫ ===
float Menu::getFrequency() const { return mFrequency; }
uint8_t Menu::getOffset() const { return mOffset; }
bool Menu::getAttenuator1() const { return mAtt1; }
bool Menu::getAttenuator2() const { return mAtt2; }
uint8_t Menu::getTimebase() const { return mTimebase; }
uint16_t Menu::getTrigger() const { return mTrigger; }
float Menu::getScanCenter() const { return mScanCenter; }
float Menu::getScanSpan() const { return mScanSpan; }

// === ФЛАГИ ИЗМЕНЕНИЙ ===
bool Menu::isFrequencyChanged() const { return mFreqChanged; }
bool Menu::isOffsetChanged() const { return mOffsetChanged; }
bool Menu::isAtt1Changed() const { return mAtt1Changed; }
bool Menu::isAtt2Changed() const { return mAtt2Changed; }
bool Menu::isTimebaseChanged() const { return mTimebaseChanged; }
bool Menu::isTriggerChanged() const { return mTriggerChanged; }
bool Menu::isScanCenterChanged() const { return mScanCenterChanged; }
bool Menu::isScanSpanChanged() const { return mScanSpanChanged; }

void Menu::resetFlags()
{
    mFreqChanged = false;
    mOffsetChanged = false;
    mAtt1Changed = false;
    mAtt2Changed = false;
    mTimebaseChanged = false;
    mTriggerChanged = false;
    mScanCenterChanged = false;
    mScanSpanChanged = false;
}
