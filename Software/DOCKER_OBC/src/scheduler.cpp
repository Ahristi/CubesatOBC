#include "scheduler.h"

void COMMS_task();
void EPS_task();
void ADCS_task();
void PAYLOAD_task();
void LOGGING_task();
void SYSTEM_task();


Task tasks[] = {
    {COMMS_task,      5, 0, 0, "Comms"},
    {EPS_task,        5, 0, 0, "EPS"},
    {ADCS_task,      20, 0, 0, "ADCS"},
    {PAYLOAD_task, PAYLOAD_TASK_PERIOD_MS, 0, 0, "Payload"},
    {LOGGING_task, 1000, 0, 0, "Logging"},
    {SYSTEM_task,   100, 0, 0, "System"},
};


static constexpr uint8_t NUM_TASKS = sizeof(tasks) / sizeof(tasks[0]);

void Scheduler_init()
{
    uint32_t now = millis();

    for (uint8_t i = 0; i < NUM_TASKS; i++)
    {
        tasks[i].last_run_ms = now;
    }
}

void Scheduler_run() {
    uint32_t now = millis();
    for (auto &task : tasks) {
        if (now - task.last_run_ms >= task.period_ms) {
            task.last_run_ms = now;
            task.function();
            task.duration = millis() -  task.last_run_ms;
        }
    }
}

void Scheduler_debugPrint()
{
    Serial.println();
    Serial.println("========== Schedular DEBUG ==========");
    for (auto &task : tasks) 
    {
        Serial.println(task.name);
        Serial.print(task.duration);
        Serial.println(" ms");
    }
}