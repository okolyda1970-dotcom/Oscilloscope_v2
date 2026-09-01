#include "PotReader.hpp"

PotReader::PotReader(AdcDma* adc)
    : mAdc(adc), mFilterStrength(4), mInitialized(false)
{
    for (uint8_t i = 0; i < POT_COUNT; i++) {
        mFiltered[i] = 0.0f;
    }
}

void PotReader::update()
{
    if (mAdc == nullptr) return;

    // Проверяем, готовы ли новые данные
    if (!mAdc->isDataReady()) return;

    uint16_t* buffer = mAdc->getBuffer();
    uint16_t bufferSize = mAdc->getBufferSize();

    if (bufferSize < POT_COUNT) return;

    // Коэффициент фильтра: alpha = 1 / 2^strength
    // strength=0: alpha=1.0 (без фильтра)
    // strength=4: alpha=0.0625 (сильное сглаживание)
    float alpha = 1.0f / (float)(1 << mFilterStrength);

    // Применяем экспоненциальный фильтр для каждого канала
    for (uint8_t i = 0; i < POT_COUNT; i++) {
        float newValue = (float)buffer[i];

        if (!mInitialized) {
            // Первое значение — просто присваиваем
            mFiltered[i] = newValue;
        } else {
            // Экспоненциальное сглаживание
            mFiltered[i] = mFiltered[i] * (1.0f - alpha) + newValue * alpha;
        }
    }

    mInitialized = true;

    // Сбрасываем флаг готовности
    mAdc->clearDataReadyFlag();
}

uint16_t PotReader::getRawValue(PotID pot) const
{
    if (pot >= POT_COUNT) return 0;
    return (uint16_t)mFiltered[pot];
}

float PotReader::getOffsetPercent() const
{
    // A0: 12-бит АЦП (0-4095) → 0.0 - 1.0
    return mFiltered[OFFSET] / 4095.0f;
}

float PotReader::getCenterPercent() const
{
    // A1: 12-бит АЦП (0-4095) → 0.0 - 1.0
    return mFiltered[CENTER] / 4095.0f;
}

void PotReader::setFilterStrength(uint8_t strength)
{
    if (strength > 8) strength = 8;
    mFilterStrength = strength;
}
