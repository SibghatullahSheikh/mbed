/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "pwmout_api.h"

#include "cmsis.h"
#include "pinmap.h"
#include "error.h"

// Only TIM2 and TIM3 can be used (TIM1 and TIM4 are used by the us_ticker)
static const PinMap PinMap_PWM[] = {
    // TIM2
    {PA_2,  PWM_2, STM_PIN_DATA(GPIO_Mode_AF_PP, 0)}, // TIM2_CH3 OK
    {PA_3,  PWM_2, STM_PIN_DATA(GPIO_Mode_AF_PP, 0)}, // TIM2_CH4 OK
    // TIM2 remap
    {PA_15, PWM_2, STM_PIN_DATA(GPIO_Mode_AF_PP, 5)}, // TIM2r_CH1 FAIL
    {PB_3,  PWM_2, STM_PIN_DATA(GPIO_Mode_AF_PP, 5)}, // TIM2r_CH2 FAIL - ARDUINO D3
    {PB_10, PWM_2, STM_PIN_DATA(GPIO_Mode_AF_PP, 5)}, // TIM2r_CH3 OK - ARDUINO D6
    {PB_11, PWM_2, STM_PIN_DATA(GPIO_Mode_AF_PP, 5)}, // TIM2r_CH4 OK
    // TIM3
    {PA_6,  PWM_3, STM_PIN_DATA(GPIO_Mode_AF_PP, 0)}, // TIM3_CH1 OK
    {PA_7,  PWM_3, STM_PIN_DATA(GPIO_Mode_AF_PP, 0)}, // TIM3_CH2 OK - ARDUINO D11
    {PB_1,  PWM_3, STM_PIN_DATA(GPIO_Mode_AF_PP, 0)}, // TIM3_CH4 OK
    // TIM3 remap
    {PB_4,  PWM_3, STM_PIN_DATA(GPIO_Mode_AF_PP, 6)}, // TIM3r_CH1 FAIL - ARDUINO D5
    {PC_6,  PWM_3, STM_PIN_DATA(GPIO_Mode_AF_PP, 6)}, // TIM3r_CH1 OK
    {PC_7,  PWM_3, STM_PIN_DATA(GPIO_Mode_AF_PP, 6)}, // TIM3r_CH2 OK - ARDUINO D9
    {PB_5,  PWM_3, STM_PIN_DATA(GPIO_Mode_AF_PP, 6)}, // TIM3r_CH2 FAIL - Bug confirmed in ES
    {PC_8,  PWM_3, STM_PIN_DATA(GPIO_Mode_AF_PP, 6)}, // TIM3r_CH3 OK
    {PC_9,  PWM_3, STM_PIN_DATA(GPIO_Mode_AF_PP, 6)}, // TIM3r_CH4 OK
    {NC,    NC,    0}
};

void pwmout_init(pwmout_t* obj, PinName pin) {  
    // Get the peripheral name from the pin and assign it to the object
    obj->pwm = (PWMName)pinmap_peripheral(pin, PinMap_PWM);
  
    if (obj->pwm == (PWMName)NC) {
        error("PWM pinout mapping failed");
    }
    
    // Enable TIM clock
    if (obj->pwm == PWM_2) RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    if (obj->pwm == PWM_3) RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
 
    // Configure GPIO
    pinmap_pinout(pin, PinMap_PWM);
    
    obj->pin = pin;
    obj->period = 0;
    obj->pulse = 0;
    
    pwmout_period_us(obj, 20000); // 20 ms per default
}

void pwmout_free(pwmout_t* obj) {
    TIM_TypeDef *tim = (TIM_TypeDef *)(obj->pwm);
    TIM_DeInit(tim);
}

void pwmout_write(pwmout_t* obj, float value) {
    TIM_TypeDef *tim = (TIM_TypeDef *)(obj->pwm);
    TIM_OCInitTypeDef TIM_OCInitStructure;
  
    if (value < 0.0) {
        value = 0.0;
    } else if (value > 1.0) {
        value = 1.0;
    }

    //while(TIM_GetFlagStatus(tim, TIM_FLAG_Update) == RESET);
    //TIM_ClearFlag(tim, TIM_FLAG_Update);
    
    obj->pulse = (uint32_t)((float)obj->period * value);
    
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = obj->pulse;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

    if ((obj->pin == PA_6) || (obj->pin == PA_15) || (obj->pin == PB_4) || (obj->pin == PC_6)) { // TIM Channel 1
        TIM_OC1PreloadConfig(tim, TIM_OCPreload_Enable);
        TIM_OC1Init(tim, &TIM_OCInitStructure);
    }

    if ((obj->pin == PA_7) || (obj->pin == PB_3) || (obj->pin == PB_5) || (obj->pin == PC_7)) { // TIM Channel 2
        TIM_OC2PreloadConfig(tim, TIM_OCPreload_Enable);
        TIM_OC2Init(tim, &TIM_OCInitStructure);
    }

    if ((obj->pin == PA_2) || (obj->pin == PB_10) || (obj->pin == PC_8)) { // TIM Channel 3
        TIM_OC3PreloadConfig(tim, TIM_OCPreload_Enable);
        TIM_OC3Init(tim, &TIM_OCInitStructure);
    }

    if ((obj->pin == PA_3) || (obj->pin == PB_1) || (obj->pin == PB_11) || (obj->pin == PC_9)) { // TIM Channel 4
        TIM_OC4PreloadConfig(tim, TIM_OCPreload_Enable);
        TIM_OC4Init(tim, &TIM_OCInitStructure);
    }
}

float pwmout_read(pwmout_t* obj) {
    float value = 0;
    if (obj->period > 0) {
        value = (float)(obj->pulse) / (float)(obj->period);
    }
    return ((value > 1.0) ? (1.0) : (value));
}

void pwmout_period(pwmout_t* obj, float seconds) {
    pwmout_period_us(obj, seconds * 1000000.0f);
}

void pwmout_period_ms(pwmout_t* obj, int ms) {
    pwmout_period_us(obj, ms * 1000);
}

void pwmout_period_us(pwmout_t* obj, int us) {
    TIM_TypeDef *tim = (TIM_TypeDef *)(obj->pwm);
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    float dc = pwmout_read(obj);

    TIM_Cmd(tim, DISABLE);  
    
    obj->period = us;
  
    TIM_TimeBaseStructure.TIM_Period = obj->period - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = (uint16_t)(SystemCoreClock / 1000000) - 1; // 1 �s tick
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(tim, &TIM_TimeBaseStructure);

    // Set duty cycle again
    pwmout_write(obj, dc);
  
    TIM_ARRPreloadConfig(tim, ENABLE);    
    TIM_Cmd(tim, ENABLE);
}

void pwmout_pulsewidth(pwmout_t* obj, float seconds) {
    pwmout_pulsewidth_us(obj, seconds * 1000000.0f);
}

void pwmout_pulsewidth_ms(pwmout_t* obj, int ms) {
    pwmout_pulsewidth_us(obj, ms * 1000);
}

void pwmout_pulsewidth_us(pwmout_t* obj, int us) {
    float value = (float)us / (float)obj->period;
    pwmout_write(obj, value);
}
