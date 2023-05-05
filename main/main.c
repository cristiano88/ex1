#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define ESP_INTR_FLAG_DEFAULT 0

#define LED_1 GPIO_NUM_12
#define LED_2 GPIO_NUM_14
#define LED_3 GPIO_NUM_27
#define LED_4 GPIO_NUM_26
#define BC GPIO_NUM_18
#define KEY_TURBO GPIO_NUM_34
#define KEY_POWER GPIO_NUM_15
#define KEY_UP GPIO_NUM_13
#define KEY_DOWN GPIO_NUM_35
static QueueHandle_t keys_evt_queue = NULL;

static void IRAM_ATTR keysIsrHandle(void *pvArg)
{
    uint32_t key = (uint32_t)pvArg;
    xQueueSendFromISR(keys_evt_queue, &key, NULL);
}

void vTaskHandleKeys(void *pvArg)
{
    uint32_t key;
    int counter = 0;

    for (;;)
    {
        if (xQueueReceive(keys_evt_queue, &key, portMAX_DELAY))
        {

            // de-bouncing
            if (!gpio_get_level(key))
            {
                gpio_isr_handler_remove(key);
                counter++;
                printf("os botoes GPIO[%" PRIu32 "] foram acionandos %d vezes.\n", key, counter);
                while (!gpio_get_level(key))
                {
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                }
            }

            vTaskDelay(50 / portTICK_PERIOD_MS);
            gpio_isr_handler_add(key, keysIsrHandle, (void *)key);
        }
    }
}

void app_main(void)
{
    gpio_config_t s_io_conf = {};

    s_io_conf.intr_type = GPIO_INTR_DISABLE;
    s_io_conf.mode = GPIO_MODE_OUTPUT;
    s_io_conf.pin_bit_mask = ((1ULL << BC) | (1ULL << LED_1) | (1ULL << LED_2)) | (1ULL << LED_3) | (1ULL << LED_4);
    s_io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    s_io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    gpio_config(&s_io_conf);

    s_io_conf.intr_type = GPIO_INTR_NEGEDGE;
    s_io_conf.pin_bit_mask = ((1ULL << KEY_TURBO) | (1ULL << KEY_POWER) | (1ULL << KEY_UP) | (1ULL << KEY_DOWN));
    s_io_conf.mode = GPIO_MODE_INPUT;

    gpio_config(&s_io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    gpio_isr_handler_add(KEY_TURBO, keysIsrHandle, (void *)KEY_TURBO);
    gpio_isr_handler_add(KEY_POWER, keysIsrHandle, (void *)KEY_POWER);
    gpio_isr_handler_add(KEY_UP, keysIsrHandle, (void *)KEY_UP);
    gpio_isr_handler_add(KEY_DOWN, keysIsrHandle, (void *)KEY_DOWN);

    gpio_set_level(BC, 0);

    keys_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    xTaskCreate(vTaskHandleKeys, "vTaskHandleKeys", 4096, NULL, 1, NULL);

    for (int i = 0; i < 9; i++)
    {
        gpio_set_level(LED_1, (i & 1) >> 0);
        gpio_set_level(LED_2, (i & 2) >> 1);
        gpio_set_level(LED_3, (i & 4) >> 2);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
