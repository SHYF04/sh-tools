/*
 * gpio_test.c
 *
 *  Created on: 2025年10月24日
 *      Author: PC
 */
#include "gpio_test.h"

const gpio_test_group_t gpio_test_group_table[] =
{
	/*	端口组		引脚			端口组		引脚			 组名*/
	/*	测试A组		测试A引脚		测试B组		测试B引脚		 测试组名*/
	{{GPIO_GROUPD, GPIO_PD0}, {GPIO_GROUPA, GPIO_PA2}, "GROUP1"},
	{{GPIO_GROUPD, GPIO_PD2}, {GPIO_GROUPA, GPIO_PA3}, "GROUP2"},
	{{GPIO_GROUPD, GPIO_PD3}, {GPIO_GROUPA, GPIO_PA4}, "GROUP3"},
	{{GPIO_GROUPE, GPIO_PE1}, {GPIO_GROUPA, GPIO_PA5}, "GROUP4"},
	{{GPIO_GROUPE, GPIO_PE2}, {GPIO_GROUPA, GPIO_PA6}, "GROUP5"},
	{{GPIO_GROUPE, GPIO_PE3}, {GPIO_GROUPA, GPIO_PA7}, "GROUP6"},
	{{GPIO_GROUPE, GPIO_PE4}, {GPIO_GROUPB, GPIO_PB4}, "GROUP7"},
	{{GPIO_GROUPE, GPIO_PE5}, {GPIO_GROUPB, GPIO_PB5}, "GROUP8"},
	{{GPIO_GROUPE, GPIO_PE7}, {GPIO_GROUPB, GPIO_PB6}, "GROUP9"},
	{{GPIO_GROUPC, GPIO_PC0}, {GPIO_GROUPB, GPIO_PB7}, "GROUPA"},
};


#define GPIO_TEST_LEVEL_NUM			14
const uint32_t gpio_test_level[GPIO_TEST_LEVEL_NUM] =
{
	//需要引脚检测的电平值
	0x155, 0x2AA, 0x000, 0x3FF, 0x01F, 0x3E0, 0x0CC, 0x333, 0x455, 0x568, 0x783, 0x240, 0x031, 0x992
};

gpio_test_t gpio_test =
{
	.num = sizeof(gpio_test_group_table) / sizeof(gpio_test_group_t),
	.table = gpio_test_group_table,
};

static void gpio_set_output_mode(gpio_t* gpio_p, uint8_t enable)
{
	/*user customize begin*/
	gpio_set_output(gpio_p->pin, enable);
	/*user customize end*/
}

static void gpio_set_input_mode(gpio_t* gpio_p, uint8_t enable)
{
	/*user customize begin*/
	gpio_set_input(gpio_p->pin, enable);
	/*user customize end*/
}

static void gpio_set_output_level(gpio_t* gpio_p, uint8_t level)
{
	/*user customize begin*/
	gpio_set_level(gpio_p->pin, level);
	/*user customize end*/
}

static uint8_t gpio_get_input_level(gpio_t* gpio_p)
{
	/*user customize begin*/
	return gpio_get_level(gpio_p->pin);
	/*user customize end*/
}

static void gpio_set_input_pull(gpio_t* gpio_p, uint8_t pull)
{
	/*user customize begin*/
	if(pull)
	{
		gpio_set_digital_pullup(gpio_p->pin);
	}
	else
	{
		gpio_set_digital_pulldown(gpio_p->pin);

	}
	/*user customize end*/
}

static void gpio_mode_disable(gpio_test_t* gpio_test_p)
{
	uint16_t i = 0;

	for(i = 0; i < gpio_test_p->num; i++)
	{
		/*将一半设置为输出，一半设置为输入*/
		gpio_set_output_mode(& ( ((gpio_test_group_t*) (gpio_test_p->table))[i].gpioA), 0);
		gpio_set_input_mode (& ( ((gpio_test_group_t*) (gpio_test_p->table))[i].gpioA), 0);
		gpio_set_output_mode(& ( ((gpio_test_group_t*) (gpio_test_p->table))[i].gpioB), 0);
		gpio_set_input_mode (& ( ((gpio_test_group_t*) (gpio_test_p->table))[i].gpioB), 0);
	}
}

static void gpio_mode_swap(gpio_test_t* gpio_test_p, uint8_t swap)//接口函数
{
	uint16_t i = 0;

	if(swap < 2)
	{
		gpio_mode_disable(gpio_test_p);
		switch(swap)
		{
			case 0:
			{
				for(i = 0; i < gpio_test_p->num; i++)
				{
					/*将一半设置为输出，一半设置为输入*/
					gpio_set_output_mode(& ( ((gpio_test_group_t*) (gpio_test_p->table))[i].gpioA), 1);
					gpio_set_input_mode (& ( ((gpio_test_group_t*) (gpio_test_p->table))[i].gpioB), 1);
				}
			}break;

			case 1:
			{
				for(i = 0; i < gpio_test_p->num; i++)
				{
					/*将一半设置为输出，一半设置为输入*/
					gpio_set_output_mode(& ( ((gpio_test_group_t*) (gpio_test_p->table))[i].gpioB), 1);
					gpio_set_input_mode (& ( ((gpio_test_group_t*) (gpio_test_p->table))[i].gpioA), 1);
				}
			}break;
		}
	}
}

uint32_t gpio_group_test(gpio_test_t* gpio_test_p)
{
	uint8_t out_va = 0;//gpio输出值
	uint16_t i = 0, j = 0, k = 0;
	uint32_t input_va = 0, output_va = 0;//记录输出输入值
	uint32 res = 0, temp_res[GPIO_TEST_LEVEL_NUM] = {0};//返回值

	for(k = 0; k < 2; k++)
	{
		gpio_mode_swap(gpio_test_p, k);//引脚IO功能交换
		delay_us(100);
		for(j = 0; j < GPIO_TEST_LEVEL_NUM; j++)
		{
			input_va = 0, output_va = 0;
			for(i = 0; i < gpio_test_p->num; i++)
			{
				if(gpio_test_level[j] & (0x01 << i))
					out_va = 1;
				else
					out_va = 0;

				if(!k)
					gpio_set_output_level(& ( ((gpio_test_group_t*) (gpio_test_p->table))[i].gpioA), out_va);
				else
					gpio_set_output_level(& ( ((gpio_test_group_t*) (gpio_test_p->table))[i].gpioB), out_va);

				delay_us(100);
				output_va <<= 1;
				output_va |= out_va;
			}

			delay_us(100);

			for(i = 0; i < gpio_test_p->num; i++)
			{
				if(!k)
					gpio_set_input_pull(& ( ((gpio_test_group_t*) (gpio_test_p->table))[i].gpioB), 1);
				else
					gpio_set_input_pull(& ( ((gpio_test_group_t*) (gpio_test_p->table))[i].gpioA), 1);

				delay_us(100);

				input_va <<= 1;
				if(!k)
					input_va |= gpio_get_input_level(& ( ((gpio_test_group_t*) (gpio_test_p->table))[i].gpioB));
				else
					input_va |= gpio_get_input_level(& ( ((gpio_test_group_t*) (gpio_test_p->table))[i].gpioA));

				delay_us(100);
			}
			temp_res[j] = (output_va ^ input_va);//分别记录两次高低电平的差异值，防止引脚“侥幸”逃脱检测,输出电平类型越多检测越严格
		}
	}
	for(i = 0; i < GPIO_TEST_LEVEL_NUM; i++)
	{
		res |= temp_res[i];
	}
	gpio_mode_disable(gpio_test_p);
	if(res)
	{
		uint32 res_swap = 0;
		for(i = 0; i < gpio_test_p->num; i++)
		{
			res_swap <<= 1;
			if(res & (0x01 << i))
			{
				res_swap |= 0x01;
			}
			else
			{
				res_swap &= (~0x01);
			}
		}
		res = res_swap;
	}
	return res;
}
