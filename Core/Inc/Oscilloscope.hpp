#ifndef OSCILLOSCOPE_HPP_
#define OSCILLOSCOPE_HPP_

#include "stm32h7xx_hal.h"
#include "AdcDma.hpp"
#include <stdint.h>

class Oscilloscope {
public:
    // Конструктор: принимает объект AdcDma
    Oscilloscope(AdcDma* adc);

    // === УПРАВЛЕНИЕ ЗАХВАТОМ ===
    void capture();              // Запускает захват и ждёт завершения
    void startCapture();         // Запускает захват (не блокирующий)
    bool isCaptureComplete();    // Проверяет завершение захвата

    // === ДОСТУП К ДАННЫМ ===
    const uint16_t* getBuffer() const;
    uint16_t getBufferSize() const;

    // === СТАТИСТИКА СИГНАЛА ===
    uint16_t getAverage() const;
    uint16_t getAmplitude() const;
    uint16_t getMin() const;
    uint16_t getMax() const;

    // === НАСТРОЙКИ ОТОБРАЖЕНИЯ ===
    void setGain(float gain);
    float getGain() const;
    void setTriggerLevel(uint16_t level);
    uint16_t getTriggerLevel() const;

private:
    AdcDma* mAdc;

    // Кэшированные значения статистики
    mutable uint16_t mAverage;
    mutable uint16_t mAmplitude;
    mutable uint16_t mMin;
    mutable uint16_t mMax;
    mutable bool mStatsValid;

    // Настройки отображения
    float mGain;
    uint16_t mTriggerLevel;

    // Внутренний метод расчёта статистики
    void calculateStats() const;
};

#endif // OSCILLOSCOPE_HPP_
