#ifndef UARTPROTOCOL_HPP_
#define UARTPROTOCOL_HPP_

#include "stm32h7xx_hal.h"
#include "Registers.hpp"

class UartProtocol {
public:
    // === КОНСТРУКТОР И ДЕСТРУКТОР ===
    UartProtocol(UART_HandleTypeDef* huart);
    ~UartProtocol();

    // === ОРИГИНАЛЬНЫЕ МЕТОДЫ ОТПРАВКИ ===
    void sendCommand(uint32_t reg1, uint32_t reg2);
    void armRx();
    void clearRxFlag();
    void resetReceiver();

    // === НОВЫЕ МЕТОДЫ УПРАВЛЕНИЯ ЗОНДОМ ===
    bool sendRegisters(uint32_t reg1, uint32_t reg2);
    bool setFrequency(float freqMhz);
    bool setRfOutput(bool enable);

    // === ГЕТТЕРЫ ===
    uint32_t getSuccessCount() const { return successCount; }
    uint32_t getErrorCount() const { return errorCount; }
    float getCurrentFrequency() const { return currentFrequency; }
    bool isRfEnabled() const { return rfEnabled; }

    // === БУФЕРЫ (публичные для диагностики) ===
    uint8_t mTxBuffer[9];
    uint8_t mRxBuffer[9];
    volatile bool mTxComplete;
    volatile bool mRxComplete;
    UART_HandleTypeDef* mHuart;

    // === СТАТИЧЕСКИЕ КОЛБЭКИ ===
    static void txCompleteCallback(UART_HandleTypeDef* huart);
    static void rxCompleteCallback(UART_HandleTypeDef* huart);
    static void errorCallback(UART_HandleTypeDef* huart);

private:
    // === ДАННЫЕ ДЛЯ УПРАВЛЕНИЯ ЗОНДОМ ===
    float currentFrequency;
    bool rfEnabled;
    uint32_t successCount;
    uint32_t errorCount;

    // === РАСЧЁТ РЕГИСТРОВ ===
    void calculateRegisters(float freqMhz, uint32_t* reg1, uint32_t* reg2);

    // === СТАТИЧЕСКИЙ УКАЗАТЕЛЬ НА ЭКЗЕМПЛЯР ===
    static UartProtocol* mInstance;

    // === ФЛАГИ СОСТОЯНИЯ ===
    static volatile bool flagTx;
    static volatile bool flagRx;
};

#endif // UARTPROTOCOL_HPP_
