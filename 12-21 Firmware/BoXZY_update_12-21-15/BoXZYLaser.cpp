/*
    This file is part of BoXZY's version Repetier-Firmware.

    Repetier-Firmware is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Repetier-Firmware is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Repetier-Firmware.  If not, see <http://www.gnu.org/licenses/>.

    This firmware is a nearly complete rewrite of the sprinter firmware
    by kliment (https://github.com/kliment/Sprinter)
    which based on Tonokip RepRap firmware rewrite based off of Hydra-mmm firmware.

  Functions in this file are used to communicate using ascii or repetier protocol.
*/

#include "BoXZYLaser.h"

#include "Configuration.h"
#include "HAL.h"

BoXZYLBuffer_t BoXZYLBuffer;
uint8_t BoXZYLBuffer_t::elts[BOXZY_LASER_MAX_L_ELTS];

static bool is_laser_on_flag;
static bool is_fan_on_flag;

static void shut_fan_off(void)
{
    is_fan_on_flag = false;
    analogWrite(EXT0_HEATER_PIN, 0);
    WRITE(EXT0_HEATER_PIN,HEATER_PINS_INVERTED);
}

static void turn_fan_on(void)
{
    is_fan_on_flag = true;
    analogWrite(EXT0_HEATER_PIN, 128);
}

void manage_laser(void)
{
    if (is_fan_on_flag
        && !is_laser_on_flag
        && BoXZYLBuffer.is_empty())
    {
        shut_fan_off();
    }
}

void set_laser(uint8_t power)
{
    is_laser_on_flag = (power > 0.0);

    if (is_laser_on_flag)
    {
        turn_fan_on();
    }

    analogWrite(FAN_PIN, power);
}

void disable_laser(void)
{
    set_laser(0);
    shut_fan_off();
}


