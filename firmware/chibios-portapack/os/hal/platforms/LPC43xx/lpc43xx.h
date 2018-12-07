/*
    Copyright (C) 2018 Jared Boone, ShareBrained Technology

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef _LPC43XX_H_
#define _LPC43XX_H_

typedef struct {
  LPC_CGU_BASE_CLK_Type* clk;
  __I uint32_t* stat;
  uint32_t stat_mask;
} base_clock_regs_t;

typedef struct {
  LPC_CCU1_CFG_160_Type* cfg;
  LPC_CCU_STAT_Type* stat;
} branch_clock_regs_t;

typedef struct {
  uint_fast8_t output_index;
} peripheral_reset_t;

typedef struct {
  IRQn_Type irq;
  uint32_t priority_mask;
} interrupt_config_t;

typedef struct {
  union {
    struct {
      uint16_t MODE : 3;
      uint16_t EPD  : 1;
      uint16_t EPUN : 1;
      uint16_t EHS  : 1;
      uint16_t EZI  : 1;
      uint16_t ZIF  : 1;
      uint16_t RESERVED0 : 8;
    };
    uint16_t word;
  };
} scu_config_t;

typedef struct {
  uint8_t port;
  uint8_t pin;
  scu_config_t config;
} scu_setup_t;

#ifdef __cplusplus
extern "C" {
#endif
  
void peripheral_reset(const peripheral_reset_t* const reset);

void base_clock_enable(const base_clock_regs_t* const base);
void base_clock_disable(const base_clock_regs_t* const base);

void branch_clock_enable(const branch_clock_regs_t* const branch);
void branch_clock_disable(const branch_clock_regs_t* const branch);

void interrupt_enable(const interrupt_config_t* const interrupt);
void interrupt_disable(const interrupt_config_t* const interrupt);

#ifdef __cplusplus
}
#endif

#endif /* _LPC43XX_H_ */
