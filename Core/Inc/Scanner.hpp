#ifndef SCANNER_HPP_
#define SCANNER_HPP_

#include "stm32h7xx_hal.h"
#include "UartProtocol.hpp"
#include <stdint.h>

class Scanner {
public:
    // Максимальное количество точек сканирования
    static const uint16_t MAX_POINTS = 640;

    // Конструктор
    Scanner(UartProtocol* uart, ADC_HandleTypeDef* hadc,
            uint16_t* adcBuffer, uint16_t adcBufferSize);

    // === НАСТРОЙКА ПАРАМЕТРОВ ===
    void setSpan(float spanMhz);
    void setStep(float stepMhz);
    void setCenter(float centerMhz);
    void setSettleTime(uint16_t ms);

    // === УПРАВЛЕНИЕ СКАНИРОВАНИЕМ ===
    void start();
    void stop();
    void pause();
    void resume();

    // === ОБНОВЛЕНИЕ (вызывать каждый цикл) ===
    void update();

    // === СОСТОЯНИЕ ===
    bool isRunning() const { return mRunning; }
    bool isPaused() const { return mPaused; }
    bool isFinished() const { return mFinished; }
    uint16_t getProgress() const;  // 0-100%

    // === РЕЗУЛЬТАТЫ ===
    const uint16_t* getResults() const { return mResults; }
    uint16_t getResultsSize() const { return mResultsSize; }
    uint16_t getMaxLevel() const { return mMaxLevel; }
    float getPeakFrequency() const;
    float getStartFrequency() const;
    float getEndFrequency() const;

private:
    // Внешние объекты
    UartProtocol* mUart;
    ADC_HandleTypeDef* mHadc;
    uint16_t* mAdcBuffer;
    uint16_t mAdcBufferSize;

    // Параметры сканирования
    float mSpanMhz;
    float mStepMhz;
    float mCenterMhz;
    uint16_t mSettleMs;

    // Результаты
    uint16_t mResults[MAX_POINTS];
    uint16_t mResultsSize;

    // Состояние
    uint16_t mCurrentIndex;
    bool mRunning;
    bool mPaused;
    bool mFinished;
    bool mWaitingSettle;
    uint32_t mSettleStartTime;

    // Статистика
    uint16_t mMaxLevel;
    uint16_t mPeakIndex;

    // Внутренние методы
    uint16_t measureLevel();
    float getFrequencyAtIndex(uint16_t index) const;
};

#endif // SCANNER_HPP_
