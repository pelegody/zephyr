/*
 * Copyright (c) Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief
 *
 * Based on reference manual:
 *   RM0368 Reference manual STM32F401xB/C and STM32F401xD/E
 *   advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 8: General-purpose I/Os (GPIOs)
 */

#include <errno.h>

#include <device.h>
#include "soc.h"
#include "soc_registers.h"
#include <gpio.h>
#include <gpio/gpio_stm32.h>

/**
 * @brief map pin function to MODE register value
 */
static u32_t __func_to_mode(int func)
{
	switch (func) {
	case STM32F4X_PIN_CONFIG_BIAS_HIGH_IMPEDANCE:
	case STM32F4X_PIN_CONFIG_BIAS_PULL_UP:
	case STM32F4X_PIN_CONFIG_BIAS_PULL_DOWN:
		return 0x0;
	case STM32F4X_PIN_CONFIG_DRIVE_PUSH_PULL:
	case STM32F4X_PIN_CONFIG_DRIVE_PUSH_UP:
	case STM32F4X_PIN_CONFIG_DRIVE_PUSH_DOWN:
	case STM32F4X_PIN_CONFIG_DRIVE_OPEN_DRAIN:
	case STM32F4X_PIN_CONFIG_DRIVE_OPEN_UP:
	case STM32F4X_PIN_CONFIG_DRIVE_OPEN_DOWN:
		return 0x1;
	case STM32F4X_PIN_CONFIG_AF_PUSH_PULL:
	case STM32F4X_PIN_CONFIG_AF_PUSH_UP:
	case STM32F4X_PIN_CONFIG_AF_PUSH_DOWN:
	case STM32F4X_PIN_CONFIG_AF_OPEN_DRAIN:
	case STM32F4X_PIN_CONFIG_AF_OPEN_UP:
	case STM32F4X_PIN_CONFIG_AF_OPEN_DOWN:
		return 0x2;
	case STM32F4X_PIN_CONFIG_ANALOG:
		return 0x3;
	}

	return 0;
}

/**
 * @brief map pin function to OTYPE register value
 */
static u32_t __func_to_otype(int func)
{
	switch (func) {
	case STM32F4X_PIN_CONFIG_DRIVE_OPEN_DRAIN:
	case STM32F4X_PIN_CONFIG_DRIVE_OPEN_UP:
	case STM32F4X_PIN_CONFIG_DRIVE_OPEN_DOWN:
	case STM32F4X_PIN_CONFIG_AF_OPEN_DRAIN:
	case STM32F4X_PIN_CONFIG_AF_OPEN_UP:
	case STM32F4X_PIN_CONFIG_AF_OPEN_DOWN:
		return 0x1;
	}

	return 0;
}

/**
 * @brief map pin function to OSPEED register value
 */
static u32_t __func_to_ospeed(int func)
{
	switch (func) {
	case STM32F4X_PIN_CONFIG_DRIVE_PUSH_PULL:
	case STM32F4X_PIN_CONFIG_DRIVE_PUSH_UP:
	case STM32F4X_PIN_CONFIG_DRIVE_PUSH_DOWN:
	case STM32F4X_PIN_CONFIG_DRIVE_OPEN_DRAIN:
	case STM32F4X_PIN_CONFIG_DRIVE_OPEN_UP:
	case STM32F4X_PIN_CONFIG_DRIVE_OPEN_DOWN:
	case STM32F4X_PIN_CONFIG_AF_PUSH_PULL:
	case STM32F4X_PIN_CONFIG_AF_PUSH_UP:
	case STM32F4X_PIN_CONFIG_AF_PUSH_DOWN:
	case STM32F4X_PIN_CONFIG_AF_OPEN_DRAIN:
	case STM32F4X_PIN_CONFIG_AF_OPEN_UP:
	case STM32F4X_PIN_CONFIG_AF_OPEN_DOWN:
		/* Force fast speed by default */
		return 0x2;
	}

	return 0;
}

/**
 * @brief map pin function to PUPD register value
 */
static u32_t __func_to_pupd(int func)
{
	switch (func) {
	case STM32F4X_PIN_CONFIG_DRIVE_PUSH_PULL:
	case STM32F4X_PIN_CONFIG_DRIVE_OPEN_DRAIN:
	case STM32F4X_PIN_CONFIG_AF_PUSH_PULL:
	case STM32F4X_PIN_CONFIG_AF_OPEN_DRAIN:
	case STM32F4X_PIN_CONFIG_BIAS_HIGH_IMPEDANCE:
	case STM32F4X_PIN_CONFIG_ANALOG:
		return 0x0;
	case STM32F4X_PIN_CONFIG_DRIVE_PUSH_UP:
	case STM32F4X_PIN_CONFIG_DRIVE_OPEN_UP:
	case STM32F4X_PIN_CONFIG_AF_PUSH_UP:
	case STM32F4X_PIN_CONFIG_AF_OPEN_UP:
	case STM32F4X_PIN_CONFIG_BIAS_PULL_UP:
		return 0x1;
	case STM32F4X_PIN_CONFIG_DRIVE_PUSH_DOWN:
	case STM32F4X_PIN_CONFIG_DRIVE_OPEN_DOWN:
	case STM32F4X_PIN_CONFIG_AF_PUSH_DOWN:
	case STM32F4X_PIN_CONFIG_AF_OPEN_DOWN:
	case STM32F4X_PIN_CONFIG_BIAS_PULL_DOWN:
		return 0x2;
	}

	return 0;
}


int stm32_gpio_flags_to_conf(int flags, int *pincfg)
{
	int direction = flags & GPIO_DIR_MASK;
	int pud = flags & GPIO_PUD_MASK;

	if (!pincfg) {
		return -EINVAL;
	}

	if (direction == GPIO_DIR_OUT) {
		if (pud == GPIO_PUD_PULL_UP) {
			*pincfg = STM32F4X_PIN_CONFIG_DRIVE_PUSH_UP;
		} else if (pud == GPIO_PUD_PULL_DOWN) {
			*pincfg = STM32F4X_PIN_CONFIG_DRIVE_PUSH_DOWN;
		} else {
			*pincfg = STM32F4X_PIN_CONFIG_DRIVE_PUSH_PULL;
		}
	} else {
		if (pud == GPIO_PUD_PULL_UP) {
			*pincfg = STM32F4X_PIN_CONFIG_BIAS_PULL_UP;
		} else if (pud == GPIO_PUD_PULL_DOWN) {
			*pincfg = STM32F4X_PIN_CONFIG_BIAS_PULL_DOWN;
		} else {
			*pincfg = STM32F4X_PIN_CONFIG_BIAS_HIGH_IMPEDANCE;
		}
	}

	return 0;
}

int stm32_gpio_configure(u32_t *base_addr, int pin, int conf, int altf)
{
	volatile struct stm32f4x_gpio *gpio =
		(struct stm32f4x_gpio *)(base_addr);
	u32_t mode = __func_to_mode(conf);
	u32_t otype = __func_to_otype(conf);
	u32_t ospeed = __func_to_ospeed(conf);
	u32_t pupd = __func_to_pupd(conf);
	u32_t tmpreg = 0;

	/* TODO: validate if indeed alternate */
	if (altf) {
		/* Set the alternate function */
		tmpreg = gpio->afr[pin >> 0x3];
		tmpreg &= ~(0xf << ((pin & 0x07) * 4));
		tmpreg |= (altf << ((pin & 0x07) * 4));
		gpio->afr[pin >> 0x3] = tmpreg;
	}

	/* Set the IO direction mode */
	tmpreg = gpio->mode;
	tmpreg &= ~(0x3 << (pin * 2));
	tmpreg |= (mode << (pin * 2));
	gpio->mode = tmpreg;

	if (otype) {
		tmpreg = gpio->otype;
		tmpreg &= ~(0x1 << pin);
		tmpreg |= (otype << pin);
		gpio->otype = tmpreg;
	}

	if (ospeed) {
		tmpreg = gpio->ospeed;
		tmpreg &= ~(0x3 << (pin * 2));
		tmpreg |= (ospeed << (pin * 2));
		gpio->ospeed = tmpreg;
	}

	tmpreg = gpio->pupdr;
	tmpreg &= ~(0x3 << (pin * 2));
	tmpreg |= (pupd << (pin * 2));
	gpio->pupdr = tmpreg;

	return 0;
}

int stm32_gpio_set(u32_t *base, int pin, int value)
{
	struct stm32f4x_gpio *gpio = (struct stm32f4x_gpio *)base;

	if (value) {
		/* atomic set */
		gpio->bsr = (1 << (pin & 0x0f));
	} else {
		/* atomic reset */
		gpio->bsr = (1 << ((pin & 0x0f) + 0x10));
	}

	return 0;
}

int stm32_gpio_get(u32_t *base, int pin)
{
	struct stm32f4x_gpio *gpio = (struct stm32f4x_gpio *)base;

	return (gpio->idr >> pin) & 0x1;
}

int stm32_gpio_enable_int(int port, int pin)
{
	volatile struct stm32f4x_syscfg *syscfg =
		(struct stm32f4x_syscfg *)SYSCFG_BASE;
	volatile union syscfg_exticr *exticr;
	struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	struct stm32_pclken pclken = {
		.bus = STM32_CLOCK_BUS_APB2,
		.enr = LL_APB2_GRP1_PERIPH_SYSCFG
	};
	int shift = 0;

	/* Enable SYSCFG clock */
	clock_control_on(clk, (clock_control_subsys_t *) &pclken);

	if (pin <= 3) {
		exticr = &syscfg->exticr1;
	} else if (pin <= 7) {
		exticr = &syscfg->exticr2;
	} else if (pin <= 11) {
		exticr = &syscfg->exticr3;
	} else if (pin <= 15) {
		exticr = &syscfg->exticr4;
	} else {
		return -EINVAL;
	}

	shift = 4 * (pin % 4);

	exticr->val &= ~(0xf << shift);
	exticr->val |= port << shift;

	return 0;
}
