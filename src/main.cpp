#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <timers.h>
#include <MAX30105.h>
#include <heartRate.h>
#include <avr/sleep.h>
#include <avr/power.h>

#define RED_STRIP_PIN   5
#define GREEN_STRIP_PIN   3
#define BLUE_STRIP_PIN   6

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

TaskHandle_t Handle_MeasureHeartRate;
TimerHandle_t Handle_UpdateRemote;

void setStripBrightnessFromSlider(int8_t sliderValue, uint8_t pin)
{
    uint8_t value = map(sliderValue, 0, 100, 0, 0xFF);
    value = constrain(value, 0, 0xFF);
    analogWrite(pin, value);
}

void Task_ProcessInput(void* arg __attribute__((unused)))
{
    bool pulseOximeterSwitchLast = false;
    uint8_t redSliderLast = 0, greenSliderLast = 0, blueSliderLast = 0;

    for (;;) {
        if (RemoteXY.pulseOximeterSwitch != pulseOximeterSwitchLast) {
            if (RemoteXY.pulseOximeterSwitch) {
                pulseOximeter.wakeUp();
                vTaskResume(Handle_MeasureHeartRate);
            } else {
                vTaskSuspend(Handle_MeasureHeartRate);
                pulseOximeter.shutDown();
            }
            pulseOximeterSwitchLast = RemoteXY.pulseOximeterSwitch;
        }

        if (RemoteXY.redSlider != redSliderLast) {
            if (pulseOximeterSwitchLast == false)
                setStripBrightnessFromSlider(RemoteXY.redSlider, RED_STRIP_PIN);
            redSliderLast = RemoteXY.redSlider;
        }
        if (RemoteXY.greenSlider != greenSliderLast) {
            setStripBrightnessFromSlider(RemoteXY.greenSlider, GREEN_STRIP_PIN);
            greenSliderLast = RemoteXY.greenSlider;
        }
        if (RemoteXY.blueSlider != blueSliderLast) {
            setStripBrightnessFromSlider(RemoteXY.blueSlider, BLUE_STRIP_PIN);
            blueSliderLast = RemoteXY.blueSlider;
        }

        vTaskDelay(pdMS_TO_TICKS(60));
    }
}

void Task_MeasureHeartRate(void* arg __attribute__((unused)))
{

    for (;;) {
        int32_t irValue = pulseOximeter.getIR();

        if (irValue < 50000) {
            RemoteXY.pulseGraph = 0;
        } else {
            RemoteXY.pulseGraph = irValue;
            if (checkForBeat(irValue)) {
                analogWrite(RED_STRIP_PIN, 0xFF);
            } else {
                analogWrite(RED_STRIP_PIN, 0);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(15));
        //taskYIELD();
    }
}

void updateRemote(TimerHandle_t timer __attribute__((unused)))
{
    RemoteXY_Handler();
}

void setup()
{
    power_adc_disable();
    power_spi_disable();

    RemoteXY_Init();

    if (!pulseOximeter.begin(Wire, I2C_SPEED_FAST)) {
        digitalWrite(LED_BUILTIN, 1);
        delay(2000);
        digitalWrite(LED_BUILTIN, 0);
    }
    pulseOximeter.setup(0x1F, 4, 2, 400, 411, 4096);
    pulseOximeter.shutDown();

    xTaskCreate(Task_ProcessInput, "prcinp", 128, NULL, 3, NULL);
    xTaskCreate(Task_MeasureHeartRate, "meashr", 128, NULL, 2,
                &Handle_MeasureHeartRate);

    Handle_UpdateRemote = xTimerCreate("updtmr", pdMS_TO_TICKS(45), pdTRUE,
                                       NULL, updateRemote);
    xTimerStart(Handle_UpdateRemote, 0);
}

void loop()  // вызывается планировщиком во время простоя
{
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_mode();
}