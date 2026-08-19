#ifndef DISPLAY_HPP_
#define DISPLAY_HPP_

#include "stm32h7xx_hal.h"
#include <stdint.h>
#include <string.h>

// === ЦВЕТА ===
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F

// === ОРИЕНТАЦИЯ ===
#define ST7735_MADCTL_MY  0x80
#define ST7735_MADCTL_MX  0x40
#define ST7735_MADCTL_MV  0x20
#define ST7735_MADCTL_ML  0x10
#define ST7735_MADCTL_RGB 0x00
#define ST7735_MADCTL_BGR 0x08
#define ST7735_MADCTL_MH  0x04

// === КОМАНДЫ ST7735 ===
#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID   0x04
#define ST7735_RDDST   0x09

#define ST7735_SLPIN   0x10
#define ST7735_SLPOUT  0x11
#define ST7735_PTLON   0x12
#define ST7735_NORON   0x13

#define ST7735_INVOFF  0x20
#define ST7735_INVON   0x21
#define ST7735_GAMSET  0x26
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_RAMRD   0x2E

#define ST7735_PTLAR   0x30
#define ST7735_COLMOD  0x3A
#define ST7735_MADCTL  0x36

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5

#define ST7735_RDID1   0xDA
#define ST7735_RDID2   0xDB
#define ST7735_RDID3   0xDC
#define ST7735_RDID4   0xDD

#define ST7735_PWCTR6  0xFC

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

// === РАЗМЕРЫ ЭКРАНА ===
#define ST7735_WIDTH  160
#define ST7735_HEIGHT 128
#define ST7735_XSTART 0
#define ST7735_YSTART 0
#define ST7735_ROTATION (ST7735_MADCTL_MY | ST7735_MADCTL_MV)

// === КЛАСС DISPLAY ===
class Display {
public:
    Display(SPI_HandleTypeDef* hspi,
            GPIO_TypeDef* csPort, uint16_t csPin,
            GPIO_TypeDef* dcPort, uint16_t dcPin,
            GPIO_TypeDef* rstPort, uint16_t rstPin);

    void init();
    void clear(uint16_t color);
    void clearArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
    void drawPixel(uint16_t x, uint16_t y, uint16_t color);
    void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                  uint16_t color, uint8_t thickness);
    void drawString(uint16_t x, uint16_t y, const char* str,
                    uint16_t color, uint16_t bgColor);
    void drawOscillogram(const uint16_t* data, uint16_t length,
                         uint16_t color, uint8_t thickness);
    void drawSpectrum(const uint16_t* data, uint16_t length, uint16_t color);
    void synchronizeSignal(const uint16_t* data, uint16_t length);
    void setTriggerLevel(uint16_t level);
    void setSpan(float newSpan);

    float getSpan() const;
    void setGain(float newGain);
    float getGain() const;

private:
    uint16_t triggerLevel = 64;
    SPI_HandleTypeDef* mHspi;
    GPIO_TypeDef* mCsPort;
    uint16_t mCsPin;
    GPIO_TypeDef* mDcPort;
    uint16_t mDcPin;
    GPIO_TypeDef* mRstPort;
    uint16_t mRstPin;
    uint16_t mWidth;
    uint16_t mHeight;

    void select();
    void unselect();
    void reset();
    void writeCommand(uint8_t cmd);
    void writeData(uint8_t* data, uint16_t len);
    void setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
    void sendCommandList(const uint8_t* commands);
};

#endif // DISPLAY_HPP_
