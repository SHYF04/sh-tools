/*
 * gpio_test.h
 *
 *  Created on: 2025年10月24日
 *      Author: PC
 */

#ifndef VENDOR_FEATURE_TEST_GPIO_TEST_H_
#define VENDOR_FEATURE_TEST_GPIO_TEST_H_

#include "string.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "feature_dle/app.h"

#define gpio_pin_t 		uint16_t
#define gpio_port_t		uint16_t

typedef struct __GPIO
{
	gpio_port_t port;//端口组
	gpio_pin_t  pin;//引脚号

}gpio_t;

typedef struct __GPIO_TEST_GROUP
{
	gpio_t	gpioA;
	gpio_t	gpioB;
	const char* name;

}gpio_test_group_t;

typedef struct __GPIO_TEST
{
	uint16_t 					num;//gpio需要测试的数量-(需成对)
	const gpio_test_group_t*	table;

}gpio_test_t;

uint32_t gpio_group_test(gpio_test_t* gpio_test_p);
extern gpio_test_t gpio_test;

#endif /* VENDOR_FEATURE_TEST_GPIO_TEST_H_ */
