#include <Arduino_GFX_Library.h>

#define TFT_BLK 7
#define TFT_RES 11

#define TFT_CS 15
#define TFT_MOSI 13
#define TFT_MISO 12
#define TFT_SCLK 14
#define TFT_DC 21

Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true); // Constructor
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES, 0 /* rotation */, true /* IPS */);

//For 2.1"
// #define TFT_BLK 38

// Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
//     1 /* CS */, 46 /* SCK */, 0 /* SDA */,
//     2 /* DE */, 42 /* VSYNC */, 3 /* HSYNC */, 45 /* PCLK */,
//     11 /* R0 */, 15 /* R1 */, 12 /* R2 */, 16 /* R3 */, 21 /* R4 */,
//     39 /* G0/P22 */, 7 /* G1/P23 */, 47 /* G2/P24 */, 8 /* G3/P25 */, 48 /* G4/P26 */, 9 /* G5 */,
//     4 /* B0 */, 41 /* B1 */, 5 /* B2 */, 40 /* B3 */, 6 /* B4 */
// );

// // Uncomment for 2.1" round display
// Arduino_ST7701_RGBPanel *gfx = new Arduino_ST7701_RGBPanel(
//     bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */,
//     false /* IPS */, 480 /* width */, 480 /* height */,
//     st7701_type5_init_operations, sizeof(st7701_type5_init_operations),
//     true /* BGR */,
//     10 /* hsync_front_porch */, 8 /* hsync_pulse_width */, 50 /* hsync_back_porch */,
//     10 /* vsync_front_porch */, 8 /* vsync_pulse_width */, 20 /* vsync_back_porch */);


void setup()
{
    pinMode(TFT_BLK, OUTPUT);
    digitalWrite(TFT_BLK, HIGH);

    Serial.begin(115200);
    Serial.println("FPS Test!");

    gfx->begin();
}

void loop(void)
{

    long runtime = testFillScreen();
    float fps = 25.0 * 1000 / runtime;

    int w = gfx->width();
    int h = gfx->height();

    gfx->setTextSize(4);
    gfx->setTextColor(WHITE, BLACK);

    char c[20];

    gfx->setCursor(10, h / 2);
    gfx->println("FillScreen 25 Times");

    sprintf(c, "Use: %ld ms", runtime);
    gfx->setCursor(20, h / 2 + 20);
    gfx->println(c);

    sprintf(c, "FPS : %.2f", fps);
    gfx->setCursor(40, h / 2 + 40);
    gfx->println(c);

    delay(5000);
}

long testFillScreen()
{
    long start_time = millis();

    for (int i = 0; i < 5; i++)
    {
        gfx->fillScreen(WHITE);
        gfx->fillScreen(RED);
        gfx->fillScreen(GREEN);
        gfx->fillScreen(BLUE);
        gfx->fillScreen(BLACK);
    }

    return millis() - start_time;
}
