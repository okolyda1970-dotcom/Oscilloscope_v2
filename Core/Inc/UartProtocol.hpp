#ifndef UARTPROTOCOL_HPP_
#define UARTPROTOCOL_HPP_

#include "stm32h7xx_hal.h"
#include <stdint.h>

class UartProtocol {
public:
    // Конструктор: передаём хендл UART и DMA
    UartProtocol(UART_HandleTypeDef* huart);

    // Отправка данных на зонд (32-битные регистры)
    void sendCommand(uint32_t reg1, uint32_t reg2);

    // Проверка, завершена ли отправка
    bool isTransmitComplete() const;

    // Колбэк завершения передачи (вызывается из HAL)
    static void txCompleteCallback(UART_HandleTypeDef* huart);
    static void rxCompleteCallback(UART_HandleTypeDef* huart);
    void armRx();                                   // взвести приём
    static void errorCallback(UART_HandleTypeDef* huart);  // перезапуск после ошибки
    void resetReceiver();   // чистое перевзведение окна приёма
    bool isRxComplete() const;
    void clearRxFlag();
    uint8_t mTxBuffer[9];  // 8 байт данных + 1 байт CRC
    volatile bool mTxComplete = true;
    volatile bool mRxComplete = false;
    uint8_t mRxBuffer[9];  // 8 байт данных + 1 байт CRC
private:
    UART_HandleTypeDef* mHuart;

    static volatile bool flagTx;
    static volatile bool flagRx;


    // Расчет контрольной суммы (CRC8)
    uint8_t calculateCRC(const uint8_t* data, uint8_t len);

    // Статический указатель для колбэка
    static UartProtocol* mInstance;
};

#endif // UARTPROTOCOL_HPP_
