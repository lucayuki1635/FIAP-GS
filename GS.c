#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_heap_caps.h"

#define NUM_REDES_SEGURAS 5
#define TAREFA_SCAN_PERIOD_MS 2000
#define WDT_TIMEOUT_S 10

//Lista de redes seguras 
const char *redes_seguras[NUM_REDES_SEGURAS] = {
    "WiFi_Casa",
    "WiFi_Trabalho",
    "WiFi_ESP32",
    "Rede_Segura_1",
    "Rede_Segura_2",
    "Rede_Segura_3",
    "Rede_Segura_4",
};


QueueHandle_t fila_alerta;
SemaphoreHandle_t semaforo_redes;
TaskHandle_t handle_scan = NULL;
TaskHandle_t handle_alerta = NULL;
TaskHandle_t handle_wdt_monitor = NULL;

//Validação se a rede é segura, baseada na lista redes_seguras
int eh_rede_segura(const char *ssid) {
    int segura = 0;
    if (xSemaphoreTake(semaforo_redes, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (int i = 0; i < NUM_REDES_SEGURAS; i++) {
            if (strcmp(ssid, redes_seguras[i]) == 0) {
                segura = 1;
                break;
            }
        }
        xSemaphoreGive(semaforo_redes);
    } else {
        printf("Erro: não foi possível acessar lista de redes seguras\n");
    }
    return segura;
}

//Caso seja detectado uma rede não autorizada, será feito o log.
void registrar_alerta(const char *ssid) {
    printf("ALERTA! Rede não autorizada detectada: %s\n", ssid);
}

//Randomização para selecionar rede segura ou não segura
void gerar_rede_simulada(char *ssid) {
    int segura = rand() % 2;
    if (segura) {
        int idx = rand() % NUM_REDES_SEGURAS;
        strcpy(ssid, redes_seguras[idx]);
    } else {
        sprintf(ssid, "Rede_Insegura_%d", rand() % 100);
    }
}

//Scannea a rede conectada
void tarefa_scan(void *pvParameters) {
    esp_task_wdt_add(NULL); 
    char ssid_atual[32];

    while (1) {
        esp_task_wdt_reset();
        gerar_rede_simulada(ssid_atual);
        printf("Rede atual acessada: %s\n", ssid_atual);

        if (!eh_rede_segura(ssid_atual)) {
            if (xQueueSend(fila_alerta, &ssid_atual, pdMS_TO_TICKS(100)) != pdPASS) {
                printf("Erro: fila de alerta cheia! Tentativa perdida\n");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(TAREFA_SCAN_PERIOD_MS));
    }
}


//Registra o alerta, chamando a função de registrar_alerta()
void tarefa_alerta(void *pvParameters) {
    esp_task_wdt_add(NULL); 
    char ssid_recebido[32];

    while (1) {
        esp_task_wdt_reset();
        if (xQueueReceive(fila_alerta, &ssid_recebido, pdMS_TO_TICKS(1000))) {
            registrar_alerta(ssid_recebido);
        }
    }
}


void tarefa_wdt_monitor(void *pvParameters) {
    while (1) {
        size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
        printf("Monitoramento: memória livre = %u bytes\n", (unsigned int)free_heap);

        // Recuperação
        if (eTaskGetState(handle_scan) == eSuspended) {
            printf("Tarefa Scan travada! Reiniciando...\n");
            vTaskDelete(handle_scan);
            xTaskCreate(tarefa_scan, "Tarefa_Scan", 4096, NULL, 5, &handle_scan);
        }
        if (eTaskGetState(handle_alerta) == eSuspended) {
            printf("Tarefa Alerta travada! Reiniciando...\n");
            vTaskDelete(handle_alerta);
            xTaskCreate(tarefa_alerta, "Tarefa_Alerta", 4096, NULL, 4, &handle_alerta);
        }

        vTaskDelay(pdMS_TO_TICKS(WDT_TIMEOUT_S * 500));
    }
}

void app_main(void) {
    srand((unsigned int)time(NULL));

    semaforo_redes = xSemaphoreCreateMutex();
    fila_alerta = xQueueCreate(5, sizeof(char[32]));

    if (!semaforo_redes || !fila_alerta) {
        printf("Erro: falha ao criar semáforo ou fila\n");
        return;
    }


    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = WDT_TIMEOUT_S * 1000,
        .idle_core_mask = 0xFFFFFFFF,
        .trigger_panic = true
    };
    esp_task_wdt_init(&wdt_config);


    xTaskCreate(tarefa_scan, "Tarefa_Scan", 4096, NULL, 5, &handle_scan);
    xTaskCreate(tarefa_alerta, "Tarefa_Alerta", 4096, NULL, 4, &handle_alerta);
    xTaskCreate(tarefa_wdt_monitor, "Tarefa_WDT", 2048, NULL, 3, &handle_wdt_monitor);
}
