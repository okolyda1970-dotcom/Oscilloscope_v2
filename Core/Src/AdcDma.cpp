#include "AdcDma.hpp"

// Инициализация статического указателя
AdcDma* AdcDma::mInstance = nullptr;

AdcDma::AdcDma(ADC_HandleTypeDef* hadc, DMA_HandleTypeDef* hdma,
               uint16_t* buffer, uint16_t bufferSize)
    : mHadc(hadc), mHdma(hdma), mBuffer(buffer),
      mBufferSize(bufferSize), mDataReady(false)
{
    mInstance = this;
}

void AdcDma::startContinuousCapture()
{
    if (mHadc == nullptr || mBuffer == nullptr) return;

    // Запуск АЦП в циркулярном режиме с DMA
    HAL_ADC_Start_DMA(mHadc, (uint32_t*)mBuffer, mBufferSize);
    mDataReady = false;
}

void AdcDma::stopCapture()
{
    if (mHadc != nullptr) {
        HAL_ADC_Stop_DMA(mHadc);
    }
    mDataReady = false;
}

bool AdcDma::isDataReady() const
{
    return mDataReady;
}

uint16_t* AdcDma::getBuffer() const
{
    return mBuffer;
}

uint16_t AdcDma::getBufferSize() const
{
    return mBufferSize;
}

void AdcDma::clearDataReadyFlag()
{
    mDataReady = false;
}

// Колбэк завершения захвата (вызывается из HAL)
void AdcDma::convCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (mInstance != nullptr && mInstance->mHadc == hadc) {
        mInstance->mDataReady = true;
    }
}

// Колбэк ошибки
void AdcDma::errorCallback(ADC_HandleTypeDef* hadc)
{
    // Можно добавить обработку ошибок, например, перезапуск
    if (mInstance != nullptr && mInstance->mHadc == hadc) {
        mInstance->stopCapture();
        // Попытка перезапуска
        mInstance->startContinuousCapture();
    }
}
