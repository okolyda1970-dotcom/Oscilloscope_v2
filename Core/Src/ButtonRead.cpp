#include "ButtonRead.h"
#include "main.h"

void ButtonRead::readButton()
{
    // В новом проекте кнопки называются BUTTON_1, BUTTON_2, BUTTON_3, BUTTON_4
    if (!LL_GPIO_IsInputPinSet(BUTTON_1_GPIO_Port, BUTTON_1_Pin))
    {
        buttonPlus = true;
    }
    if (!LL_GPIO_IsInputPinSet(BUTTON_2_GPIO_Port, BUTTON_2_Pin))
    {
        buttonMines = true;
    }
    if (!LL_GPIO_IsInputPinSet(BUTTON_3_GPIO_Port, BUTTON_3_Pin))
    {
        buttonSet = true;
        statusWork = true;
    }
    if (!LL_GPIO_IsInputPinSet(BUTTON_4_GPIO_Port, BUTTON_4_Pin))
    {
        buttonSelect = true;
    }
//    setFrqBtn();
}

void ButtonRead::setFrqBtn()
{
    if (buttonSelect)
    {
        buttonSelect = false;
        if (stepFreqw != 100.0)
        {
            stepFreqw = 100.0;
        }
        else
        {
            stepFreqw = 1.0;
        }
    }
}
