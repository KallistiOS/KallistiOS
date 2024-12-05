/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/upgrades.h
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#ifndef BBOX_UPGRADE_H
#define BBOX_UPGRADE_H

#include <stdint.h>

typedef enum PlayerUpgrade : uint8_t {
    UPGRADE_SPEED,
    UPGRADE_METER,
    UPGRADE_TELEPORT_UNLOCK,
    UPGRADE_TELEPORT_COOLDOWN,
    UPGRADE_TELEPORT_DISTANCE,
    UPGRADE_SLOWDOWN_UNLOCK,
    UPGRADE_SLOWDOWN_DRAIN,
    UPGRADE_COUNT
} player_upgrade_t;

typedef struct PlayerUpgradeLevels {
        uint8_t speed : 4;
        uint8_t meter : 4;
        uint8_t teleport_cooldown : 4;
        uint8_t teleport_distance : 4;
        uint8_t slowdown_drain : 4;
        bool    teleport_unlocked : 1;
        bool    slowdown_unlocked : 1;
} player_upgrade_levels_t;

#endif