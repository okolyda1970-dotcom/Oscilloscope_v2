#include "Scanner.hpp"

Scanner::Scanner(UartProtocol* uart, ADC_HandleTypeDef* hadc,
                 uint16_t* adcBuffer, uint16_t adcBufferSize)
    : mUart(uart), mHadc(hadc), mAdcBuffer(adcBuffer),
      mAdcBufferSize(adcBufferSize),
      mCenterMhz(900.0f), mSettleMs(30),
      mResultsSize(DISPLAY_WIDTH),
      mState(IDLE), mCurrentIndex(0),
      mWaitingSettle(false), mSettleStartTime(0),
      mMaxLevel(0), mPeakIndex(0), mStoppedFrequency(0.0f)
{
    // Инициализация буфера результатов
    for (uint16_t i = 0; i < DISPLAY_WIDTH; i++) {
        mResults[i] = 0;
    }
}

// === НАСТРОЙКА ===

void Scanner::setCenter(float centerMhz) {
    mCenterMhz = centerMhz;
}

void Scanner::setSettleTime(uint16_t ms) {
    mSettleMs = ms;
}

// === УПРАВЛЕНИЕ ===

void Scanner::start() {
    // Сброс состояния
    mCurrentIndex = 0;
    mMaxLevel = 0;
    mPeakIndex = 0;
    mState = SCANNING;
    mWaitingSettle = false;
    mStoppedFrequency = 0.0f;

    // Очищаем буфер результатов
    for (uint16_t i = 0; i < DISPLAY_WIDTH; i++) {
        mResults[i] = 0;
    }

    // Устанавливаем начальную частоту
    float startFreq = getFrequencyAtIndex(0);
    mUart->setFrequency(startFreq);
    mSettleStartTime = HAL_GetTick();
    mWaitingSettle = true;
}

void Scanner::stop() {
    if (mState == SCANNING) {
        // Сохраняем текущую частоту при остановке
        mStoppedFrequency = getFrequencyAtIndex(mCurrentIndex);
        mState = STOPPED;
        mWaitingSettle = false;
    }
}

// === ОБНОВЛЕНИЕ ===

void Scanner::update() {
    if (mState != SCANNING) return;

    // Ждём стабилизации частоты
    if (mWaitingSettle) {
        if (HAL_GetTick() - mSettleStartTime >= mSettleMs) {
            mWaitingSettle = false;
        } else {
            return;  // Ещё ждём
        }
    }

    // Измеряем уровень на текущей частоте
    uint16_t level = measureLevel();
    mResults[mCurrentIndex] = level;

    // Обновляем максимум
    if (level > mMaxLevel) {
        mMaxLevel = level;
        mPeakIndex = mCurrentIndex;
    }

    // Переходим к следующей точке
    mCurrentIndex++;

    if (mCurrentIndex >= mResultsSize) {
        // Сканирование завершено
        mState = FINISHED;
    } else {
        // Устанавливаем следующую частоту
        float nextFreq = getFrequencyAtIndex(mCurrentIndex);
        mUart->setFrequency(nextFreq);
        mSettleStartTime = HAL_GetTick();
        mWaitingSettle = true;
    }
}

// === СОСТОЯНИЕ ===

uint16_t Scanner::getProgress() const {
    if (mResultsSize == 0) return 0;
    return (uint16_t)((uint32_t)mCurrentIndex * 100 / mResultsSize);
}

// === РЕЗУЛЬТАТЫ ===

float Scanner::getPeakFrequency() const {
    return getFrequencyAtIndex(mPeakIndex);
}

float Scanner::getStartFrequency() const {
    return mCenterMhz - SPAN_MHZ / 2.0f;
}

float Scanner::getEndFrequency() const {
    return mCenterMhz + SPAN_MHZ / 2.0f;
}

float Scanner::getCurrentFrequency() const {
    return getFrequencyAtIndex(mCurrentIndex);
}

float Scanner::getStoppedFrequency() const {
    return mStoppedFrequency;
}

// === ВНУТРЕННИЕ МЕТОДЫ ===

uint16_t Scanner::measureLevel() {
    // Запускаем АЦП через DMA
    HAL_ADC_Start_DMA(mHadc, (uint32_t*)mAdcBuffer, mAdcBufferSize);

    // Ждём завершения (до 10 мс)
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < 10) {
        // Проверяем флаг завершения АЦП
    }

    // Останавливаем АЦП
    HAL_ADC_Stop_DMA(mHadc);

    // Вычисляем амплитуду (размах)
    uint16_t minVal = 65535, maxVal = 0;
    for (uint16_t j = 0; j < mAdcBufferSize; j++) {
        uint16_t val = mAdcBuffer[j];
        if (val < minVal) minVal = val;
        if (val > maxVal) maxVal = val;
    }
    return maxVal - minVal;
}

float Scanner::getFrequencyAtIndex(uint16_t index) const {
    float startFreq = mCenterMhz - SPAN_MHZ / 2.0f;
    return startFreq + index * STEP_MHZ;
}
