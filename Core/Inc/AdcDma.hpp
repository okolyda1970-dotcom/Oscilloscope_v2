#ifndef ADCDMA_HPP_
#define ADCDMA_HPP_

#include "stm32h7xx_hal.h"
#include <stdint.h>

class AdcDma {
public:
    // Конструктор: передаём хендлы АЦП и DMA, буфер и его размер
    AdcDma(ADC_HandleTypeDef* hadc, DMA_HandleTypeDef* hdma,
           uint16_t* buffer, uint16_t bufferSize);

    // Запуск непрерывного захвата (циркулярный режим)
    void startContinuousCapture();

    // Остановка захвата
    void stopCapture();

    // Проверка, готовы ли новые данные
    bool isDataReady() const;

    // Получение указателя на буфер с данными
    uint16_t* getBuffer() const;

    // Получение размера буфера
    uint16_t getBufferSize() const;

    // Сброс флага готовности данных (вызывать после чтения)
    void clearDataReadyFlag();

    // Колбэки (вызываются из прерываний HAL)
    static void convCpltCallback(ADC_HandleTypeDef* hadc);
    static void errorCallback(ADC_HandleTypeDef* hadc);

private:
    ADC_HandleTypeDef* mHadc;
    DMA_HandleTypeDef* mHdma;
    uint16_t* mBuffer;
    uint16_t mBufferSize;
    volatile bool mDataReady;

    // Статический указатель на экземпляр для колбэков
    static AdcDma* mInstance;
};

#endif // ADCDMA_HPP_
