// 使用 1.3.1  版本的库 GFX_Library_for_Arduino 在文件夹： C:\Users\maker\Documents\Arduino\libraries\GFX_Library_for_Arduino
// 使用 2.0.0  版本的库 SPI 在文件夹： C:\Users\maker\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.11\libraries\SPI
// 使用 2.0.0  版本的库 SD 在文件夹： C:\Users\maker\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.11\libraries\SD
// 使用 2.0.0  版本的库 FS 在文件夹： C:\Users\maker\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.11\libraries\FS
// 使用 2.0.0  版本的库 Wire 在文件夹： C:\Users\maker\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.11\libraries\Wire
// 使用 1.2.8  版本的库 JPEGDEC 在文件夹： C:\Users\maker\Documents\Arduino\libraries\JPEGDEC

#include <Arduino_GFX_Library.h>
#include <SD.h>
#include <FS.h>
#include "touch.h"
#include "JpegFunc.h"

#define TFT_BLK 7
#define TFT_RES 11

#define TFT_CS 15
#define TFT_MOSI 13
#define TFT_MISO 12
#define TFT_SCLK 14
#define TFT_DC 21

#define SD_SCK 42
#define SD_MISO 41
#define SD_MOSI 2
#define SD_CS 1

#define JPEG_FILENAME "/logo_240240.jpg"

#define TOUCH_INT 40
#define TOUCH_SDA 38
#define TOUCH_SCL 39
#define TOUCH_RST 16

#define BUTTON_PIN 6
#define ENCODER_CLK 9 // CLK
#define ENCODER_DT 10 // DT

#define COLOR_NUM 5
int ColorArray[COLOR_NUM] = {WHITE, BLUE, GREEN, RED, YELLOW};

int counter = 0;
int State;
int old_State;
int move_flag = 0;
int button_flag = 0;
int flesh_flag = 1;

int x = 0, y = 0;

Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true); // Constructor
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES, 0 /* rotation */, true /* IPS */);
// CST816S touch(TOUCH_SDA, TOUCH_SCL, TOUCH_RST, TOUCH_INT); // sda, scl, rst, irq

void setup(void)
{
    USBSerial.begin(115200);

    pin_init();
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI);

    delay(1000);
    USBSerial.println("start");

    gfx->begin();
    if (!SD.begin(SD_CS))
    {
        USBSerial.println(F("ERROR: File System Mount Failed!"));

        gfx->setTextSize(4);
        gfx->setCursor(10, 120);
        gfx->println(F("SD ERROR"));
        while (1)
            delay(3000);
    }
    else
    {
        unsigned long start = millis();
        jpegDraw(JPEG_FILENAME, jpegDrawCallback, true, 0, 0, 240, 240);
        USBSerial.printf("Time used: %lu\n", millis() - start);
        delay(2000);
    }

    gfx->fillScreen(RED);
    delay(1000);
    gfx->fillScreen(GREEN);
    delay(1000);
    gfx->fillScreen(BLUE);
    delay(1000);
    gfx->fillScreen(BLACK);
    delay(1000);
    gfx->fillScreen(WHITE);
}

void loop()
{

    if (read_touch(&x, &y) == 1)
    {
        USBSerial.print("Touch ");
        USBSerial.print(x);
        USBSerial.print("\t");
        USBSerial.println(y);

        flesh_flag = 1;
    }
    if (digitalRead(BUTTON_PIN) == 0)
    {
        if (button_flag != 1)
        {
            button_flag = 1;
            flesh_flag = 1;
        }

        USBSerial.println("Button Press");
    }
    else
    {
        if (button_flag != 0)
        {
            button_flag = 0;
            flesh_flag = 1;
        }
    }

    if (move_flag == 1)
    {
        USBSerial.print("Position: ");
        USBSerial.println(counter);
        move_flag = 0;
        flesh_flag = 1;
    }
    if (flesh_flag == 1)
        page_1();
}

void pin_init()
{
    pinMode(TFT_BLK, OUTPUT);
    digitalWrite(TFT_BLK, HIGH);

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    old_State = digitalRead(ENCODER_CLK);

    attachInterrupt(ENCODER_CLK, encoder_irq, CHANGE);
}

void encoder_irq()
{
    State = digitalRead(ENCODER_CLK);
    if (State != old_State)
    {
        if (digitalRead(ENCODER_DT) == State)
        {
            counter++;
        }
        else
        {
            counter--;
        }
    }
    old_State = State; // the first position was changed
    move_flag = 1;
}

void page_1()
{
    gfx->fillRect(135, 95, 50, 85, YELLOW);
    gfx->fillCircle(x, y, 5, RED);

    gfx->setTextSize(2);
    gfx->setTextColor(BLACK);
    gfx->setCursor(60, 50);
    gfx->println(F("Makerfabs"));

    gfx->setTextSize(1);
    gfx->setCursor(40, 80);
    gfx->println(F("1.28 inch TFT with Touch "));

    char temp[30];

    gfx->setTextSize(2);
    gfx->setCursor(30, 100);
    sprintf(temp, "Encoder: %4d", counter);
    gfx->println(temp);

    gfx->setTextSize(2);
    gfx->setCursor(30, 120);
    sprintf(temp, "BUTTON:  %4d", button_flag);
    gfx->println(temp);

    gfx->setTextSize(2);
    gfx->setCursor(30, 140);
    sprintf(temp, "Touch X: %4d", x);
    gfx->println(temp);

    gfx->setTextSize(2);
    gfx->setCursor(30, 160);
    sprintf(temp, "Touch Y: %4d", y);
    gfx->println(temp);

    flesh_flag = 0;
}

// pixel drawing callback
static int jpegDrawCallback(JPEGDRAW *pDraw)
{
    // USBSerial.printf("Draw pos = %d,%d. size = %d x %d\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
    gfx->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
    return 1;
}
