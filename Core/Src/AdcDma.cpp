#include "AdcDma.hpp"

// Инициализация статических членов
AdcDma* AdcDma::sInstances[MAX_INSTANCES] = {nullptr};
uint8_t AdcDma::sInstanceCount = 0;

AdcDma::AdcDma(ADC_HandleTypeDef* hadc, DMA_HandleTypeDef* hdma,
               uint16_t* buffer, uint16_t bufferSize)
    : mHadc(hadc), mHdma(hdma), mBuffer(buffer),
      mBufferSize(bufferSize), mDataReady(false)
{
    if (sInstanceCount < MAX_INSTANCES) {
        sInstances[sInstanceCount++] = this;
    }
}

AdcDma::~AdcDma()
{
    for (uint8_t i = 0; i < sInstanceCount; i++) {
        if (sInstances[i] == this) {
            for (uint8_t j = i; j < sInstanceCount - 1; j++) {
                sInstances[j] = sInstances[j + 1];
            }
            sInstances[sInstanceCount - 1] = nullptr;
            sInstanceCount--;
            break;
        }
    }
}

void AdcDma::startContinuousCapture()
{
    if (mHadc == nullptr || mBuffer == nullptr) return;

    // СНАЧАЛА останавливаем (если был запущен)
    HAL_ADC_Stop_DMA(mHadc);
    HAL_Delay(1);

    // Сбрасываем состояние
    __HAL_ADC_CLEAR_FLAG(mHadc, ADC_FLAG_EOS | ADC_FLAG_EOC |
                         ADC_FLAG_OVR | ADC_FLAG_EOSMP);

    // Запускаем заново
    HAL_StatusTypeDef status = HAL_ADC_Start_DMA(mHadc, (uint32_t*)mBuffer, mBufferSize);
    if (status != HAL_OK) {
        // Ошибка запуска — пробуем ещё раз
        HAL_ADC_Stop_DMA(mHadc);
        HAL_Delay(5);
        HAL_ADC_Start_DMA(mHadc, (uint32_t*)mBuffer, mBufferSize);
    }

    mDataReady = false;
}

void AdcDma::stopCapture()
{
    if (mHadc != nullptr) {
        HAL_ADC_Stop_DMA(mHadc);
    }
    mDataReady = false;
}

bool AdcDma::isDataReady() const { return mDataReady; }
uint16_t* AdcDma::getBuffer() const { return mBuffer; }
uint16_t AdcDma::getBufferSize() const { return mBufferSize; }
void AdcDma::clearDataReadyFlag() { mDataReady = false; }

AdcDma* AdcDma::findByHadc(ADC_HandleTypeDef* hadc)
{
    for (uint8_t i = 0; i < sInstanceCount; i++) {
        if (sInstances[i] != nullptr && sInstances[i]->mHadc == hadc) {
            return sInstances[i];
        }
    }
    return nullptr;
}

void AdcDma::convCpltCallback(ADC_HandleTypeDef* hadc)
{
    AdcDma* instance = findByHadc(hadc);
    if (instance != nullptr) {
        instance->mDataReady = true;
    }
}

void AdcDma::errorCallback(ADC_HandleTypeDef* hadc)
{
    AdcDma* instance = findByHadc(hadc);
    if (instance != nullptr) {
        instance->stopCapture();
        instance->startContinuousCapture();
    }
}
