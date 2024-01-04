#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <ui.h>
#include "touch.h"
#include <WiFi.h>

#define SSID "Makerfabs"
#define PWD "20160704"

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

/*Change to your screen resolution*/
static const uint16_t screenWidth = 240;
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 10];

Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true); // Constructor
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES, 0 /* rotation */, true /* IPS */);

int counter = 0;
int State;
int old_State;
int move_flag = 0;

// NTP
const char *ntpServer = "120.25.108.11";
int net_flag = 0;
int bg_flag = 0;

void setup()
{
    USBSerial.begin(115200); /* prepare for possible serial debug */

    pin_init();
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    gfx->begin();

    wifi_init();

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

    // lv_group_t *g = lv_group_create();
    // lv_group_add_obj(g, ui_Arc1);
    // lv_indev_set_group(encoder_indev, g);

    USBSerial.println("Setup done");

    xTaskCreatePinnedToCore(Task_TFT, "Task_TFT", 40960, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(Task_my, "Task_my", 20000, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(Task_button, "Task_button", 1000, NULL, 1, NULL, 1);
}

void Task_TFT(void *pvParameters)
{
    while (1)
    {
        lv_timer_handler();
        vTaskDelay(10);
    }
}

long task_runtime_1 = 0;
int bg_index = 0;
void Task_my(void *pvParameters)
{
    while (1)
    {

        if (net_flag == 1)
            if ((millis() - task_runtime_1) > 1000)
            {
                display_time();
                task_runtime_1 = millis();
            }

        if (bg_flag == 1)
        {
            bg_flag = 0;
            set_bg_img(bg_index++);
        }

        vTaskDelay(100);
    }
}

void Task_button(void *pvParameters)
{
    while (1)
    {
        if (digitalRead(BUTTON_PIN) == 0)
        {
            vTaskDelay(40);
            if (digitalRead(BUTTON_PIN) == 0)
            {
                while (digitalRead(BUTTON_PIN) == 0)
                {
                    vTaskDelay(200);
                }
                bg_flag = 1;
            }
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
        data->key = LV_KEY_LEFT;
        counter--;
        move_flag = 1;
    }

    else if (counter < 0)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        data->key = LV_KEY_RIGHT;
        counter++;
        move_flag = 1;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    // if (enc_pressed())
    //     data->state = LV_INDEV_STATE_PRESSED;
    // else
    //     data->state = LV_INDEV_STATE_RELEASED;
}

void wifi_init()
{
    WiFi.begin(SSID, PWD);

    int connect_count = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        vTaskDelay(500);
        USBSerial.print(".");
        connect_count++;
    }

    USBSerial.println("Wifi connect");
    configTime((const long)(8 * 3600), 0, ntpServer);

    net_flag = 1;
}

void display_time()
{
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo))
    {
        USBSerial.println("Failed to obtain time");
        return;
    }
    else
    {
        int year = timeinfo.tm_year + 1900;
        int month = timeinfo.tm_mon + 1;
        int day = timeinfo.tm_mday;
        int hour = timeinfo.tm_hour;
        int min = timeinfo.tm_min;
        int sec = timeinfo.tm_sec;

        int sec_angle = 3600 * sec / 60;
        int min_angle = 3600 * min / 60 + 60 * sec / 60;
        int hour_angle = 3600 * (hour % 12) / 12 + 300 * min / 60;

        lv_img_set_angle(ui_Image1, hour_angle);
        lv_img_set_angle(ui_Image2, min_angle);
        lv_img_set_angle(ui_Image3, sec_angle);
    }
}

void set_bg_img(int index)
{
    index = index % 4;
    switch (index)
    {
    case 0:
        lv_obj_set_style_bg_img_src(ui_Screen1, &ui_img_bg1_png, LV_PART_MAIN | LV_STATE_DEFAULT);
        break;
    case 1:
        lv_obj_set_style_bg_img_src(ui_Screen1, &ui_img_bg2_png, LV_PART_MAIN | LV_STATE_DEFAULT);
        break;

    case 2:
        lv_obj_set_style_bg_img_src(ui_Screen1, &ui_img_bg3_png, LV_PART_MAIN | LV_STATE_DEFAULT);
        break;

    case 3:
        lv_obj_set_style_bg_img_src(ui_Screen1, &ui_img_bg4_png, LV_PART_MAIN | LV_STATE_DEFAULT);
        break;

    default:
        break;
    }
}