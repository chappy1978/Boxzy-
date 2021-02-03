/*
    This file is part of BoXZY's version of Repetier-Firmware.

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
#
  Functions in this file are used to communicate using ascii or repetier protocol.
*/

#ifndef BOXZYLASER_H_INCLUDED
#define BOXZYLASER_H_INCLUDED

#include <stdlib.h>
#include <stdint.h>

#include "Configuration.h"

#ifndef COUNTOF
#define COUNTOF(ARRAY)  (sizeof(ARRAY)/sizeof(ARRAY[0])) // TODO: Gcc-provided version
#endif

static inline uint8_t laser_pct_to_power(float pct)
{
    return pct >= 100.0 ? 255U
        :  pct <=   0.0 ?   0U
        :                 (uint8_t)(pct*2.55);
}

void manage_laser(void);

void set_laser(uint8_t power);

void disable_laser(void);


// Design note: This class uses the approach of never *quite* filling:
// there's always one unused elt[]. This prevents the other indexes from
// ever catching up to the oldest_index, which allows us to have simple
// test in pop() (which is often called from an IRQ) to prevent pop()ing
// too many in the event of defective data.
// 
// TODO: Allow saving / restoring of this to/from SD card and
class BoXZYLBuffer_t
{
    public:
        /// elts[write_index] is next available element, unless full
        uint16_t write_index;

        /// elts[committed_index] is oldest element that has been committed
        /// (post checksum, see GCode::pushCommand())
        uint16_t committed_index;

        /// elts[unclaimed_index] is oldest element not claimed by a G or M
        /// code.
        uint16_t unclaimed_index;

        /// elts[oldest_index] is oldest in-use element, unless full.
        volatile uint16_t oldest_index;

        /// Whether or not the ring is full
        bool is_full(void)
        {
            uint16_t p = write_index;
            inc(&p);
            return p == oldest_index;
        }

        /// Laser power values
        static uint8_t elts[BOXZY_LASER_MAX_L_ELTS];

        inline bool is_empty(void)
        {
            return (oldest_index == write_index);
        }

        void reset(void)
        {
            oldest_index = unclaimed_index = committed_index = write_index = 0;
        }

        inline void append_pct(float pct)
        {
            uint16_t p = write_index;
            inc(&p);

            if (p != oldest_index)
            {
                elts[write_index] = laser_pct_to_power(pct);
                write_index = p;
            }
        }

        static inline void inc(volatile uint16_t *index)
        {
            ++*index;
            if (*index >= COUNTOF(elts))
            {
                *index = 0;
            }
        }

        inline uint8_t pop(void)
        {
            uint8_t p = 0;
            if (oldest_index != unclaimed_index)
            {
                p = elts[oldest_index];
                inc(&oldest_index);
            }

            return p;
        }

        // Gcodes are processed FIFO from the serial port, so when one is processed
        // before being queued for motion, it will only have the latest L powers
        // in it. This allows them to be freed in no-motion cases.
        inline void free_recently_committed(uint16_t index, uint16_t end_index)
        {
            if (committed_index == end_index)
            {
                if (unclaimed_index == committed_index)
                {
                    unclaimed_index = index;
                }
                committed_index = index;
                write_index = committed_index;
            }
            else
            {
                // Should never happen
                // TODO: fill with 0s and have pop() skip them?
            }
        }

        /// Returns (index_a - index_b) assuming a non-empty range (will never
        /// return 0).
        static inline uint16_t subtract(uint16_t index_a, uint16_t index_b)
        {
            if (index_a <= index_b)
            {
                index_a += COUNTOF(elts);
            }

            return index_a - index_b;
        }
};

extern BoXZYLBuffer_t BoXZYLBuffer;

#endif // MOTION_H_INCLUDED
