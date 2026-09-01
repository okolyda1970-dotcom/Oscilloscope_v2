#include "Scanner.hpp"

Scanner::Scanner(UartProtocol* uart, ADC_HandleTypeDef* hadc,
                 uint16_t* adcBuffer, uint16_t adcBufferSize)
    : mUart(uart), mHadc(hadc), mAdcBuffer(adcBuffer),
      mAdcBufferSize(adcBufferSize),
      mSpanMhz(320.0f), mStepMhz(0.5f), mCenterMhz(2000.0f),
      mSettleMs(30), mResultsSize(0), mCurrentIndex(0),
      mRunning(false), mPaused(false), mFinished(false),
      mWaitingSettle(false), mSettleStartTime(0),
      mMaxLevel(0), mPeakIndex(0)
{
    // Инициализация буфера результатов
    for (uint16_t i = 0; i < MAX_POINTS; i++) {
        mResults[i] = 0;
    }
}

// === НАСТРОЙКА ПАРАМЕТРОВ ===

void Scanner::setSpan(float spanMhz) {
    mSpanMhz = spanMhz;
}

void Scanner::setStep(float stepMhz) {
    mStepMhz = stepMhz;
}

void Scanner::setCenter(float centerMhz) {
    mCenterMhz = centerMhz;
}

void Scanner::setSettleTime(uint16_t ms) {
    mSettleMs = ms;
}

// === УПРАВЛЕНИЕ СКАНИРОВАНИЕМ ===

void Scanner::start() {
    // Вычисляем количество точек
    mResultsSize = (uint16_t)(mSpanMhz / mStepMhz);
    if (mResultsSize > MAX_POINTS) mResultsSize = MAX_POINTS;

    // Сброс состояния
    mCurrentIndex = 0;
    mMaxLevel = 0;
    mPeakIndex = 0;
    mRunning = true;
    mPaused = false;
    mFinished = false;
    mWaitingSettle = false;

    // Устанавливаем начальную частоту
    float startFreq = getFrequencyAtIndex(0);
    mUart->setFrequency(startFreq);
    mSettleStartTime = HAL_GetTick();
    mWaitingSettle = true;
}

void Scanner::stop() {
    mRunning = false;
    mPaused = false;
    mFinished = false;
}

void Scanner::pause() {
    if (mRunning) {
        mPaused = true;
    }
}

void Scanner::resume() {
    if (mRunning && mPaused) {
        mPaused = false;
        // Перезапуск с текущей позиции
        float currentFreq = getFrequencyAtIndex(mCurrentIndex);
        mUart->setFrequency(currentFreq);
        mSettleStartTime = HAL_GetTick();
        mWaitingSettle = true;
    }
}

// === ОБНОВЛЕНИЕ (вызывать каждый цикл) ===

void Scanner::update() {
    if (!mRunning || mPaused || mFinished) return;

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
        mRunning = false;
        mFinished = true;
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
    return mCenterMhz - mSpanMhz / 2.0f;
}

float Scanner::getEndFrequency() const {
    return mCenterMhz + mSpanMhz / 2.0f;
}

// === ВНУТРЕННИЕ МЕТОДЫ ===

uint16_t Scanner::measureLevel() {
    // Запускаем АЦП через DMA
    HAL_ADC_Start_DMA(mHadc, (uint32_t*)mAdcBuffer, mAdcBufferSize);

    // Ждём завершения (до 10 мс)
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < 10) {
        // Проверяем флаг завершения АЦП
        // Здесь можно использовать HAL_ADC_PollForConversion
    }

    // Останавливаем АЦП
    HAL_ADC_Stop_DMA(mHadc);

    // Вычисляем амплитуду
    uint16_t minVal = 65535, maxVal = 0;
    for (uint16_t j = 0; j < mAdcBufferSize; j++) {
        uint16_t val = mAdcBuffer[j];
        if (val < minVal) minVal = val;
        if (val > maxVal) maxVal = val;
    }
    return maxVal - minVal;
}

float Scanner::getFrequencyAtIndex(uint16_t index) const {
    float startFreq = mCenterMhz - mSpanMhz / 2.0f;
    return startFreq + index * mStepMhz;
}
