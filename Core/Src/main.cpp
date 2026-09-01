/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body — Простой осциллограф
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
#include "Registers.hpp"
#include "st7735.h"
#include <cstdio>
#include <string.h>
#include "ButtonManager.hpp"
#include "Scanner.hpp"
#include "Oscilloscope.hpp"
#include "PotReader.hpp"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PI 3.14159265358979323846
#define ADC_BUFFER_SIZE 160
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;
DMA_HandleTypeDef hdma_adc1;
DMA_HandleTypeDef hdma_adc2;

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart5;
DMA_HandleTypeDef hdma_uart5_rx;

/* USER CODE BEGIN PV */

// === БУФЕРЫ АЦП ===
uint16_t adcBuffer[ADC_BUFFER_SIZE];   // Буфер детектора сигнала
uint16_t potBuffer[2];                  // Буфер потенциометров (A0, A1)

// === ОБЪЕКТЫ АЦП (создаются ДО остальных объектов!) ===
AdcDma adcDetector(&hadc2, &hdma_adc2, adcBuffer, ADC_BUFFER_SIZE);
AdcDma adcPots(&hadc1, &hdma_adc1, potBuffer, 2);
// === ОБЪЕКТЫ ===
UartProtocol uart(&huart5);
Display display(&hspi1, GPIOB, GPIO_PIN_8, GPIOB, GPIO_PIN_6, GPIOB, GPIO_PIN_7);

// === МЕНЕДЖЕР КНОПОК ===
ButtonManager buttons(BUTTON_1_GPIO_Port, BUTTON_1_Pin,   // BTN1 (PC0)
                      BUTTON_2_GPIO_Port, BUTTON_2_Pin,   // BTN2 (PC1)
                      BUTTON_3_GPIO_Port, BUTTON_3_Pin,   // BTN3 (PE1)
                      BUTTON_4_GPIO_Port, BUTTON_4_Pin);  // BTN4 (PE2)
Scanner scanner(&uart, &hadc2, adcBuffer, ADC_BUFFER_SIZE);
Oscilloscope oscilloscope(&adcDetector);
PotReader potReader(&adcPots);
// === ФЛАГ�? ===
volatile uint8_t flagAdc = 0;
uint8_t currentMode = 0;
float fixedFrequency = 900.0f;
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
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
void setOffset(uint8_t value);

void setOffset(uint8_t value);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// === КОЛБЭК�? UART ===
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART5) {
        HAL_GPIO_TogglePin(LED_1_GPIO_Port, LED_1_Pin);  // ← Мигание при передаче
        UartProtocol::txCompleteCallback(huart);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART5) {
        HAL_GPIO_TogglePin(LED_1_GPIO_Port, LED_1_Pin);  // ← Мигание при приёме
        UartProtocol::rxCompleteCallback(huart);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART5) {
        __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_PE | UART_FLAG_FE |
                              UART_FLAG_NE | UART_FLAG_ORE);
        UartProtocol::errorCallback(huart);
    }
}

// === КОЛБЭК АЦП ===
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
	// ПРАВ�?ЛЬНО:
	AdcDma::convCpltCallback(hadc);
}

// === УСТАНОВКА УРОВНЯ СМЕЩЕН�?Я ОУ ===
void setOffset(uint8_t value) {
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, value);
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
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
    HAL_Delay(500);
    display.init();
    display.clear(COLOR_WHITE);

    // === ЗАПУСК Ш�?М ДЛЯ СМЕЩЕН�?Я ОУ ===
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    setOffset(128);

    // === КАЛ�?БРОВКА АЦП ===
    HAL_ADCEx_Calibration_Start(&hadc2, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
    // === ЗАПУСК НЕПРЕРЫВНОГО ЧТЕН�?Я ПОТЕНЦ�?ОМЕТРОВ ===
    adcPots.startContinuousCapture();
    potReader.setFilterStrength(6);  // Среднее сглаживание
    HAL_Delay(10);

    // === НАСТРОЙКА РЕГ�?СТРОВ ===
    setDefaultRegisters();

    // === ДАЁМ ЗОНДУ ВРЕМЯ ПРОСНУТЬСЯ ===
    display.drawString(0, 0, "Probe waking up...", COLOR_BLACK, COLOR_WHITE);
    HAL_Delay(1000);

    // === УСТАНОВКА НАЧАЛЬНОЙ ЧАСТОТЫ ===
    uart.setFrequency(900.0);
    uart.setRfOutput(true);

    // === ГОТОВО ===
    display.clear(COLOR_WHITE);
    display.drawString(0, 0, "INIT COMPLETE", COLOR_GREEN, COLOR_WHITE);
    display.drawString(0, 10, "F: 900MHz", COLOR_BLACK, COLOR_WHITE);
    display.drawString(0, 20, "Mode: Oscilloscope", COLOR_BLACK, COLOR_WHITE);
    HAL_Delay(1000);
  /* USER CODE END 2 */

  /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    char str[64];
    static uint32_t lastTextUpdate = 0;

    while (1) {
        // === ОБНОВЛЕНИЕ КНОПОК ===
        buttons.update();

        uint32_t now = HAL_GetTick();

        // === ОБНОВЛЕНИЕ ПОТЕНЦИОМЕТРОВ ===
        potReader.update();

        // === ПРИМЕНЕНИЕ СМЕЩЕНИЯ A0 (работает во всех режимах) ===
        static uint32_t lastOffsetUpdate = 0;
        static uint8_t lastOffsetValue = 128;

        if (now - lastOffsetUpdate >= 50) {
            lastOffsetUpdate = now;
            uint8_t offsetValue = (uint8_t)(potReader.getOffsetPercent() * 255.0f);

            if (offsetValue != lastOffsetValue) {
                lastOffsetValue = offsetValue;
                setOffset(offsetValue);
            }
        }

        // ================================================================
        // === РЕЖИМ 2: НАСТРОЙКА ЧАСТОТЫ ===
        // ================================================================
        if (currentMode == 2) {
            // Принудительно перезапускаем АЦП1 каждые 500 мс для надёжности
            static uint32_t lastAdcRestartSetup = 0;
            if (now - lastAdcRestartSetup >= 500) {
                lastAdcRestartSetup = now;
                adcPots.startContinuousCapture();
            }

            // Вычисляем текущую частоту с потенциометра
            float setupFreq = 50.0f + potReader.getCenterPercent() * 3950.0f;

            // Обновляем отображение частоты раз в 100 мс
            static uint32_t lastSetupUpdate = 0;
            if (now - lastSetupUpdate >= 100) {
                lastSetupUpdate = now;

                // Читаем сырые значения для диагностики
                uint16_t a1Raw = potReader.getRawValue(PotReader::CENTER);

                display.clearArea(0, 40, 160, 40, COLOR_WHITE);

                // Частота (тёмно-зелёный)
                sprintf(str, "F: %.1f MHz", setupFreq);
                display.drawString(0, 40, str, COLOR_DARKGREEN, COLOR_WHITE);

                // Значение потенциометра (для отладки)
                sprintf(str, "A1 raw: %d", a1Raw);
                display.drawString(0, 60, str, COLOR_BLUE, COLOR_WHITE);

                // Процент
                sprintf(str, "Percent: %.1f%%", potReader.getCenterPercent() * 100);
                display.drawString(0, 70, str, COLOR_BLACK, COLOR_WHITE);
            }

            // КНОПКА BTN3: ФИКСАЦИЯ ЧАСТОТЫ
            ButtonManager::ButtonEvent evt3 = buttons.getEvent(ButtonManager::BTN3);
            if (evt3 == ButtonManager::PRESSED) {
                fixedFrequency = setupFreq;
                uart.setFrequency(fixedFrequency);

                display.clear(COLOR_WHITE);
                sprintf(str, "SET: %.1f MHz", fixedFrequency);
                display.drawString(0, 0, str, COLOR_DARKGREEN, COLOR_WHITE);
                display.drawString(0, 20, "Returning to oscilloscope", COLOR_BLACK, COLOR_WHITE);
                HAL_Delay(1000);

                currentMode = 0;
                continue;
            }

            // КНОПКА BTN1: ОТМЕНА
            ButtonManager::ButtonEvent evt1 = buttons.getEvent(ButtonManager::BTN1);
            if (evt1 == ButtonManager::PRESSED) {
                display.clear(COLOR_WHITE);
                display.drawString(0, 0, "CANCELLED", COLOR_RED, COLOR_WHITE);
                HAL_Delay(500);
                currentMode = 0;
                continue;
            }

            HAL_Delay(20);
            continue;
        }

        // ================================================================
        // === ОБРАБОТКА КНОПОК (для режимов 0 и 1) ===
        // ================================================================

        // === КНОПКА BTN3: ВХОД В РЕЖИМ НАСТРОЙКИ ЧАСТОТЫ ===
        ButtonManager::ButtonEvent evt3 = buttons.getEvent(ButtonManager::BTN3);
        if (evt3 == ButtonManager::PRESSED) {
            currentMode = 2;
            display.clear(COLOR_WHITE);
            display.drawString(0, 0, "FREQUENCY SETUP", COLOR_BLUE, COLOR_WHITE);
            display.drawString(0, 10, "Turn pot A1 to set", COLOR_BLACK, COLOR_WHITE);
            display.drawString(0, 20, "BTN3=SET  BTN1=CANCEL", COLOR_BLACK, COLOR_WHITE);
            HAL_Delay(500);
            continue;
        }

        // === КНОПКА BTN1: ПЕРЕКЛЮЧЕНИЕ РЕЖИМА (Осциллограф <-> Сканер) ===
        ButtonManager::ButtonEvent evt1 = buttons.getEvent(ButtonManager::BTN1);
        if (evt1 == ButtonManager::PRESSED) {
            currentMode = !currentMode;  // 0 <-> 1
            display.clear(COLOR_WHITE);

            if (currentMode == 0) {
                // === РЕЖИМ ОСЦИЛЛОГРАФА ===
                display.drawString(0, 0, "MODE: OSCILLOSCOPE", COLOR_BLACK, COLOR_WHITE);
                sprintf(str, "F: %.1f MHz", fixedFrequency);
                display.drawString(0, 10, str, COLOR_BLACK, COLOR_WHITE);
                uart.setFrequency(fixedFrequency);
            } else {
                // === РЕЖИМ СКАНЕРА ===
                display.drawString(0, 0, "MODE: SCANNER", COLOR_BLACK, COLOR_WHITE);
                scanner.setSpan(320.0);
                scanner.setStep(0.5);
                scanner.setCenter(fixedFrequency);
                scanner.setSettleTime(30);
                scanner.start();
            }
            HAL_Delay(500);
            continue;
        }

        // ================================================================
        // === РЕЖИМ 0: ОСЦИЛЛОГРАФ ===
        // ================================================================
        if (currentMode == 0) {
            // Захват данных
            oscilloscope.capture();

            // Отрисовка осциллограммы
            display.drawOscillogramFast(oscilloscope.getBuffer(),
                                         oscilloscope.getBufferSize(),
                                         COLOR_RED, COLOR_WHITE);

            // === ТЕКСТ ОБНОВЛЯЕМ РАЗ В 500 МС ===
            if (now - lastTextUpdate >= 500) {
                lastTextUpdate = now;

                display.clearArea(0, 0, 160, 50, COLOR_WHITE);

                sprintf(str, "AVG:%d AMP:%d", oscilloscope.getAverage(), oscilloscope.getAmplitude());
                display.drawString(0, 0, str, COLOR_BLACK, COLOR_WHITE);

                sprintf(str, "MIN:%d MAX:%d", oscilloscope.getMin(), oscilloscope.getMax());
                display.drawString(0, 10, str, COLOR_BLACK, COLOR_WHITE);

                sprintf(str, "F:%.1fMHz", fixedFrequency);
                display.drawString(0, 20, str, COLOR_BLACK, COLOR_WHITE);

                sprintf(str, "A0:%d A1:%d",
                        potReader.getRawValue(PotReader::OFFSET),
                        potReader.getRawValue(PotReader::CENTER));
                display.drawString(0, 30, str, COLOR_BLUE, COLOR_WHITE);

                display.drawString(0, 40, "OSC MODE  BTN3=FREQ", COLOR_GREEN, COLOR_WHITE);
            }
        }
        // ================================================================
        // === РЕЖИМ 1: СКАНЕР ===
        // ================================================================
        else {
            // Обновляем сканер (обрабатывает одну точку за вызов)
            scanner.update();

            // === КНОПКА BTN2: ПАУЗА СКАНЕРА ===
            ButtonManager::ButtonEvent evt2 = buttons.getEvent(ButtonManager::BTN2);
            if (evt2 == ButtonManager::PRESSED) {
                if (scanner.isRunning() && !scanner.isPaused()) {
                    scanner.pause();
                    display.clearArea(0, 0, 160, 22, COLOR_WHITE);
                    display.drawString(0, 0, "PAUSED: SET CENTER", COLOR_BLACK, COLOR_WHITE);
                } else if (scanner.isPaused()) {
                    scanner.resume();
                    display.clearArea(0, 0, 160, 22, COLOR_WHITE);
                    display.drawString(0, 0, "RESUMING...", COLOR_BLACK, COLOR_WHITE);
                }
            }

            // === КНОПКА BTN4: НАЙТИ ПИК ===
            ButtonManager::ButtonEvent evt4 = buttons.getEvent(ButtonManager::BTN4);
            if (evt4 == ButtonManager::PRESSED && scanner.isFinished()) {
                float peakFreq = scanner.getPeakFrequency();
                scanner.setCenter(peakFreq);
                scanner.start();
            }

            if (scanner.isRunning()) {
                // === СКАНИРОВАНИЕ В ПРОЦЕССЕ ===
                if (now - lastTextUpdate >= 100) {
                    lastTextUpdate = now;

                    display.clearArea(0, 0, 160, 22, COLOR_WHITE);
                    sprintf(str, "SCAN %.0f-%.0fMHz", scanner.getStartFrequency(), scanner.getEndFrequency());
                    display.drawString(0, 0, str, COLOR_BLACK, COLOR_WHITE);

                    sprintf(str, "Progress: %d%%", scanner.getProgress());
                    display.drawString(0, 10, str, COLOR_BLACK, COLOR_WHITE);
                }
            }
            else if (scanner.isFinished()) {
                // === СКАНИРОВАНИЕ ЗАВЕРШЕНО ===
                if (now - lastTextUpdate >= 500) {
                    lastTextUpdate = now;

                    display.clearArea(0, 0, 160, 22, COLOR_WHITE);
                    sprintf(str, "MAX:%d @ %.1fMHz", scanner.getMaxLevel(), scanner.getPeakFrequency());
                    display.drawString(0, 0, str, COLOR_BLACK, COLOR_WHITE);

                    sprintf(str, "SPAN:%.0f STEP:%.1f", scanner.getEndFrequency() - scanner.getStartFrequency(), 0.5);
                    display.drawString(0, 10, str, COLOR_BLACK, COLOR_WHITE);
                }

                // Отрисовка графика спектра
                display.drawScanGraph(scanner.getResults(), scanner.getResultsSize(), scanner.getMaxLevel());
            }
        }

        HAL_Delay(20);
    }
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
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
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.NbrOfConversion = 2;
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
  sConfig.Channel = ADC_CHANNEL_16;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_8CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_15;
  sConfig.Rank = ADC_REGULAR_RANK_2;
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
  hadc2.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV2;
  hadc2.Init.Resolution = ADC_RESOLUTION_8B;
  hadc2.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc2.Init.LowPowerAutoWait = DISABLE;
  hadc2.Init.ContinuousConvMode = DISABLE;
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
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
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
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 255;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

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
  /* DMA1_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);

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
  GPIO_InitStruct.Pin = BUTTON_4_Pin|BUTTON_3_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = BUTTON_2_Pin|BUTTON_1_Pin;
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
