/*****************************************************************************
 *  Copyright 2018 Thomas Watson                                             *
 *                                                                           *
 *  This file is a part of 88mph: https://github.com/tpwrules/121gw-88mph/   *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *      http://www.apache.org/licenses/LICENSE-2.0                           *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *****************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include "stm32l1xx.h"

#include "system/timer.h"

// number of milliseconds since timer was inited
volatile uint32_t timer_1ms_ticks = 0;
// number of 10 millisecond periods since timer was inited
volatile uint32_t timer_10ms_ticks = 0;

static volatile bool timer_is_inited = false;

void timer_init(void) {
    // 1ms timer is already set up by HAL
    // so just turn on the flag
    __disable_irq();
    timer_is_inited = true;
    // also zero out the tick counts
    timer_1ms_ticks = 0;
    timer_10ms_ticks = 0;
    __enable_irq();

    // set up TIM6 to interrupt at a 10ms interval
    // this happens every 120000 system clock cycles

    // power up timer and bring it out of reset
    __HAL_RCC_TIM6_FORCE_RESET();
    __HAL_RCC_TIM6_CLK_ENABLE();
    __HAL_RCC_TIM6_RELEASE_RESET();

    // we want 1 timer cycle per 64 clock cycles
    TIM6->PSC = 64-1;

    // automatically reload with 120000/64
    TIM6->ARR = 120000/64;

    TIM6->DIER = TIM_DIER_UIE; // enable interrupt on update
    TIM6->CR1 = TIM_CR1_URS | // only trigger update on overflow
                TIM_CR1_CEN; // turn on counting

    // enable the timer interrupt
    NVIC_EnableIRQ(TIM6_IRQn);
}

void timer_deinit(void) {
    // 1ms timer must be kept running for HAL
    // so just turn off the flag
    __disable_irq();
    timer_is_inited = false;
    __enable_irq();

    // turn off the timer IRQ
    NVIC_DisableIRQ(TIM6_IRQn);
    NVIC_ClearPendingIRQ(TIM6_IRQn);

    // and turn off its clock
    __HAL_RCC_TIM6_CLK_DISABLE();
}

// callback for 1ms timer
// eventually this will just be the systick handler
// but for now HAL needs it too and it stops us overriding
// so we get called by HAL, with some performance penalty
void HAL_SYSTICK_Callback(void) {
    // HAL needs to be running all the time or we will hang
    // so only do our work if we are inited
    if (!timer_is_inited) return;
    timer_1ms_ticks++;
}

// callback for 10ms timer
void TIM6_IRQHandler(void) {
    // acknowledge interrupt
    TIM6->SR = 0;

    timer_10ms_ticks++;
}