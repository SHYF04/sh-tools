#ifndef __AT_CMD_H__
#define __AT_CMD_H__


typedef unsigned long long sh_size_t;

typedef void(*at_cmd_log_t)	(const char* fmt, ...);
extern at_cmd_log_t at_cmd_log;

#define AT_CMD_LOG(fmt, ...)	do{if(at_cmd_log) {at_cmd_log(fmt, ##__VA_ARGS__);}}while(0)

/****** AT CMD Assert ******/
#define AT_CMD_ASSERT(expr)			do{if(!(expr)) {goto end;}}while(0)

/****** AT CMD Config ******/
#define AT_CMD_CRLF					"\r\n"		//CRLF
#define AT_CMD_NAME_DXX_SUPPORT		1		//?指令大小写1-en 0-dis
#define	AT_CMD_ESCAPE_SUPPORT		1		//?转义字符1-en 0-dis
#define AT_CMD_PARAM_NUM_MAX		10		//指令参数最大数量
#define AT_CMD_LEN_MAX				256		//?指令最大长度

/****** AT CMD Type ******/
typedef void (*at_cmd_callback) (sh_size_t argc, char* argv[]);
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
	sh_size_t number;
}at_cmd_param_t;

typedef struct __AT_CMD_ADMIN
{
	at_cmd_t*		list;
	sh_size_t		number;
	at_cmd_param_t	param;
}at_cmd_admin_t;

/****** AT CMD Fun ******/
void at_cmd_log_register(at_cmd_log_t log_func);
at_cmd_ret_t at_cmd_handle(at_cmd_admin_t* at_cmd_admin, char* buffer, sh_size_t len);
at_cmd_ret_t at_cmd_init(at_cmd_admin_t* at_cmd_admin, at_cmd_t* at_cmd_list);
#endif /*__AT_CMD_H__*/
