#ifndef __AT_CMD_H__
#define __AT_CMD_H__

#include <stdint.h>

/****** AT CMD Assert ******/
#define AT_CMD_ASSERT(expr)			do{if(!(expr)) {goto end;}}while(0)

/****** AT CMD Config ******/
#define AT_CMD_CRLF					""		//指令结束符配置
#define AT_CMD_NAME_DXX_SUPPORT		1		//指令名称支持大小写字母混合 1-en 0-dis
#define	AT_CMD_ESCAPE_SUPPORT		1		//指令参数支持转义字符 1-en 0-dis
#define AT_CMD_PARAM_NUM_MAX		10		//指令参数最大数量
#define AT_CMD_LEN_MAX				256		//指令支持的最大长度

/****** AT CMD Type ******/
typedef void (*at_cmd_callback) (size_t argc, char* argv[]);
typedef enum __AT_CMD_RET		at_cmd_ret_t;
typedef struct __AT_CMD			at_cmd_t;
typedef struct __AT_CMD_ADMIN	at_cmd_admin_t;
typedef struct __AT_CMD_PARAM	at_cmd_param_t;

typedef enum __AT_CMD_RET
{
	AT_CMD_OK = 0,
	AT_CMD_ERR,
}at_cmd_ret_t;

typedef struct __AT_CMD
{
	const char* name;
	at_cmd_callback cb;
}at_cmd_t;

typedef struct __AT_CMD_PARAM
{
	char* list[AT_CMD_PARAM_NUM_MAX];
	size_t number;
}at_cmd_param_t;

typedef struct __AT_CMD_ADMIN	//指令表管理结构体
{
	at_cmd_t*		list;//指令列表
	size_t			number;//指令表数量
	at_cmd_param_t	param;//当前指令参数
}at_cmd_admin_t;

/****** AT CMD Fun ******/
at_cmd_ret_t at_cmd_handle(at_cmd_admin_t* at_cmd_admin, char* buffer, size_t len);
at_cmd_ret_t at_cmd_init(at_cmd_admin_t* at_cmd_admin, at_cmd_t* at_cmd_list);
#endif /*__AT_CMD_H__*/
