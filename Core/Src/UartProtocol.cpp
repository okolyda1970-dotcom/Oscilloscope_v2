#include "UartProtocol.hpp"

UartProtocol* UartProtocol::mInstance = nullptr;
// Инициализация статических флагов
volatile bool UartProtocol::flagTx = false;
volatile bool UartProtocol::flagRx = false;

UartProtocol::UartProtocol(UART_HandleTypeDef* huart)
    : mHuart(huart), mTxComplete(true)
{
    mInstance = this;
}


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
        HAL_UART_Transmit_IT(mHuart, mTxBuffer, 9);
    }
}

void UartProtocol::txCompleteCallback(UART_HandleTypeDef* huart)
{
    if (mInstance != nullptr && mInstance->mHuart == huart) {
        mInstance->mTxComplete = true;
        flagTx = false;

        // === ЖДЁМ 10 МС (ПОКА ЗОНД ПОДГОТОВИТ ОТВЕТ) ===
 //       HAL_Delay(10);

        // === ЗАПУСКАЕМ ПРИЁМ ===
/*        if (!flagRx) {
            flagRx = true;
            HAL_UART_Receive_IT(mInstance->mHuart, mInstance->mRxBuffer, 9);
        }*/
    }
}

void UartProtocol::rxCompleteCallback(UART_HandleTypeDef* huart)
{
    if (mInstance != nullptr && mInstance->mHuart == huart) {
        mInstance->mRxComplete = true;
        flagRx = false;
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_15);

        mInstance->resetReceiver();
    }
}
bool UartProtocol::isRxComplete() const {
    return mRxComplete;
}

void UartProtocol::clearRxFlag() {
    mRxComplete = false;
}



void UartProtocol::resetReceiver()
{
    HAL_UART_AbortReceive(mHuart);                    // стоп, если что-то шло
    __HAL_UART_CLEAR_FLAG(mHuart, UART_FLAG_PE | UART_FLAG_FE |
                                    UART_FLAG_NE | UART_FLAG_ORE);
    volatile uint32_t dummy = mHuart->Instance->RDR;  // выкинуть застрявший байт
    (void)dummy;
    HAL_UART_Receive_IT(mHuart, mRxBuffer, 9);        // окно с чистого листа
}



// armRx теперь — через него (заменить старую реализацию):
void UartProtocol::armRx()
{
    resetReceiver();
}

void UartProtocol::errorCallback(UART_HandleTypeDef* huart)
{
    if (mInstance != nullptr && mInstance->mHuart == huart) {
        mInstance->mRxComplete = false;
        // Если HAL остановил приём — взводим заново.
        // Если приём ещё жив — HAL вернёт BUSY, и всё продолжится.
        mInstance->resetReceiver();
    }
}
