#include "scheduler.h"

void COMMS_task();
void EPS_task();
void ADCS_task();
void PAYLOAD_task();
void LOGGING_task();
void SYSTEM_task();


Task tasks[] = {
    {COMMS_task,    200, 0},
    {EPS_task,        5, 0},
    {ADCS_task,     200, 0},
    {PAYLOAD_task, 1000, 0},
    {LOGGING_task, 1000, 0},
    {SYSTEM_task,   100, 0},
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
        }
    }
}