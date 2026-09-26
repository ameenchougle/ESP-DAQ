#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mlx90614_brake.h"

static const char *TAG = "main";


#define BRAKE_TEMP_MIN_C    (0.0f)      
#define BRAKE_TEMP_MAX_C    (500.0f)    


#define PERFORM_ONE_TIME_PROVISIONING (0)


void vTelemetryTask(void *pvParameters)
{
    (void)pvParameters;
    MLX90614_PWM_Reading_t tempReading;

    for (;;)
    {
        
        if (xQueueReceive(MLX90614_Brake_TempQueue, &tempReading, portMAX_DELAY) == pdTRUE)
        {
            if (tempReading.valid)
            {
                ESP_LOGI(TAG, "Brake Temperature: %.2f degC", tempReading.celsius);
            }
            else
            {
                ESP_LOGW(TAG, "Invalid reading received (noise or pulse fault)");
            }
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Brake Temperature Monitoring System...");

#if PERFORM_ONE_TIME_PROVISIONING
    ESP_LOGW(TAG, "Running SMBus EEPROM provisioning phase...");
    esp_err_t prov_err = MLX90614_Brake_Provision(true, BRAKE_TEMP_MIN_C, BRAKE_TEMP_MAX_C);
    if (prov_err == ESP_OK)
    {
        ESP_LOGI(TAG, "Provisioning succeeded! POWER CYCLE THE SENSOR NOW.");
    }
    else
    {
        ESP_LOGE(TAG, "Provisioning failed: %s", esp_err_to_name(prov_err));
    }

    while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
#else


    ESP_ERROR_CHECK(MLX90614_Brake_Init(10));


    if (MLX90614_Brake_StartTask(BRAKE_TEMP_MIN_C, BRAKE_TEMP_MAX_C, 5, 4096) != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to start MLX90614 decode task!");
        return;
    }

   
    xTaskCreate(vTelemetryTask, "telemetry_task", 4096, NULL, 4, NULL);

    ESP_LOGI(TAG, "System running. Waiting for PWM sensor data...");
#endif
}