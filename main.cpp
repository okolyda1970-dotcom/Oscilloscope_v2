/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "AdcDma.hpp"
#include "Display.hpp"
#include "UartProtocol.hpp"
#include <cstdio>
#include <string.h>
#include "Registers.hpp"
#include "ButtonRead.h"
#include "st7735.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ADC_BUFFER_SIZE 4096  // Было 320
#define PI 3.14159265358979323846
// === ГЕНЕРАЦ�?Я С�?НУСО�?ДЫ ===
#define SIN_LENGTH 160



#define SPECTRUM_POINTS 160          // Ровно под ширину экрана
#define DISPLAY_WIDTH 160

uint16_t spectrum[SPECTRUM_POINTS];
float spectrumFreq[SPECTRUM_POINTS];
uint16_t floorLevel[SPECTRUM_POINTS];
uint16_t baseLine = 64;
uint8_t scanMode = 0;
uint8_t scanRunning = 0;
uint16_t scanIndex = 0;
/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;
DMA_HandleTypeDef hdma_adc1;
DMA_HandleTypeDef hdma_adc2;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart5;
DMA_HandleTypeDef hdma_uart5_rx;

/* USER CODE BEGIN PV */
// Буфер для данных АЦП
#define LENGHT 320
uint16_t adcRfData[LENGHT];
uint16_t sinhroValue = 0;
uint16_t takt = 0;
uint16_t sinchroFlag = 0;
float span = 0.2;
uint16_t step = 1000;
//extern float span;  // 1.0 — норма, >1 — сжатие, <1 — растяжение
uint16_t triggerLevel = 64;   // Уровень триггера (0-127)
uint16_t triggerIndex = 0;    // Индекс, с которого начинаем рисовать
uint8_t triggerFlag = 0;      // Флаг, что триггер сработал
volatile uint8_t flagAdc = 0;
uint16_t adcBuffer[ADC_BUFFER_SIZE];
UartProtocol uart(&huart5);
float currentFrequency = 100.0;  // Текущая частота в МГц
float targetFrequency = 100.0;   // Целевая частота

extern Display display;
ButtonRead btnRead;
float sineFrequency = 1.0;  // Коэффициент скорости (1 = нормально)
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_ADC2_Init(void);
static void MX_SPI1_Init(void);
static void MX_UART5_Init(void);
/* USER CODE BEGIN PFP */


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// === СОЗДАН�?Е ОБЪЕКТОВ ===
AdcDma adc(&hadc2, &hdma_adc2, adcBuffer, ADC_BUFFER_SIZE);
Display display(&hspi1, GPIOB, GPIO_PIN_8, GPIOB, GPIO_PIN_6, GPIOB, GPIO_PIN_7);


void calculateRegisters(float freqMhz, uint32_t* reg1, uint32_t* reg2)
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

    // === ФОРМИРУЕМ reg1 (КАК remoteInit1) ===
    *reg1 = (intValue << 16) |           // INT
            (fracValue << 4) |           // FRAC
            (0b010 << 1);                // MUXOUT = Lock Detect

    // === ФОРМИРУЕМ reg2 (КАК remoteInit2) ===
    *reg2 = (txRem2.modValue << 20) |          // MOD
            (txRem2.chargePampCurrent << 16) | // Charge Pump
            (txRem2.outPower << 14) |          // Output Power
            (txRem2.ldPinMod << 12) |          // LD Pin Mode
            (divider << 9) |                   // RF Divider (из расчёта)
            (txRem2.attenuator1 << 8) |        // Attenuator 1
            (txRem2.attenuator2 << 7) |        // Attenuator 2
            (txRem2.attenuator3 << 6) |        // Attenuator 3
            (txRem2.reserved << 0);            // Reserved

    char str[64];
/*    sprintf(str, "int=%lu frac=%lu", intValue, fracValue);
    ST7735_WriteString(0, 0, str, Font_7x10, ST7735_BLACK, ST7735_WHITE);*/
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART5)
    {

        // Уведомляем класс UartProtocol о завершении передачи
        UartProtocol::txCompleteCallback(huart);
    }
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART5)
    {
        // Уведомляем класс UartProtocol о завершении передачи
        UartProtocol::rxCompleteCallback(huart);
    }
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART5)
    {
        // Обработка ошибок UART (опционально)
        __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_PE | UART_FLAG_FE | UART_FLAG_NE | UART_FLAG_ORE);
    }
}

/* USER CODE BEGIN 0 */
/* USER CODE BEGIN 0 */
void synchronizeSignal(const uint16_t* data, uint16_t length)
{
    for (uint16_t i = 1; i < length; i++) {
        // �?щем пересечение уровня снизу вверх
        if (data[i-1] < triggerLevel && data[i] >= triggerLevel) {
            triggerIndex = i;
            triggerFlag = 1;
            return;
        }
    }
    // Если не нашли — рисуем с начала
    triggerIndex = 0;
    triggerFlag = 0;
}
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == ADC2) {
        HAL_ADC_Start_DMA(&hadc2, (uint32_t*)adcRfData, LENGHT);  // <-- ПЕРЕЗАПУСК СРАЗУ
        flagAdc = 1;
    }
}

void display_old(void) {

    static uint16_t yBuffer[160];

    // === ВЫЧИСЛЯЕМ СРЕДНЕЕ ===
    uint32_t sum = 0;
    for (uint16_t i = 0; i < LENGHT; i++) {
        sum += adcRfData[i];
    }
    uint16_t avg = sum / LENGHT;

    // === ПОИСК ТРИГГЕРА ===
    triggerFlag = 0;
    for (uint16_t i = 1; i < LENGHT; i++) {
        if (adcRfData[i-1] < triggerLevel && adcRfData[i] >= triggerLevel) {
            triggerIndex = i;
            triggerFlag = 1;
            break;
        }
    }
    if (!triggerFlag) {
        triggerIndex = 0;
    }

    // === ЗАПОЛНЯЕМ БУФЕР ===
    uint16_t idx = triggerIndex;
    uint16_t stepInt = step / 100;  // ДЕЛИМ НА 100
    if (stepInt < 1) stepInt = 1;

    for (uint16_t i = 0; i < 160; i++) {
        uint16_t value = adcRfData[idx];
        int16_t centered = (int16_t)value - (int16_t)avg;
        uint16_t y = (centered + 128) / 2;
        if (y > 127) y = 127;
        y = 127 - y;
        yBuffer[i] = y;
        idx += stepInt;
        if (idx >= LENGHT) idx = 0;
    }

    // === ОЧИЩАЕМ ЭКРАН ===
    ST7735_FillRectangleFast(0, 20, 160, 108, ST7735_WHITE);

    // === РИСУЕМ ТОЧКИ ===
    for (uint16_t i = 0; i < 160; i++) {
        ST7735_DrawPixel(i, yBuffer[i] + 20, ST7735_RED);
    }
}
void updateFloor(uint16_t* data, uint16_t len) {
    // Находим минимум и максимум
    uint16_t minVal = 65535;
    uint16_t maxVal = 0;
    for (uint16_t i = 0; i < len; i++) {
        if (data[i] < minVal) minVal = data[i];
        if (data[i] > maxVal) maxVal = data[i];
    }

    // Динамический диапазон
    uint16_t range = maxVal - minVal;

    // Порог = минимум + 30% от диапазона (фон)
    uint16_t threshold = minVal + (range * 30) / 100;

    // Считаем среднее ТОЛЬКО для точек ниже порога
    uint32_t sum = 0;
    uint16_t count = 0;
    for (uint16_t i = 0; i < len; i++) {
        if (data[i] < threshold) {
            sum += data[i];
            count++;
        }
    }
    uint16_t avgFloor = (count > 0) ? (sum / count) : minVal;

    // Плавно обновляем фон
    for (uint16_t i = 0; i < len; i++) {
        floorLevel[i] = (uint16_t)(floorLevel[i] * 0.9 + avgFloor * 0.1);
    }
}

void calibrateBaseLine(void) {
    // Отключаем сигнал (например, выключаем выход зонда)
    uint32_t reg1, reg2;
    calculateRegisters(1000.0, &reg1, &reg2);
    reg2 &= ~(1 << 5);  // RF OFF
    uart.sendCommand(reg1, reg2);
    HAL_Delay(10);

    // Измеряем шум
    uint32_t sum = 0;
    for (uint16_t j = 0; j < LENGHT; j++) {
        sum += adcRfData[j];
    }
    baseLine = sum / LENGHT;

    // Включаем сигнал обратно
    reg2 |= (1 << 5);  // RF ON
    uart.sendCommand(reg1, reg2);
}

void drawRawSignal(uint16_t index) {
    if (index == 0) {
        ST7735_FillScreenFast(ST7735_WHITE);
    }

    // === РИСУЕМ СЫРОЙ СИГНАЛ (БЕЗ ВЫЧИТАНИЯ) ===
    uint16_t height = (spectrum[index] * 120) / 255;  // Просто масштаб
    if (height > 120) height = 120;
    if (height < 5) height = 5;

    // Инвертируем, чтобы 0 был внизу
    height = 127 - height;

    ST7735_FillRectangle(index, height, 1, 127 - height, ST7735_RED);
}

void scanSpectrumStep(void) {
    if (scanRunning) return;

    if (scanIndex >= SPECTRUM_POINTS) {
        scanRunning = 0;
        scanIndex = 0;
        return;
    }

    scanRunning = 1;

    // === ДИАПАЗОН 992–1008 МГц ===
    float centerFreq = 1000.0;
    float halfRange = 8.0;
    float startFreq = centerFreq - halfRange;
    float stepFreq = (2 * halfRange) / SPECTRUM_POINTS;

    float freq = startFreq + scanIndex * stepFreq;

    // === ОТПРАВКА ЧАСТОТЫ ===
    uint32_t reg1, reg2;
    calculateRegisters(freq, &reg1, &reg2);
    uart.sendCommand(reg1, reg2);
    HAL_Delay(5);

    // === ИЗМЕРЕНИЕ (СЫРЫЕ ДАННЫЕ) ===
    uint32_t sum = 0;
    for (uint16_t j = 0; j < LENGHT; j++) {
        sum += adcRfData[j];
    }
    spectrum[scanIndex] = sum / LENGHT;

    // === РИСУЕМ СЫРОЙ СИГНАЛ ===
    drawRawSignal(scanIndex);

    scanIndex++;
    scanRunning = 0;
}
void initFloor(void) {
    for (uint16_t i = 0; i < SPECTRUM_POINTS; i++) {
        floorLevel[i] = 64;  // Начальное значение
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

/* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
  MX_SPI1_Init();
  MX_UART5_Init();
  /* USER CODE BEGIN 2 */
  display.init();
   display.clear(COLOR_WHITE);

   // === НАСТРОЙКА ЧАСТОТЫ ===
   setDefaultRegisters();
   // === ОТПРАВКА НА ЗОНД ===
   uint32_t reg1 = remoteInit1();
   uint32_t reg2 = remoteInit2();
   uart.sendCommand(reg1, reg2);
   // === ОТПРАВКА ЧАСТОТЫ 1000 МГц НА ЗОНД ===
   float freq = 70.0;

 //  LL_mDelay(200);
   calculateRegisters(freq, &reg1, &reg2);
   uart.sendCommand(reg1, reg2);
   HAL_Delay(20);

   HAL_UART_RegisterCallback(&huart5, HAL_UART_TX_COMPLETE_CB_ID, UartProtocol::txCompleteCallback);
   HAL_UART_RegisterCallback(&huart5, HAL_UART_RX_COMPLETE_CB_ID, UartProtocol::rxCompleteCallback);

/*   // === ЖЁСТКАЯ ОТПРАВКА (без функций) ===
   uint8_t rawCommand[9] = {
       0x00, 0x98, 0x00, 0x04,   // reg1 = 0x00980004
       0x3E, 0x82, 0x78, 0x22,   // reg2 = 0x3E827822
       0x55
   };
   HAL_UART_Transmit(&huart5, rawCommand, 9, 100);*/



/*   HAL_ADC_RegisterCallback(&hadc2, HAL_ADC_CONVERSION_COMPLETE_CB_ID, AdcDma::convCpltCallback);
   adc.startContinuousCapture();*/
   // === ПРЯМОЙ ЗАПУСК АЦП (БЕЗ КЛАССА) ===
   HAL_ADC_Start_DMA(&hadc2, (uint32_t*)adcRfData, LENGHT);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
/*
   initFloor();  // В main() после инициализации дисплея
   calibrateBaseLine();  // Калибровка фона
*/

/*
     reg1 = 0x00980004;   // INT=152, FRAC=0, MUXOUT=010
     reg2 = 0x3E827822;   // Правильный reg2
*/
   while (1) {
	    // === МЕНЯЕМ СОСТОЯНИЕ LD ===
	    if (txRem2.ldPinMod == 0b11) {
	        txRem2.ldPinMod = 0b00;
	    } else {
	        txRem2.ldPinMod = 0b11;
	    }

	    // === ПЕРЕСЧИТЫВАЕМ РЕГИСТРЫ (С НОВЫМ LD) ===
	    calculateRegisters(freq, &reg1, &reg2);

	    // === ОТПРАВКА КОМАНДЫ ===
	    uart.sendCommand(reg1, reg2);
	    HAL_Delay(50);
	    if (huart5.gState == HAL_UART_STATE_READY) {
	        HAL_UART_Receive_IT(&huart5, uart.mRxBuffer, 9);
	    }
	    // === МИГАЕМ СВЕТОДИОДОМ ПРИ ОТПРАВКЕ ===
	    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_15);

	    // === ВЫВОД НА ДИСПЛЕЙ ===
	    char str[128];
	    display.clearArea(0, 0, 160, 80, COLOR_WHITE);

	    sprintf(str, "TX: %02X %02X %02X %02X %02X %02X %02X %02X %02X",
	            uart.mTxBuffer[0], uart.mTxBuffer[1], uart.mTxBuffer[2],
	            uart.mTxBuffer[3], uart.mTxBuffer[4], uart.mTxBuffer[5],
	            uart.mTxBuffer[6], uart.mTxBuffer[7], uart.mTxBuffer[8]);
	    display.drawString(0, 10, str, COLOR_BLACK, COLOR_WHITE);

	    sprintf(str, "RX: %02X %02X %02X %02X %02X %02X %02X %02X %02X",
	            uart.mRxBuffer[0], uart.mRxBuffer[1], uart.mRxBuffer[2],
	            uart.mRxBuffer[3], uart.mRxBuffer[4], uart.mRxBuffer[5],
	            uart.mRxBuffer[6], uart.mRxBuffer[7], uart.mRxBuffer[8]);
	    display.drawString(0, 42, str, COLOR_BLACK, COLOR_WHITE);

	    // === ВЫВОД СОСТОЯНИЯ LD ===
	    sprintf(str, "LD: %s", (txRem2.ldPinMod == 0b11) ? "ON " : "OFF");
	    display.drawString(0, 74, str, COLOR_BLACK, COLOR_WHITE);

	    HAL_Delay(500);

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_CSI;
  RCC_OscInitStruct.CSIState = RCC_CSI_ON;
  RCC_OscInitStruct.CSICalibrationValue = RCC_CSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_CSI;
  RCC_OscInitStruct.PLL.PLLM = 2;
  RCC_OscInitStruct.PLL.PLLN = 480;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_ADC|RCC_PERIPHCLK_SPI1;
  PeriphClkInitStruct.PLL2.PLL2M = 2;
  PeriphClkInitStruct.PLL2.PLL2N = 80;
  PeriphClkInitStruct.PLL2.PLL2P = 2;
  PeriphClkInitStruct.PLL2.PLL2Q = 4;
  PeriphClkInitStruct.PLL2.PLL2R = 2;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_1;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOMEDIUM;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL2;
  PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_8B;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_14;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  sConfig.OffsetSignedSaturation = DISABLE;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief ADC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC2_Init(void)
{

  /* USER CODE BEGIN ADC2_Init 0 */

  /* USER CODE END ADC2_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC2_Init 1 */

  /* USER CODE END ADC2_Init 1 */

  /** Common config
  */
  hadc2.Instance = ADC2;
  hadc2.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV8;
  hadc2.Init.Resolution = ADC_RESOLUTION_8B;
  hadc2.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc2.Init.LowPowerAutoWait = DISABLE;
  hadc2.Init.ContinuousConvMode = ENABLE;
  hadc2.Init.NbrOfConversion = 1;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc2.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_ONESHOT;
  hadc2.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc2.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
  hadc2.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_19;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  sConfig.OffsetSignedSaturation = DISABLE;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC2_Init 2 */

  /* USER CODE END ADC2_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES_TXONLY;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 0x0;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi1.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi1.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi1.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief UART5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART5_Init(void)
{

  /* USER CODE BEGIN UART5_Init 0 */

  /* USER CODE END UART5_Init 0 */

  /* USER CODE BEGIN UART5_Init 1 */

  /* USER CODE END UART5_Init 1 */
  huart5.Instance = UART5;
  huart5.Init.BaudRate = 9600;
  huart5.Init.WordLength = UART_WORDLENGTH_8B;
  huart5.Init.StopBits = UART_STOPBITS_1;
  huart5.Init.Parity = UART_PARITY_EVEN;
  huart5.Init.Mode = UART_MODE_TX_RX;
  huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart5.Init.OverSampling = UART_OVERSAMPLING_16;
  huart5.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart5.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart5.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart5) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart5, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart5, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART5_Init 2 */

  /* USER CODE END UART5_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
  /* DMA1_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
  /* DMA1_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOE);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOC);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOA);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOB);

  /**/
  LL_GPIO_ResetOutputPin(GPIOB, EN_VDD_Pin|LED_1_Pin|A0_DISP_Pin|RESET_DISPL_Pin
                          |CS_DISPL_Pin);

  /**/
  GPIO_InitStruct.Pin = BUTTON_3_Pin|BUTTON_4_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = BUTTON_1_Pin|BUTTON_2_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = EN_VDD_Pin|LED_1_Pin|A0_DISP_Pin|RESET_DISPL_Pin
                          |CS_DISPL_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LL_GPIO_PIN_11|LL_GPIO_PIN_12;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_10;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
