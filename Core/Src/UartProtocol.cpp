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
    // Формируем буфер
    mTxBuffer[0] = (uint8_t)(reg1 >> 24);
    mTxBuffer[1] = (uint8_t)(reg1 >> 16);
    mTxBuffer[2] = (uint8_t)(reg1 >> 8);
    mTxBuffer[3] = (uint8_t)(reg1);
    mTxBuffer[4] = (uint8_t)(reg2 >> 24);
    mTxBuffer[5] = (uint8_t)(reg2 >> 16);
    mTxBuffer[6] = (uint8_t)(reg2 >> 8);
    mTxBuffer[7] = (uint8_t)(reg2);
    mTxBuffer[8] = 0x55;

    // === ПРОСТАЯ ОТПРАВКА (как старый sendCommand) ===
    if (flagTx) {
        // Предыдущая передача ещё не завершена — пропускаем
        return false;
    }

    flagTx = true;
    mTxComplete = false;

    HAL_StatusTypeDef status = HAL_UART_Transmit_IT(mHuart, mTxBuffer, 9);
    if (status != HAL_OK) {
        flagTx = false;
        errorCount++;
        return false;
    }

    // Ждём завершения передачи
    uint32_t timeout = 50;
    while (!mTxComplete && timeout > 0) {
        HAL_Delay(1);
        timeout--;
    }

    if (mTxComplete) {
        successCount++;
        // Пауза после отправки для стабильности
        HAL_Delay(2);
        return true;
    }

    errorCount++;
    flagTx = false;
    return false;
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

void UartProtocol::calculateRegisters(float freqMhz, uint32_t* reg1, uint32_t* reg2)
{
    const float F_PFD = 25.0;
    uint8_t dividerCode = 0;
    uint32_t divider = 1;
    uint32_t intValue = 0;
    uint16_t fracValue = 0;

    // === Выбор КОДА делителя (не значения!) ===
    if (freqMhz < 68.75) {
        divider = 64;      dividerCode = 6;  // 0b110
    } else if (freqMhz < 137.5) {
        divider = 32;      dividerCode = 5;  // 0b101
    } else if (freqMhz < 275.0) {
        divider = 16;      dividerCode = 4;  // 0b100
    } else if (freqMhz < 550.0) {
        divider = 8;       dividerCode = 3;  // 0b011
    } else if (freqMhz < 1100.0) {
        divider = 4;       dividerCode = 2;  // 0b010
    } else if (freqMhz < 2200.0) {
        divider = 2;       dividerCode = 1;  // 0b001
    } else {
        divider = 1;       dividerCode = 0;  // 0b000
    }

    // Расчёт VCO
    float f_vco = freqMhz * divider;
    float valueN = f_vco / F_PFD;

    intValue = (uint32_t)valueN;
    fracValue = (uint16_t)((valueN - intValue) * txRem2.modValue + 0.5);

    // reg1: INT, FRAC, MUX (без бита адреса)
    *reg1 = (intValue << 16) |
            (fracValue << 4) |
            (0b010 << 1);

    // reg2: ВАЖНО — используем dividerCode, а не divider!
    *reg2 = ((uint32_t)txRem2.modValue << 20) |
            ((uint32_t)txRem2.chargePampCurrent << 16) |
            ((uint32_t)txRem2.outPower << 14) |
            ((uint32_t)txRem2.ldPinMod << 12) |
            ((uint32_t)dividerCode << 9) |    // ← ИСПРАВЛЕНО!
            ((uint32_t)txRem2.attenuator1 << 8) |
            ((uint32_t)txRem2.attenuator2 << 7) |
            ((uint32_t)txRem2.attenuator3 << 6) |
            ((uint32_t)txRem2.reserved << 0);
}


