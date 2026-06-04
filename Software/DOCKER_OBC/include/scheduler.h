#ifndef SCHEDULER_H
#define SCHEDULER_H
#include <Arduino.h>

#include "adcs.h"
#include "comms.h"
#include "eps.h"
#include "logging.h"
#include "payload.h"
#include "system.h"

typedef void (*TaskFunction)();

struct Task {
    TaskFunction function;
    uint32_t period_ms;
    uint32_t last_run_ms;
    uint32_t duration;
    const char *name;
};

void Scheduler_init();
void Scheduler_run();
void Scheduler_debugPrint();
#endif