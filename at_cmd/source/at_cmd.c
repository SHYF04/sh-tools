#include "at_cmd.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


#if AT_CMD_NAME_DXX_SUPPORT
/**
 * @brief 字符串小写转大写
 * @param src 输入字符串
 * @param len 输入字符串长度
 * @return void
 */
static void at_cmd_str2upper(char* str, size_t len)
{
	if (str && len)
	{
		for (size_t i = 0; i < len; i++)
		{
			str[i] = toupper(str[i]);//转大写
		}
	}
}
#endif

#if AT_CMD_ESCAPE_SUPPORT
/**
 * @brief 解析字符串包含的转义字符
 * @param src 输入字符串
 * @param dst 输出缓冲区
 * @param src_size 输入字符串长度
 * @param dst_size 输出字符串长度
 * @return 解析后的字符串长度
 */
static int at_cmd_str2escape(const char* src, char* dst, size_t src_size, size_t dst_size)
{
	if (!src || !dst || src_size == 0 || dst_size == 0) return -1;

	const char* s = src;
	const char* end = src + src_size;
	char* d = dst;
	char* dst_end = dst + dst_size - 1;
	int len = 0;
	while (s < end && *s && d < dst_end)
	{
		if (*s == '\\' && s + 1 < end)
		{
			s++;  // 跳过 '\'
			switch (*s)
			{
				case 'a': *d++ = '\a'; len++; s++; break;
				case 'b': *d++ = '\b'; len++; s++; break;
				case 'f': *d++ = '\f'; len++; s++; break;
				case 'n': *d++ = '\n'; len++; s++; break;
				case 'r': *d++ = '\r'; len++; s++; break;
				case 't': *d++ = '\t'; len++; s++; break;
				case 'v': *d++ = '\v'; len++; s++; break;
				case '\\': *d++ = '\\'; len++; s++; break;
				case '\'': *d++ = '\''; len++; s++; break;
				case '\"': *d++ = '\"'; len++; s++; break;
				case '?':  *d++ = '\?'; len++; s++; break;
				case 'x':  // 十六进制 \xHH
				{
					s++;
					if (s >= end || !isxdigit(*s))
					{
						// 无效十六进制，原样保留
						if (d + 1 < dst_end)
						{
							*d++ = '\\';
							*d++ = 'x';
							len += 2;
						}
						// 跳过无效字符（如果有的话）
						if (s < end) s++;
						break;
					}
					char hex[3] = { *s++, 0, 0 };
					if (s < end && isxdigit(*s))
					{
						hex[1] = *s++;
					}
					*d++ = (char)strtol(hex, NULL, 16);
					len++;
					break;
				}
				default:  // 八进制 \OOO 或无效转义
				{
					if (*s >= '0' && *s <= '7')
					{
						char oct[4] = { *s++, 0, 0, 0 };
						int i = 1;
						while (i < 3 && s < end && *s >= '0' && *s <= '7')
						{
							oct[i++] = *s++;
						}
						*d++ = (char)strtol(oct, NULL, 8);
						len++;
					}
					else
					{
						// 无效转义，原样保留
						if (d + 1 < dst_end)
						{
							*d++ = '\\';
							*d++ = *s;
							len += 2;
						}
						s++;
					}
					break;
				}
			}
		}
		else
		{
			*d++ = *s++;
			len++;
		}
	}
	*d = '\0';
	return len;
}
#endif

/**
 * @brief 根据指令名称在指令列表搜索
 * @param at_cmd_admin 指令列表管理变量
 * @param name 需要寻找的指令
 * @param len 指令长度
 * @return 返回指令
 */
static at_cmd_t* at_cmd_find(at_cmd_admin_t* at_cmd_admin, const char* name, size_t len)
{
	AT_CMD_ASSERT(at_cmd_admin && name && len);
	for (size_t i = 0; i < at_cmd_admin->number; i++)
	{
		if (0 == strncmp(at_cmd_admin->list[i].name, name, len))
		{
			return (&at_cmd_admin->list[i]);
		}
	}
end:
	return NULL;
}

/**
 * @brief 在src字符串中查询dst字符串
 * @param src 被搜索的字符串
 * @param dst 需要被查询的字符串
 * @return 返回查找到的字符串首地址
 */
static char* at_cmd_strstr(const char* src, const char* dst)
{
	char* pointer = NULL;
	pointer = strstr(src, dst);
	if (pointer && (0 == strcmp(dst, "")))
	{
		pointer += strlen(src);
	}
	return pointer;
}

/**
 * @brief 将at指令参数恢复缺省值
 * @param at_cmd_admin
 * @return void
 */
static void at_cmd_param_default(at_cmd_admin_t* at_cmd_admin)
{
	memset(at_cmd_admin->param.list, 0, sizeof(char*) * AT_CMD_PARAM_NUM_MAX);
	at_cmd_admin->param.number = 0;
}

/**
 * @brief 计算指令表的指令数量
 * @param at_cmd_list 待计算的指令列表
 * @return 指令数量
 */
static size_t at_cmd_get_list_number(at_cmd_t* at_cmd_list)
{
	size_t number = 0;
	while (at_cmd_list[number].cb && at_cmd_list[number].name)
	{
		number++;
	}
	return number;
}

/**
 * @brief 初始化at指令列表
 * @param at_cmd_admin 指令列表管理变量
 * @param at_cmd_table 指令表
 * @param number 指令数量
 * @return 初始化结果
 */
at_cmd_ret_t at_cmd_init(at_cmd_admin_t* at_cmd_admin, at_cmd_t* at_cmd_list)
{
	at_cmd_admin->list = at_cmd_list;
	at_cmd_admin->number = at_cmd_get_list_number(at_cmd_list);
	at_cmd_param_default(at_cmd_admin);
	return AT_CMD_OK;
}

/**
 * @brief 解析AT指令
 * @param at_cmd_admin 指令列表管理变量
 * @param buffer 需要解析的字符串
 * @param len 输入字符串大小
 * @return 解析结果@__AT_CMD_RET
 */
at_cmd_ret_t at_cmd_handle(at_cmd_admin_t* at_cmd_admin, char* buffer, size_t len)
{
	enum
	{
		ATCMD_STEP_RES_NAME = 0,//解析指令名称
		ATCMD_STEP_RES_PARAM,//解析指令参数
		ATCMD_STEP_RES_CRLF,//解析指令结束符
	};
	size_t byte = 0;//用于实际解析的字节数统计
	at_cmd_t* cur_atcmd = NULL;
	at_cmd_ret_t ret = AT_CMD_ERR;
	AT_CMD_ASSERT(at_cmd_admin && buffer && (len && (len <= AT_CMD_LEN_MAX)));
	char pbuffer[AT_CMD_LEN_MAX + 1] = { 0 };
	int step = ATCMD_STEP_RES_NAME;
	char* param = NULL;
	char* crlf = NULL;
	AT_CMD_ASSERT(memcpy(pbuffer, buffer, sizeof(char) * len));
	pbuffer[len] = '\0';//确保字符串结束符
#if AT_CMD_ESCAPE_SUPPORT //将指令字符串进行转移操作
	AT_CMD_ASSERT((len = at_cmd_str2escape(buffer, pbuffer, len, sizeof(pbuffer))) >= 0);//进行转义字符解析，结果保存在pbuffer
#endif
	do{
		switch (step)
		{
			case ATCMD_STEP_RES_NAME:
			{
				size_t name_size = 0;
				char* cmd_name = NULL;
				cmd_name = strstr(pbuffer, "=");//若存在参数
				param = at_cmd_strstr(pbuffer, AT_CMD_CRLF);
				AT_CMD_ASSERT(cmd_name || param);//若同时为NULL则说明什么都没找到
				if (cmd_name)//具备参数
				{
					name_size = cmd_name - pbuffer;//计算指令名称长度
					step = ATCMD_STEP_RES_PARAM;//具备参数
					byte += 1;//加上'='
				}
				else if(param)//没有参数
				{
					name_size = param - pbuffer;//计算指令名称长度
					crlf = pbuffer + name_size;//计算CRLF位置
					step = ATCMD_STEP_RES_CRLF;//没有参数
				}
				byte += name_size;
				cmd_name = pbuffer;//回到指令名称首地址
#if AT_CMD_NAME_DXX_SUPPORT//指令名称支持大小写字母混合
				at_cmd_str2upper(cmd_name, name_size);
#endif
				//查询指令表，判断指令名称是否合法
				AT_CMD_ASSERT(cur_atcmd = at_cmd_find(at_cmd_admin, cmd_name, name_size));
			}break;
			case ATCMD_STEP_RES_PARAM://分解参数
			{
				int cnt = 0;
				int offset = 0;
				char* escape_chr = NULL;
				size_t empty_cnt = 0;
				char* crlf_end = at_cmd_strstr(pbuffer, AT_CMD_CRLF);
				AT_CMD_ASSERT(crlf_end != (pbuffer + byte));
				do{
					param = pbuffer + byte + cnt;
					if (((',' == param[0]) && ('\\' != param[-1])) || (crlf_end == param))
					{
						if (crlf_end == param)//处理最后一个参数
						{
							memcpy(buffer, pbuffer + byte, cnt);
							buffer[cnt] = '\0';
							at_cmd_admin->param.list[at_cmd_admin->param.number] = buffer;//记录位置
							byte += strlen(buffer);
							escape_chr = buffer + (param - escape_chr - 1);
						}
						else//非最后一个参数
						{
							param[0] = '\0';
							at_cmd_admin->param.list[at_cmd_admin->param.number] = param - cnt;//记录位置
							byte += (strlen(param - cnt) + 1);//加1记录逗号
						}
						if (0 == strcmp(param - cnt, ""))//空参数
						{
							at_cmd_admin->param.list[at_cmd_admin->param.number] = NULL;//当前无参数
							empty_cnt++;
						}
						at_cmd_admin->param.number++;
						cnt = 0;
					}
					else
					{
						cnt++;
						if ((',' == param[0]) && ('\\' == param[-1]))
						{
							escape_chr = param - 1;
							memmove(escape_chr, escape_chr + 1, (crlf_end - escape_chr));
							offset += 1;
							cnt--;
						}
					}
				}while (crlf_end != param);//若相等则表示处理完成
				AT_CMD_ASSERT(empty_cnt != at_cmd_admin->param.number);//判断是否全为空参数
				crlf = pbuffer + byte;
				byte += offset;
				step = ATCMD_STEP_RES_CRLF;
			}break;
			case ATCMD_STEP_RES_CRLF:
			{
				if (0 == strcmp(crlf, AT_CMD_CRLF))
				{
					byte += strlen(AT_CMD_CRLF);//加上CRLF
					ret = AT_CMD_OK;
				}
			}goto end;//结束
			default: 
				goto end;
		}
	} while (1);
end:
	if (at_cmd_admin)
	{
		if (len && (byte == len) && (AT_CMD_OK == ret))
		{
			if (cur_atcmd && cur_atcmd->cb)
			{
				cur_atcmd->cb(at_cmd_admin->param.number, at_cmd_admin->param.list);
				at_cmd_param_default(at_cmd_admin);
			}
			return AT_CMD_OK;
		}
		at_cmd_param_default(at_cmd_admin);
	}
	return AT_CMD_ERR;
}
