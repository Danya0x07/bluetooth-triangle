#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <MAX30105.h>
#include <avr/sleep.h>

#include "config.h"

// определение режима соединения и подключение библиотеки RemoteXY
#define REMOTEXY_MODE__HARDSERIAL

#include <RemoteXY.h>

// настройки соединения
#define REMOTEXY_SERIAL Serial
#define REMOTEXY_SERIAL_SPEED 57600

// конфигурация интерфейса
#pragma pack(push, 1)
uint8_t RemoteXY_CONF[] = {
    255,4,0,4,0,63,0,10,161,0,
    2,1,71,44,21,11,91,26,31,31,
    79,78,0,79,70,70,0,68,49,3,
    2,94,33,26,36,208,159,209,131,208,
    187,209,140,209,129,0,4,0,11,37,
    7,24,1,26,4,0,37,37,7,24,
    6,26,4,0,24,37,7,24,134,26
};

// структура определяет все переменные и события вашего интерфейса управления
struct {

    // input variables
    uint8_t pulseOximeterSwitch; // =1 если переключатель включен и =0 если отключен
    int8_t redSlider; // =0..100 положение слайдера
    int8_t blueSlider; // =0..100 положение слайдера
    int8_t greenSlider; // =0..100 положение слайдера

    // output variables
    float pulseGraph;

    // other variable
    uint8_t connect_flag;  // =1 if wire connected, else =0

} RemoteXY;
#pragma pack(pop)

MAX30105 pulseOximeter;

void updateStrip(int8_t sliderValue, uint8_t pin)
{
    uint8_t value = map(sliderValue, 0, 100, 0, 0xFF);
    value = constrain(value, 0, 0xFF);
    analogWrite(pin, value);
}

void mainTask(void* unused)
{

    for (;;) {
        updateStrip(RemoteXY.redSlider, RED_STRIP_PIN);
        updateStrip(RemoteXY.greenSlider, GREEN_STRIP_PIN);
        updateStrip(RemoteXY.blueSlider, BLUE_STRIP_PIN);

        RemoteXY_Handler();

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void setup()
{
    RemoteXY_Init();
    while (1) {
        RemoteXY_Handler();
    }

    xTaskCreate(mainTask, "main", 128, NULL, 2, NULL);
}

void loop()  // вызывается планировщиком во время простоя
{
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_mode();
}

