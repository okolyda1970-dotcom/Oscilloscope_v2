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
      mBtn3PressTime(0), mBtn3Pressed(false)
{
}

void Menu::update(uint8_t currentMode)
{
    mCurrentMode = currentMode;

    // === ОТКРЫТИЕ МЕНЮ ПО КОРОТКОМУ НАЖАТИЮ BTN3 ===
    if (mState == STATE_HIDDEN) {
        ButtonManager::ButtonEvent evt3 = mButtons->getEvent(ButtonManager::BTN3);
        if (evt3 == ButtonManager::PRESSED) {
            show();
        }
        return;  // Меню скрыто — дальше не обрабатываем
    }

    // === ОБРАБОТКА СОСТОЯНИЙ МЕНЮ ===
    switch (mState) {
        case STATE_MAIN:
            handleMain();
            renderMain();
            break;
        case STATE_SETTINGS:
            handleSettings();
            renderSettings();
            break;
        case STATE_EDIT:
            handleEdit();
            renderEdit();
            break;
        default:
            break;
    }
}

void Menu::show()
{
    mState = STATE_MAIN;
    mSelectedItem = 0;
    mDisplay->clear(COLOR_WHITE);
}

void Menu::hide()
{
    mState = STATE_HIDDEN;
    mDisplay->clear(COLOR_WHITE);
}

bool Menu::isVisible() const
{
    return mState != STATE_HIDDEN;
}

// === ГЛАВНОЕ МЕНЮ ===
void Menu::renderMain()
{
    const char* items[] = {"Oscilloscope", "Scanner", "Settings"};

    mDisplay->clear(COLOR_WHITE);
    mDisplay->drawString(0, 0, "MAIN MENU", COLOR_BLACK, COLOR_WHITE);

    for (uint8_t i = 0; i < 3; i++) {
        uint16_t color = (i == mSelectedItem) ? COLOR_BLUE : COLOR_BLACK;
        uint16_t y = 20 + i * 15;

        if (i == mSelectedItem) {
            mDisplay->drawString(0, y, ">", COLOR_RED, COLOR_WHITE);
        }
        mDisplay->drawString(10, y, items[i], color, COLOR_WHITE);
    }
}

void Menu::handleMain()
{
    ButtonManager::ButtonEvent evt1 = mButtons->getEvent(ButtonManager::BTN1);
    ButtonManager::ButtonEvent evt2 = mButtons->getEvent(ButtonManager::BTN2);
    ButtonManager::ButtonEvent evt3 = mButtons->getEvent(ButtonManager::BTN3);
    ButtonManager::ButtonEvent evt4 = mButtons->getEvent(ButtonManager::BTN4);

    if (evt1 == ButtonManager::PRESSED) {
        mSelectedItem = (mSelectedItem + 1) % 3;
    }
    if (evt2 == ButtonManager::PRESSED) {
        mSelectedItem = (mSelectedItem + 2) % 3;  // -1 mod 3
    }
    if (evt3 == ButtonManager::PRESSED) {
        hide();
    }
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
    mDisplay->clear(COLOR_WHITE);
    mDisplay->drawString(0, 0, "SETTINGS", COLOR_BLACK, COLOR_WHITE);

    uint8_t paramCount = 0;
    char items[10][20];

    if (mCurrentMode == MODE_OSCILLOSCOPE) {
        paramCount = OSC_PARAM_COUNT;
        strcpy(items[0], "Frequency");
        strcpy(items[1], "Offset");
        strcpy(items[2], "Attenuator 1");
        strcpy(items[3], "Attenuator 2");
        strcpy(items[4], "Timebase");
        strcpy(items[5], "Trigger");
    } else {
        paramCount = SCAN_PARAM_COUNT;
        strcpy(items[0], "Center");
        strcpy(items[1], "Span");
        strcpy(items[2], "Offset");
    }

    for (uint8_t i = 0; i < paramCount; i++) {
        uint16_t color = (i == mSelectedItem) ? COLOR_BLUE : COLOR_BLACK;
        uint16_t y = 20 + i * 12;

        if (i == mSelectedItem) {
            mDisplay->drawString(0, y, ">", COLOR_RED, COLOR_WHITE);
        }
        mDisplay->drawString(10, y, items[i], color, COLOR_WHITE);
    }
}

void Menu::handleSettings()
{
    uint8_t paramCount = (mCurrentMode == MODE_OSCILLOSCOPE) ?
                         OSC_PARAM_COUNT : SCAN_PARAM_COUNT;

    ButtonManager::ButtonEvent evt1 = mButtons->getEvent(ButtonManager::BTN1);
    ButtonManager::ButtonEvent evt2 = mButtons->getEvent(ButtonManager::BTN2);
    ButtonManager::ButtonEvent evt3 = mButtons->getEvent(ButtonManager::BTN3);
    ButtonManager::ButtonEvent evt4 = mButtons->getEvent(ButtonManager::BTN4);

    if (evt1 == ButtonManager::PRESSED) {
        mSelectedItem = (mSelectedItem + 1) % paramCount;
    }
    if (evt2 == ButtonManager::PRESSED) {
        mSelectedItem = (mSelectedItem + paramCount - 1) % paramCount;
    }
    if (evt3 == ButtonManager::PRESSED) {
        mState = STATE_MAIN;
        mSelectedItem = 0;
    }
    if (evt4 == ButtonManager::PRESSED) {
        mState = STATE_EDIT;
    }
}

// === РЕДАКТИРОВАНИЕ ===
void Menu::renderEdit()
{
    mDisplay->clear(COLOR_WHITE);

    char title[20];
    char value[32];

    if (mCurrentMode == MODE_OSCILLOSCOPE) {
        switch (mSelectedItem) {
            case OSC_FREQ:
                strcpy(title, "Frequency");
                sprintf(value, "%.1f MHz", mFrequency);
                break;
            case OSC_OFFSET:
                strcpy(title, "Offset");
                sprintf(value, "%d", mOffset);
                break;
            case OSC_ATT1:
                strcpy(title, "Attenuator 1");
                strcpy(value, mAtt1 ? "ON" : "OFF");
                break;
            case OSC_ATT2:
                strcpy(title, "Attenuator 2");
                strcpy(value, mAtt2 ? "ON" : "OFF");
                break;
            case OSC_TIMEBASE:
                strcpy(title, "Timebase");
                sprintf(value, "%dx", 1 << mTimebase);
                break;
            case OSC_TRIGGER:
                strcpy(title, "Trigger");
                sprintf(value, "%d", mTrigger);
                break;
        }
    } else {
        switch (mSelectedItem) {
            case SCAN_CENTER:
                strcpy(title, "Center");
                sprintf(value, "%.1f MHz", mScanCenter);
                break;
            case SCAN_SPAN:
                strcpy(title, "Span");
                sprintf(value, "%.0f MHz", mScanSpan);
                break;
            case SCAN_OFFSET:
                strcpy(title, "Offset");
                sprintf(value, "%d", mOffset);
                break;
        }
    }

    mDisplay->drawString(0, 0, title, COLOR_BLACK, COLOR_WHITE);
    mDisplay->drawString(0, 30, value, COLOR_GREEN, COLOR_WHITE);
    mDisplay->drawString(0, 50, "BTN1:+ BTN2:-", COLOR_BLACK, COLOR_WHITE);
    mDisplay->drawString(0, 60, "BTN3:Back BTN4:OK", COLOR_BLACK, COLOR_WHITE);
}

void Menu::handleEdit()
{
    ButtonManager::ButtonEvent evt1 = mButtons->getEvent(ButtonManager::BTN1);
    ButtonManager::ButtonEvent evt2 = mButtons->getEvent(ButtonManager::BTN2);
    ButtonManager::ButtonEvent evt3 = mButtons->getEvent(ButtonManager::BTN3);
    ButtonManager::ButtonEvent evt4 = mButtons->getEvent(ButtonManager::BTN4);

    if (evt1 == ButtonManager::PRESSED) adjustValue(1);
    if (evt2 == ButtonManager::PRESSED) adjustValue(-1);
    if (evt3 == ButtonManager::PRESSED) {
        mState = STATE_SETTINGS;
    }
    if (evt4 == ButtonManager::PRESSED) {
        mState = STATE_SETTINGS;
    }
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
                mOffset += delta;
                if (mOffset > 255) mOffset = 0;
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
                mTimebase += delta;
                if (mTimebase > 3) mTimebase = 0;
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
                mOffset += delta;
                if (mOffset > 255) mOffset = 0;
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
