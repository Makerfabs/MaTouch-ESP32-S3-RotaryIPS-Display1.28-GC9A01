#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <ui.h>
#include "touch.h"
#include <Adafruit_NeoPixel.h>

#define TFT_BLK 7
#define TFT_RES 11

#define TFT_CS 15
#define TFT_MOSI 13
#define TFT_MISO 12
#define TFT_SCLK 14
#define TFT_DC 21

#define TOUCH_INT 40
#define TOUCH_SDA 38
#define TOUCH_SCL 39
#define TOUCH_RST 16

#define BUTTON_PIN 6
#define ENCODER_CLK 9 // CLK
#define ENCODER_DT 10 // DT

#define WS2812_PIN 17
#define NUMPIXELS 20

/*Change to your screen resolution*/
static const uint16_t screenWidth = 240;
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 10];

Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true); // Constructor
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES, 0 /* rotation */, true /* IPS */);
Adafruit_NeoPixel pixels(NUMPIXELS, WS2812_PIN, NEO_GRB + NEO_KHZ800);

int counter = 0;
int State;
int old_State;
int move_flag = 0;

uint32_t color_status = 0;
uint32_t my_color[8];

int rainbow_mode = 0;
int blink_mode = 0;

int bright_level = 255;
int bright_dir = -1;

void setup()
{
    USBSerial.begin(115200); /* prepare for possible serial debug */

    pin_init();
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    gfx->begin();

    my_color[0] = pixels.Color(255, 0, 0);
    my_color[1] = pixels.Color(0, 255, 0);
    my_color[2] = pixels.Color(0, 0, 255);
    my_color[3] = pixels.Color(255, 255, 0);
    my_color[4] = pixels.Color(0, 255, 255);
    my_color[5] = pixels.Color(255, 0, 255);
    my_color[6] = pixels.Color(128, 0, 128);
    my_color[7] = pixels.Color(0, 128, 0);

    delay(200);

    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * screenHeight / 10);

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    /*Change the following line to your display resolution*/
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /*Initialize the (dummy) input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    // encoder
    static lv_indev_drv_t indev_drv2;
    lv_indev_drv_init(&indev_drv2);
    indev_drv2.type = LV_INDEV_TYPE_ENCODER;
    indev_drv2.read_cb = my_encoder_read;
    lv_indev_t *encoder_indev = lv_indev_drv_register(&indev_drv2);

    ui_init();

    lv_group_t *g = lv_group_create();
    lv_group_add_obj(g, ui_Colorwheel1);
    lv_indev_set_group(encoder_indev, g);

    USBSerial.println("Setup done");

    color_status = lv_color_to32(lv_colorwheel_get_rgb(ui_Colorwheel1));

    pixels.begin();
    pixels.setBrightness(255);
    pixels.clear();
    pixels.show();

    xTaskCreatePinnedToCore(Task_TFT, "Task_TFT", 20480, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(Task_main, "Task_main", 40960, NULL, 3, NULL, 1);
}

void Task_TFT(void *pvParameters)
{
    while (1)
    {
        lv_timer_handler();
        vTaskDelay(10);
    }
}

void Task_main(void *pvParameters)
{
    while (1)
    {
        if (lv_obj_has_state(ui_Switch1, LV_STATE_CHECKED))
        {
            USBSerial.println("Switch is ON\n");
            rainbow_mode = 1;
        }
        else
        {
            USBSerial.println("Switch is OFF\n");
            rainbow_mode = 0;
        }

        if (rainbow_mode == 0)
        {
            color_status = lv_color_to32(lv_colorwheel_get_rgb(ui_Colorwheel1));

            // Fast Blink
            if (blink_mode == 1)
            {
                lv_label_set_text(ui_Label2, "Blink Slow");
                set_next_brightness();
                fresh_led();
                vTaskDelay(80);
            }

            // Slow Blink
            if (blink_mode == 2)
            {
                lv_label_set_text(ui_Label2, "No Blink");
                set_next_brightness();
                fresh_led();
                vTaskDelay(300);
            }

            // No Blink
            if (blink_mode == 0)
            {
                lv_label_set_text(ui_Label2, "Blink Fast");
                pixels.setBrightness(200);
                fresh_led();
                vTaskDelay(500);
            }
        }
        else if (rainbow_mode == 1)
        {

            random_led();
            vTaskDelay(1000);
        }

        vTaskDelay(50);
    }
}

void loop()
{
}

//---------------------------------------------

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
}

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
    gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

    lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    int touchX = 0, touchY = 0;

    if (read_touch(&touchX, &touchY) == 1)
    {
        data->state = LV_INDEV_STATE_PR;

        data->point.x = (uint16_t)touchX;
        data->point.y = (uint16_t)touchY;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}

void my_encoder_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    if (counter > 0)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        data->key = LV_KEY_RIGHT;
        counter--;
        move_flag = 1;
    }

    else if (counter < 0)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        data->key = LV_KEY_LEFT;
        counter++;
        move_flag = 1;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

//---------------------------------------------
void fresh_led()
{
    for (int i = 0; i < NUMPIXELS - 4; i++)
    {
        pixels.setPixelColor(i, color_status);
    }
    pixels.show();
}

void random_led()
{
    pixels.setBrightness(200);
    for (int i = 0; i < NUMPIXELS - 4; i++)
    {
        pixels.setPixelColor(i, my_color[random(0, 7)]);
    }

    pixels.show(); // Send the updated pixel colors to the hardware.
}

void set_next_brightness()
{
    bright_level += bright_dir * 25;
    if (bright_level < 0)

    {
        bright_level = 0;
        bright_dir = 1;
    }

    if (bright_level > 255)

    {
        bright_level = 255;
        bright_dir = -1;
    }
    pixels.setBrightness(bright_level);
}