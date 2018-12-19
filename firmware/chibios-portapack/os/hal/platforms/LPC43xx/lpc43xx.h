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

struct scu_config_normal_drive_t {
  uint16_t mode;
  uint16_t epd;
  uint16_t epun;
  uint16_t ehs;
  uint16_t ezi;
  uint16_t zif;
};

struct scu_config_sfsi2c0_t {
  uint16_t scl_efp;
  uint16_t scl_ehd;
  uint16_t scl_ezi;
  uint16_t scl_zif;
  uint16_t sda_efp;
  uint16_t sda_ehd;
  uint16_t sda_ezi;
  uint16_t sda_zif;
};

struct scu_config_t {
#ifdef __cplusplus
  scu_config_t() = delete;
  scu_config_t(const scu_config_t&) = delete;
  scu_config_t(scu_config_t&) = delete;

  constexpr scu_config_t(
    uint16_t value
  ) :
    word(value)
  {
  }
  
  constexpr scu_config_t(
    const scu_config_normal_drive_t config
  ) :
    word(
        ((config.mode & 7) << 0)
      | ((config.epd  & 1) << 3)
      | ((config.epun & 1) << 4)
      | ((config.ehs  & 1) << 5)
      | ((config.ezi  & 1) << 6)
      | ((config.zif  & 1) << 7)
    )
  {
  }

  constexpr scu_config_t(
    const scu_config_sfsi2c0_t config
  ) :
    word(
        ((config.scl_efp & 1) <<  0)
      | ((config.scl_ehd & 1) <<  2)
      | ((config.scl_ezi & 1) <<  3)
      | ((config.scl_zif & 1) <<  7)
      | ((config.sda_efp & 1) <<  8)
      | ((config.sda_ehd & 1) << 10)
      | ((config.sda_ezi & 1) << 11)
      | ((config.sda_zif & 1) << 15)
    )
  {
  }
  
  constexpr operator uint32_t() const {
    return word;
  }

private:
#endif
  uint16_t word;
};

#ifndef __cplusplus
typedef struct scu_config_t scu_config_t;
#endif

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
