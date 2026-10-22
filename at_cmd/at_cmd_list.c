#include "at_cmd.h"
#include "at_cmd_list.h"
#include <string.h>
#include <stdbool.h>

//static void at_cmd_template(size_t argc, char* argv[])
//{
//	if (argc && argv)//有参数
//	{
//		bool result = (argv[0] && (1 == argc) && (0 == strcmp("?", argv[0])));
//		switch (result)
//		{
//			case false://设置指令
//			{
//				//在获取参数时需判断当前参数是否为NULL
//				printf("当前为设置指令\n");
//			}break;
//
//			case true://查询指令
//			{
//				printf("当前为查询指令\n");
//			}break;
//		}
//	}
//	else//无参数/执行指令
//	{
//		printf("当前为执行指令\n");
//	}
//}

static void at_cmd_test(size_t argc, char* argv[])
{
	if (argc && argv)//有参数
	{
		bool result = (argv[0] && (1 == argc) && (0 == strcmp("?", argv[0])));
		switch (result)
		{
			case false://设置指令
			{
				//在获取参数时需判断当前参数是否为NULL
				//printf("当前为设置指令\n");
			}break;

			case true://查询指令
			{
				//printf("当前为查询指令\n");
			}break;
		}
	}
	else//无参数/执行指令
	{
		//printf("当前为执行指令\n");
		printf("+OK\n");
	}
}

static void at_cmd_lpower(size_t argc, char* argv[])
{
	if (argc && argv)//有参数
	{
		bool result = (argv[0] && (1 == argc) && (0 == strcmp("?", argv[0])));
		switch (result)
		{
			case false://设置指令
			{
				//在获取参数时需判断当前参数是否为NULL
				//printf("当前为设置指令\n");
				if (argv[0])
				{
					printf("%s\r\n", argv[0]);
				}
				if (argv[1])
				{
					printf("%s\r\n", argv[1]);
				}
				if (argv[2])
				{
					printf("%s\r\n", argv[2]);
				}
			}break;

			case true://查询指令
			{
				printf("当前为查询指令\n");
			}break;
		}
	}
	else//无参数/执行指令
	{
		printf("当前为执行指令\n");
	}
}

at_cmd_t at_cmd_list[] = 
{
	//指令		   		回调
	{"AT"			,at_cmd_test},
	{"AT+LPOWER"	,at_cmd_lpower},
	{NULL			,NULL}//指令表结束标志
};
