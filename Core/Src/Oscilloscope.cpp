#include "Oscilloscope.hpp"

Oscilloscope::Oscilloscope(AdcDma* adc)
    : mAdc(adc), mAverage(0), mAmplitude(0),
      mMin(0), mMax(0), mStatsValid(false),
      mGain(1.0f), mTriggerLevel(2048)
{
}

// === УПРАВЛЕНИЕ ЗАХВАТОМ ===

void Oscilloscope::capture()
{
    if (mAdc == nullptr) return;

    // Сбрасываем флаг готовности
    mAdc->clearDataReadyFlag();

    // Запускаем захват
    mAdc->startContinuousCapture();

    // Ждём завершения (до 10 мс)
    uint32_t start = HAL_GetTick();
    while (!mAdc->isDataReady() && (HAL_GetTick() - start < 10)) {
        // Ждём
    }

    // Останавливаем захват
    mAdc->stopCapture();

    // Сбрасываем кэш статистики
    mStatsValid = false;
}

void Oscilloscope::startCapture()
{
    if (mAdc == nullptr) return;
    mAdc->clearDataReadyFlag();
    mAdc->startContinuousCapture();
    mStatsValid = false;
}

bool Oscilloscope::isCaptureComplete()
{
    if (mAdc == nullptr) return false;
    return mAdc->isDataReady();
}

// === ДОСТУП К ДАННЫМ ===

const uint16_t* Oscilloscope::getBuffer() const
{
    if (mAdc == nullptr) return nullptr;
    return mAdc->getBuffer();
}

uint16_t Oscilloscope::getBufferSize() const
{
    if (mAdc == nullptr) return 0;
    return mAdc->getBufferSize();
}

// === СТАТИСТИКА СИГНАЛА ===

uint16_t Oscilloscope::getAverage() const
{
    calculateStats();
    return mAverage;
}

uint16_t Oscilloscope::getAmplitude() const
{
    calculateStats();
    return mAmplitude;
}

uint16_t Oscilloscope::getMin() const
{
    calculateStats();
    return mMin;
}

uint16_t Oscilloscope::getMax() const
{
    calculateStats();
    return mMax;
}

// === НАСТРОЙКИ ОТОБРАЖЕНИЯ ===

void Oscilloscope::setGain(float gain)
{
    mGain = gain;
}

float Oscilloscope::getGain() const
{
    return mGain;
}

void Oscilloscope::setTriggerLevel(uint16_t level)
{
    mTriggerLevel = level;
}

uint16_t Oscilloscope::getTriggerLevel() const
{
    return mTriggerLevel;
}

// === ВНУТРЕННИЙ МЕТОД РАСЧЁТА СТАТИСТИКИ ===

void Oscilloscope::calculateStats() const
{
    if (mStatsValid) return;  // Уже рассчитано

    const uint16_t* buffer = getBuffer();
    uint16_t bufferSize = getBufferSize();

    if (buffer == nullptr || bufferSize == 0) {
        mAverage = 0;
        mAmplitude = 0;
        mMin = 0;
        mMax = 0;
        return;
    }

    uint32_t sum = 0;
    uint16_t minVal = 65535;
    uint16_t maxVal = 0;

    for (uint16_t i = 0; i < bufferSize; i++) {
        uint16_t val = buffer[i];
        sum += val;
        if (val < minVal) minVal = val;
        if (val > maxVal) maxVal = val;
    }

    mAverage = (uint16_t)(sum / bufferSize);
    mMin = minVal;
    mMax = maxVal;
    mAmplitude = maxVal - minVal;
    mStatsValid = true;
}
