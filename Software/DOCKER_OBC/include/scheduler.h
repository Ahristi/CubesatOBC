#ifndef SCHEDULER_H
#define SCHEDULER_H
#include <Arduino.h>

#include "adcs.h"
#include "comms.h"
#include "eps.h"
#include "logging.h"
#include "packet.h"
#include "payload.h"


typedef void (*TaskFunction)();

struct Task {
    TaskFunction function;
    uint32_t period_ms;
    uint32_t last_run_ms;
};

void Scheduler_init();
void Scheduler_run();

#endif