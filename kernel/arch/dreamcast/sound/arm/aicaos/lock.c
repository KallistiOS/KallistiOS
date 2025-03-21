/*
   AICAOS

   lock.c
   Copyright (C) 2025 Paul Cercueil

   Mutex implementation
*/

#include <aicaos/irq.h>
#include <aicaos/lock.h>
#include <aicaos/task.h>
#include <stddef.h>

void mutex_lock(struct mutex *lock)
{
    irq_disable_scoped();

    while (lock->owner) {
        task_boost(lock->owner);
        task_wait(lock);
    }

    lock->owner = current_task;
}

bool mutex_trylock(struct mutex *lock)
{
    bool success;

    irq_disable_scoped();

    success = !lock->owner;
    if (!lock->owner)
        lock->owner = current_task;

    return success;
}

void mutex_unlock(struct mutex *lock)
{
    irq_disable_scoped();

    lock->owner = NULL;
    task_wake(lock, 1);
    task_unboost();
}
