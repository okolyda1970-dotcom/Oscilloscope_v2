#include "Display.hpp"
#include "fonts.h"
#include "st7735.h"
extern  float span;  // Было 1.0 — теперь очень маленькое (растянуто)  // Оставляем static, но добавляем методы доступа
 static float gain = 1.0;  // Коэффициент усиления по вертикали

 void Display::setGain(float newGain) {
     if (newGain < 0.1) newGain = 0.1;
     if (newGain > 5.0) newGain = 5.0;
     gain = newGain;
 }

 float Display::getGain() const {
     return gain;
 }

void Display::setSpan(float newSpan) {
    if (newSpan < 0.5) newSpan = 0.5;
    span = newSpan;
}

float Display::getSpan() const {
    return span;
}
// === КОНСТРУКТОР ===
Display::Display(SPI_HandleTypeDef* hspi,
                 GPIO_TypeDef* csPort, uint16_t csPin,
                 GPIO_TypeDef* dcPort, uint16_t dcPin,
                 GPIO_TypeDef* rstPort, uint16_t rstPin)
    : mHspi(hspi), mCsPort(csPort), mCsPin(csPin),
      mDcPort(dcPort), mDcPin(dcPin),
      mRstPort(rstPort), mRstPin(rstPin),
      mWidth(ST7735_WIDTH), mHeight(ST7735_HEIGHT)
{
}

// === ИНИЦИАЛИЗАЦИЯ ===
void Display::init()
{
	ST7735_Init();
}

// === ОЧИСТКА ===
void Display::clear(uint16_t color)
{
    select();
    setAddressWindow(0, 0, mWidth - 1, mHeight - 1);

    uint8_t data[2] = { (uint8_t)(color >> 8), (uint8_t)(color & 0xFF) };
    HAL_GPIO_WritePin(mDcPort, mDcPin, GPIO_PIN_SET);

    for (uint16_t i = 0; i < mWidth * mHeight; i++) {
        HAL_SPI_Transmit(mHspi, data, 2, HAL_MAX_DELAY);
    }

    unselect();
}

// === РИСОВАНИЕ ПИКСЕЛЯ ===
void Display::drawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= mWidth || y >= mHeight) return;

    select();
    // === МЕНЯЕМ МЕСТАМИ X И Y ===
    setAddressWindow(x, y, x, y);  // Было setAddressWindow(x, y, x, y)
    uint8_t data[2] = { (uint8_t)(color >> 8), (uint8_t)(color & 0xFF) };
    writeData(data, 2);
    unselect();
}

// === ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ ===
void Display::select() {
    HAL_GPIO_WritePin(mCsPort, mCsPin, GPIO_PIN_RESET);
}

void Display::unselect() {
    HAL_GPIO_WritePin(mCsPort, mCsPin, GPIO_PIN_SET);
}

void Display::reset() {
    HAL_GPIO_WritePin(mRstPort, mRstPin, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(mRstPort, mRstPin, GPIO_PIN_SET);
    HAL_Delay(150);
}

void Display::writeCommand(uint8_t cmd) {
    HAL_GPIO_WritePin(mDcPort, mDcPin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(mHspi, &cmd, 1, HAL_MAX_DELAY);
}

void Display::writeData(uint8_t* data, uint16_t len) {
    HAL_GPIO_WritePin(mDcPort, mDcPin, GPIO_PIN_SET);
    HAL_SPI_Transmit(mHspi, data, len, HAL_MAX_DELAY);
}

void Display::setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    // === ДОБАВЛЯЕМ СМЕЩЕНИЯ ===
    x0 += ST7735_XSTART;
    x1 += ST7735_XSTART;
    y0 += ST7735_YSTART;
    y1 += ST7735_YSTART;

    writeCommand(0x2A); // CASET
    uint8_t data[] = { (uint8_t)((x0 >> 8) & 0xFF), (uint8_t)(x0 & 0xFF),
                       (uint8_t)((x1 >> 8) & 0xFF), (uint8_t)(x1 & 0xFF) };
    writeData(data, 4);

    writeCommand(0x2B); // RASET
    data[0] = (y0 >> 8) & 0xFF;
    data[1] = y0 & 0xFF;
    data[2] = (y1 >> 8) & 0xFF;
    data[3] = y1 & 0xFF;
    writeData(data, 4);

    writeCommand(0x2C); // RAMWR
}

// === ТРИГГЕР ===
static uint16_t triggerIndex = 0;
static uint8_t triggerFlag = 0;


void Display::setTriggerLevel(uint16_t level) {
    triggerLevel = level;
}

void Display::synchronizeSignal(const uint16_t* data, uint16_t length)
{
    for (uint16_t i = 1; i < length; i++) {
        if (data[i-1] < triggerLevel && data[i] >= triggerLevel) {
            triggerIndex = i;
            triggerFlag = 1;
            return;
        }
    }
    triggerIndex = 0;
    triggerFlag = 0;
}

// === ОТРИСОВКА ОСЦИЛЛОГРАММЫ ===

void Display::drawOscillogram(const uint16_t* data, uint16_t length,
                              uint16_t color, uint8_t thickness)
{
    if (data == nullptr || length == 0) return;

    clearArea(0, 20, mWidth, mHeight - 20, COLOR_WHITE);

    // === ВЫЧИСЛЯЕМ СРЕДНЕЕ ===
    uint32_t sum = 0;
    for (uint16_t i = 0; i < length; i++) {
        sum += data[i];
    }
    uint16_t avg = sum / length;

    // === РИСУЕМ ===
    select();
    setAddressWindow(0, 20, mWidth - 1, mHeight - 1);

    uint16_t step = (uint16_t)span;
    if (step < 1) step = 1;

    uint16_t pixelBuffer[mWidth];
    for (uint16_t i = 0; i < mWidth; i++) {
        pixelBuffer[i] = COLOR_WHITE;
    }

    for (uint16_t i = 0; i < mWidth && i < length; i += step) {
        uint16_t value = data[i];

        // === ЦЕНТРОВКА ===
        int16_t centered = (int16_t)value - (int16_t)avg;
        uint16_t y = (centered + 128) / 2;
        if (y >= mHeight) y = mHeight - 1;
        y = mHeight - 1 - y;

        for (uint8_t t = 0; t < thickness; t++) {
            if (y + t < mHeight) {
                pixelBuffer[i] = color;
            }
        }
    }

    // Отправляем буфер
    HAL_GPIO_WritePin(mDcPort, mDcPin, GPIO_PIN_SET);
    for (uint16_t i = 0; i < mWidth; i++) {
        uint8_t dataBuffer[2] = { (uint8_t)(pixelBuffer[i] >> 8),
                                  (uint8_t)(pixelBuffer[i] & 0xFF) };
        HAL_SPI_Transmit(mHspi, dataBuffer, 2, HAL_MAX_DELAY);
    }

    unselect();
}

void Display::clearArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    select();
    setAddressWindow(x, y, x + w - 1, y + h - 1);

    uint8_t data[2] = { (uint8_t)(color >> 8), (uint8_t)(color & 0xFF) };
    HAL_GPIO_WritePin(mDcPort, mDcPin, GPIO_PIN_SET);

    for (uint16_t i = 0; i < w * h; i++) {
        HAL_SPI_Transmit(mHspi, data, 2, HAL_MAX_DELAY);
    }

    unselect();
}

void Display::drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                       uint16_t color, uint8_t thickness)
{
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        // Рисуем жирную точку (несколько пикселей)
        for (uint8_t t = 0; t < thickness; t++) {
            if (y1 + t < mHeight) {
                drawPixel(x1, y1 + t, color);
            }
        }

        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void Display::drawString(uint16_t x, uint16_t y, const char* str,
                         uint16_t color, uint16_t bgColor)
{
    if (str == nullptr) return;

    // Используем шрифт 7x10 (самый маленький)

    select();

    while (*str)
    {
        if (x + Font_7x10.width >= mWidth)
        {
            x = 0;
            y += Font_7x10.height;
            if (y + Font_7x10.height >= mHeight) break;
        }

        uint8_t ch = *str - 32;
        if (ch > 95) ch = 0;

        setAddressWindow(x, y, x + Font_7x10.width - 1, y + Font_7x10.height - 1);

        for (uint8_t i = 0; i < Font_7x10.height; i++)
        {
            uint16_t line = Font_7x10.data[ch * Font_7x10.height + i];
            for (uint8_t j = 0; j < Font_7x10.width; j++)
            {
                uint8_t data[2];
                if (line & (0x8000 >> j))
                {
                    data[0] = color >> 8;
                    data[1] = color & 0xFF;
                }
                else
                {
                    data[0] = bgColor >> 8;
                    data[1] = bgColor & 0xFF;
                }
                writeData(data, 2);
            }
        }

        x += Font_7x10.width;
        str++;
    }

    unselect();
}
