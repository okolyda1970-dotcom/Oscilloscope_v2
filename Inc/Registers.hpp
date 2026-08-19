#ifndef REGISTERS_HPP_
#define REGISTERS_HPP_

#include <stdint.h>

// === СТРУКТУРЫ ДЛЯ ОТПРАВКИ НА ЗОНД ===
typedef struct {
    uint8_t reserved : 1;
    uint8_t muxOut : 3;
    uint16_t frac_Value : 12;
    uint16_t intValue : 16;
} remote1;

typedef struct {
    uint16_t reserved : 6;
    uint8_t attenuator3 : 1;
    uint8_t attenuator2 : 1;
    uint8_t attenuator1 : 1;
    uint8_t rfDivider : 3;
    uint8_t ldPinMod : 2;
    uint8_t outPower : 2;
    uint8_t chargePampCurrent : 4;
    uint16_t modValue : 12;
} remote2;

// === ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ===
extern remote1 txRem1;
extern remote2 txRem2;

// === ФУНКЦИИ ===
uint32_t remoteInit1(void);
uint32_t remoteInit2(void);
void setDefaultRegisters(void);

#endif // REGISTERS_HPP_
