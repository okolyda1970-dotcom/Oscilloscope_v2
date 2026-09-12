#ifndef SCANNER_HPP_
#define SCANNER_HPP_

#include "stm32h7xx_hal.h"
#include "UartProtocol.hpp"
#include <stdint.h>

class Scanner {
public:


    // Состояния сканера
    enum State {
        IDLE,       // Не запущен
        SCANNING,   // Идёт сканирование
        FINISHED,   // Завершён (все 160 точек)
        STOPPED     // Остановлен вручную
    };

    // Константы
    static constexpr uint16_t DISPLAY_WIDTH = 160;  // ← constexpr
    static constexpr float SPAN_MHZ = 160.0f;        // ← constexpr
    static constexpr float STEP_MHZ = 1.0f;          // ← constexpr

    // Конструктор
    Scanner(UartProtocol* uart, ADC_HandleTypeDef* hadc,
            uint16_t* adcBuffer, uint16_t adcBufferSize);

    // === НАСТРОЙКА ===
    void setCenter(float centerMhz);
    void setSettleTime(uint16_t ms);

    // === УПРАВЛЕНИЕ ===
    void start();                    // Запуск сканирования
    void stop();                     // Остановка (переход в STOPPED)

    // === ОБНОВЛЕНИЕ (вызывать каждый цикл) ===
    void update();

    // === СОСТОЯНИЕ ===
    State getState() const { return mState; }
    bool isIdle() const { return mState == IDLE; }
    bool isScanning() const { return mState == SCANNING; }
    bool isFinished() const { return mState == FINISHED; }
    bool isStopped() const { return mState == STOPPED; }
    uint16_t getCurrentIndex() const { return mCurrentIndex; }  // ← ДОБАВЬТЕ ЭТУ СТРОКУ
    uint16_t getProgress() const;    // 0-100%

    // === РЕЗУЛЬТАТЫ ===
    const uint16_t* getResults() const { return mResults; }
    uint16_t getResultsSize() const { return mResultsSize; }
    uint16_t getMaxLevel() const { return mMaxLevel; }
    float getPeakFrequency() const;
    float getStartFrequency() const;
    float getEndFrequency() const;
    float getCurrentFrequency() const;  // ← Новая: текущая частота при остановке
    float getStoppedFrequency() const;  // ← Новая: частота на которой остановились

private:
    // Внешние объекты
    UartProtocol* mUart;
    ADC_HandleTypeDef* mHadc;
    uint16_t* mAdcBuffer;
    uint16_t mAdcBufferSize;

    // Параметры
    float mCenterMhz;
    uint16_t mSettleMs;
    uint16_t mResultsSize;  // Всегда 160

    // Результаты
    uint16_t mResults[DISPLAY_WIDTH];

    // Состояние
    State mState;
    uint16_t mCurrentIndex;
    bool mWaitingSettle;
    uint32_t mSettleStartTime;

    // Статистика
    uint16_t mMaxLevel;
    uint16_t mPeakIndex;
    float mStoppedFrequency;  // Частота на которой остановились

    // Внутренние методы
    uint16_t measureLevel();
    float getFrequencyAtIndex(uint16_t index) const;
};

#endif // SCANNER_HPP_
