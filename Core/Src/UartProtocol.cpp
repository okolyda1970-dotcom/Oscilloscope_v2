#include "UartProtocol.hpp"

// === ИНИЦИАЛИЗАЦИЯ СТАТИЧЕСКИХ ЧЛЕНОВ ===
UartProtocol* UartProtocol::mInstance = nullptr;
volatile bool UartProtocol::flagTx = false;
volatile bool UartProtocol::flagRx = false;

// === КОНСТРУКТОР ===
UartProtocol::UartProtocol(UART_HandleTypeDef* huart)
    : mHuart(huart), mTxComplete(true), mRxComplete(false),
      currentFrequency(100.0), rfEnabled(true),
      successCount(0), errorCount(0)
{
    mInstance = this;
}

// === ДЕСТРУКТОР ===
UartProtocol::~UartProtocol()
{
    if (mInstance == this) {
        mInstance = nullptr;
    }
}

// === ОРИГИНАЛЬНЫЙ МЕТОД ОТПРАВКИ ===
void UartProtocol::sendCommand(uint32_t reg1, uint32_t reg2)
{
    mTxBuffer[0] = (uint8_t)(reg1 >> 24);
    mTxBuffer[1] = (uint8_t)(reg1 >> 16);
    mTxBuffer[2] = (uint8_t)(reg1 >> 8);
    mTxBuffer[3] = (uint8_t)(reg1);
    mTxBuffer[4] = (uint8_t)(reg2 >> 24);
    mTxBuffer[5] = (uint8_t)(reg2 >> 16);
    mTxBuffer[6] = (uint8_t)(reg2 >> 8);
    mTxBuffer[7] = (uint8_t)(reg2);
    mTxBuffer[8] = 0x55;

    if (!flagTx) {
        flagTx = true;
        mTxComplete = false;
        HAL_UART_Transmit_IT(mHuart, mTxBuffer, 9);
    }
}

// === ВЗВОД ПРИЁМНИКА ===
void UartProtocol::armRx()
{
    resetReceiver();
}

// === СБРОС ФЛАГА ПРИЁМА ===
void UartProtocol::clearRxFlag()
{
    mRxComplete = false;
}

// === ПЕРЕЗАПУСК ПРИЁМНИКА ===
void UartProtocol::resetReceiver()
{
    HAL_UART_AbortReceive(mHuart);
    __HAL_UART_CLEAR_FLAG(mHuart, UART_FLAG_PE | UART_FLAG_FE |
                          UART_FLAG_NE | UART_FLAG_ORE);
    volatile uint32_t dummy = mHuart->Instance->RDR;
    (void)dummy;
    HAL_UART_Receive_IT(mHuart, mRxBuffer, 9);
}

// === КОЛБЭК: ПЕРЕДАЧА ЗАВЕРШЕНА ===
void UartProtocol::txCompleteCallback(UART_HandleTypeDef* huart)
{
    if (mInstance != nullptr && mInstance->mHuart == huart) {
        mInstance->mTxComplete = true;
        flagTx = false;
    }
}

// === КОЛБЭК: ПРИЁМ ЗАВЕРШЁН ===
void UartProtocol::rxCompleteCallback(UART_HandleTypeDef* huart)
{
    if (mInstance != nullptr && mInstance->mHuart == huart) {
        mInstance->mRxComplete = true;
        flagRx = false;
    }
}

// === КОЛБЭК: ОШИБКА ===
void UartProtocol::errorCallback(UART_HandleTypeDef* huart)
{
    if (mInstance != nullptr && mInstance->mHuart == huart) {
        flagTx = false;
        flagRx = false;
        mInstance->resetReceiver();
    }
}

// === НОВЫЙ МЕТОД: ОТПРАВКА РЕГИСТРОВ С ПРОВЕРКОЙ ЭХО ===
bool UartProtocol::sendRegisters(uint32_t reg1, uint32_t reg2)
{
    // Формируем буфер передачи
    mTxBuffer[0] = (uint8_t)(reg1 >> 24);
    mTxBuffer[1] = (uint8_t)(reg1 >> 16);
    mTxBuffer[2] = (uint8_t)(reg1 >> 8);
    mTxBuffer[3] = (uint8_t)(reg1);
    mTxBuffer[4] = (uint8_t)(reg2 >> 24);
    mTxBuffer[5] = (uint8_t)(reg2 >> 16);
    mTxBuffer[6] = (uint8_t)(reg2 >> 8);
    mTxBuffer[7] = (uint8_t)(reg2);
    mTxBuffer[8] = 0x55;  // Маркер конца пакета

    // Сбрасываем флаги
    mTxComplete = false;
    mRxComplete = false;

    // Полный сброс приёмника
    HAL_UART_AbortReceive(mHuart);
    __HAL_UART_CLEAR_FLAG(mHuart, UART_FLAG_PE | UART_FLAG_FE | UART_FLAG_NE | UART_FLAG_ORE);
    volatile uint32_t dummy = mHuart->Instance->RDR;
    (void)dummy;
    HAL_Delay(2);

    // Отправка и сразу приём
    HAL_UART_Transmit_IT(mHuart, mTxBuffer, 9);
    HAL_UART_Receive_IT(mHuart, mRxBuffer, 9);

    // Ждём передачу
    uint32_t timeout = 100;
    while (!mTxComplete && timeout > 0) {
        HAL_Delay(1);
        timeout--;
    }

    if (!mTxComplete) {
        errorCount++;
        return false;
    }

    // Ждём приём
    timeout = 300;
    while (!mRxComplete && timeout > 0) {
        HAL_Delay(1);
        timeout--;
    }

    if (!mRxComplete) {
        errorCount++;
        HAL_UART_AbortReceive(mHuart);
        return false;
    }

    // Инвалидация кэша перед чтением
    SCB_InvalidateDCache_by_Addr((uint32_t*)mRxBuffer, 9);
    __DMB();

    // Сравниваем первые 8 байт
    for (uint8_t i = 0; i < 8; i++) {
        if (mTxBuffer[i] != mRxBuffer[i]) {
            errorCount++;
            mRxComplete = false;
            return false;
        }
    }

    successCount++;
    mRxComplete = false;
    return true;
}

// === УСТАНОВКА ЧАСТОТЫ ===
bool UartProtocol::setFrequency(float freqMhz)
{
    uint32_t reg1, reg2;
    calculateRegisters(freqMhz, &reg1, &reg2);

    bool result = sendRegisters(reg1, reg2);
    if (result) {
        currentFrequency = freqMhz;
    }
    return result;
}

// === ВКЛЮЧЕНИЕ/ВЫКЛЮЧЕНИЕ RF ВЫХОДА ===
bool UartProtocol::setRfOutput(bool enable)
{
    uint32_t reg1, reg2;
    calculateRegisters(currentFrequency, &reg1, &reg2);

    if (enable) {
        reg2 |= (1 << 5);   // RF ON
    } else {
        reg2 &= ~(1 << 5);  // RF OFF
    }

    bool result = sendRegisters(reg1, reg2);
    if (result) {
        rfEnabled = enable;
    }
    return result;
}

// === РАСЧЁТ РЕГИСТРОВ MAX2870 ===
void UartProtocol::calculateRegisters(float freqMhz, uint32_t* reg1, uint32_t* reg2)
{
    // Параметры MAX2870
    const float F_PFD = 25.0;
    uint8_t divider = 1;
    uint32_t intValue = 0;
    uint16_t fracValue = 0;

    // Выбор делителя
    if (freqMhz >= 35.0 && freqMhz < 69.0) {
        divider = 64;
    } else if (freqMhz >= 69.0 && freqMhz < 138.0) {
        divider = 32;
    } else if (freqMhz >= 138.0 && freqMhz < 275.0) {
        divider = 16;
    } else if (freqMhz >= 275.0 && freqMhz < 550.0) {
        divider = 8;
    } else if (freqMhz >= 550.0 && freqMhz < 1100.0) {
        divider = 4;
    } else if (freqMhz >= 1100.0 && freqMhz < 2200.0) {
        divider = 2;
    } else if (freqMhz >= 2200.0 && freqMhz <= 4400.0) {
        divider = 1;
    }

    // Расчёт VCO частоты
    float f_vco = freqMhz * divider;
    float valueN = f_vco / F_PFD;

    intValue = (uint32_t)valueN;
    fracValue = (uint16_t)((valueN - intValue) * 1000.0 + 0.5);

    // Формируем reg1
    *reg1 = (intValue << 16) |
            (fracValue << 4) |
            (0b010 << 1);

    // Формируем reg2
    *reg2 = (txRem2.modValue << 20) |
            (txRem2.chargePampCurrent << 16) |
            (txRem2.outPower << 14) |
            (txRem2.ldPinMod << 12) |
            (divider << 9) |
            (txRem2.attenuator1 << 8) |
            (txRem2.attenuator2 << 7) |
            (txRem2.attenuator3 << 6) |
            (txRem2.reserved << 0);
}
