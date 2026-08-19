#include "Registers.hpp"

// === ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ===
remote1 txRem1 = {0};
remote2 txRem2 = {0};

// === УСТАНОВКА ЗНАЧЕНИЙ ПО УМОЛЧАНИЮ ===
void setDefaultRegisters(void)
{
    // === НАСТРОЙКА НА 100 МГц ===
    txRem1.intValue = 128;       // INT = 128
    txRem1.frac_Value = 0;       // FRAC = 0
    txRem1.muxOut = 0b010;       // Lock Detect
    txRem1.reserved = 0;

    txRem2.modValue = 1000;      // MOD = 1000
    txRem2.chargePampCurrent = 0b0100;  // 4 мА
    txRem2.outPower = 0b01;      // -4 dBm
    txRem2.rfDivider = 0b101;    // 32 (0b101 = 32)
    txRem2.ldPinMod = 0b11;      // Lock Detect
    txRem2.attenuator1 = 0;      // Выключить аттенюаторы
    txRem2.attenuator2 = 0;
    txRem2.attenuator3 = 0;
    txRem2.reserved = 0;
}

// === ФОРМИРОВАНИЕ РЕГИСТРОВ ===
uint32_t remoteInit1(void)
{
    return (txRem1.reserved << 0) | (txRem1.muxOut << 1) |
           (txRem1.frac_Value << 4) | (txRem1.intValue << 16);
}

uint32_t remoteInit2(void)
{
    return (txRem2.reserved << 0) | (txRem2.attenuator3 << 6) |
           (txRem2.attenuator2 << 7) | (txRem2.attenuator1 << 8) |
           (txRem2.rfDivider << 9) | (txRem2.ldPinMod << 12) |
           (txRem2.outPower << 14) | (txRem2.chargePampCurrent << 16) |
           (txRem2.modValue << 20);
}
