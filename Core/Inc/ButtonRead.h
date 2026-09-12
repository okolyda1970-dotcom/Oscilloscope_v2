#ifndef SRC_BUTTONREAD_H_
#define SRC_BUTTONREAD_H_

class ButtonRead
{
public:
    float stepFreqw = 1.0;
    bool buttonPlus = false;
    bool buttonMines = false;
    bool buttonSet = false;
    bool buttonSelect = false;
    bool statusWork = false;
    bool statusScan = false;

    void readButton();
    void setFrqBtn();
};

#endif /* SRC_BUTTONREAD_H_ */
