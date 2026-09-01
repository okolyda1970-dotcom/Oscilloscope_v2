#ifndef POTREADER_HPP_
#define POTREADER_HPP_

#include "AdcDma.hpp"
#include <stdint.h>

class PotReader {
public:
    // Идентификаторы потенциометров
    enum PotID { OFFSET = 0, CENTER = 1, POT_COUNT };

    // Конструктор
    PotReader(AdcDma* adc);

    // === ОБНОВЛЕНИЕ (вызывать каждый цикл) ===
    void update();

    // === ПОЛУЧЕНИЕ ЗНАЧЕНИЙ ===
    uint16_t getRawValue(PotID pot) const;       // Сырое значение (0-4095)
    float getOffsetPercent() const;              // 0.0 - 1.0 (для PWM)
    float getCenterPercent() const;              // 0.0 - 1.0 (для центра)

    // === НАСТРОЙКА ФИЛЬТРА ===
    void setFilterStrength(uint8_t strength);    // 0 = без фильтра, 8 = сильное сглаживание

private:
    AdcDma* mAdc;

    // Последние отфильтрованные значения
    float mFiltered[POT_COUNT];

    // Сила фильтра (0-8), чем больше — тем сильнее сглаживание
    uint8_t mFilterStrength;

    // Флаг инициализации (первое значение)
    bool mInitialized;
};

#endif // POTREADER_HPP_
