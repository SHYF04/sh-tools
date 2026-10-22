#include "at_cmd.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

at_cmd_log_t at_cmd_log = NULL;

#if AT_CMD_NAME_DXX_SUPPORT
static void at_cmd_str2upper(char* str, sh_size_t len)
{
	if (str && len)
	{
		for (sh_size_t i = 0; i < len; i++)
		{
			str[i] = toupper(str[i]);
		}
	}
}
#endif

#if AT_CMD_ESCAPE_SUPPORT
static int at_cmd_str2escape(const char* src, char* dst, sh_size_t src_size, sh_size_t dst_size)
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
			s++;
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
				case 'x':
				{
					s++;
					if (s >= end || !isxdigit(*s))
					{
						if (d + 1 < dst_end)
						{
							*d++ = '\\';
							*d++ = 'x';
							len += 2;
						}
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
				default:
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


static at_cmd_t* at_cmd_find(at_cmd_admin_t* at_cmd_admin, const char* name, sh_size_t len)
{
	AT_CMD_ASSERT(at_cmd_admin && name && len);
	for (sh_size_t i = 0; i < at_cmd_admin->number; i++)
	{
		sh_size_t index = 0;
		sh_size_t cmp_inc = 0;

		while(	('\0' != at_cmd_admin->list[i].name[index]) &&
				('\0' != name[index]) &&
				(index != len))
		{
			if(at_cmd_admin->list[i].name[index] == name[index])
			{
				cmp_inc++;
				if(	(cmp_inc == strlen(at_cmd_admin->list[i].name)) && 
					(len == cmp_inc))
				{
					return (&at_cmd_admin->list[i]);
				}
			}
			else
			{
				break;
			}
			index++;
		}
	}
end:
	return NULL;
}


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


static void at_cmd_param_default(at_cmd_admin_t* at_cmd_admin)
{
	memset(at_cmd_admin->param.list, 0, sizeof(char*) * AT_CMD_PARAM_NUM_MAX);
	at_cmd_admin->param.number = 0;
}


static sh_size_t at_cmd_get_list_number(at_cmd_t* at_cmd_list)
{
	sh_size_t number = 0;
	while (at_cmd_list[number].cb && at_cmd_list[number].name)
	{
		number++;
	}
	return number;
}

void at_cmd_log_register(at_cmd_log_t log_func)
{
	at_cmd_log = log_func;
}

at_cmd_ret_t at_cmd_init(at_cmd_admin_t* at_cmd_admin, at_cmd_t* at_cmd_list)
{
	at_cmd_admin->list = at_cmd_list;
	at_cmd_admin->number = at_cmd_get_list_number(at_cmd_list);
	at_cmd_param_default(at_cmd_admin);
	return AT_CMD_OK;
}


at_cmd_ret_t at_cmd_handle(at_cmd_admin_t* at_cmd_admin, char* buffer, sh_size_t len)
{
	enum ATCMD_STEP_TYPE
	{
		ATCMD_STEP_RES_NAME = 0,
		ATCMD_STEP_RES_PARAM,
		ATCMD_STEP_RES_CRLF,
	};
	sh_size_t byte = 0;
	at_cmd_t* cur_atcmd = NULL;
	at_cmd_ret_t ret = AT_CMD_ERR;
	AT_CMD_ASSERT(at_cmd_admin && buffer && (len && (len <= AT_CMD_LEN_MAX)));
	char pbuffer[AT_CMD_LEN_MAX + 1] = { 0 };
	enum ATCMD_STEP_TYPE step = ATCMD_STEP_RES_NAME;
	char* param = NULL;
	char* crlf = NULL;
	AT_CMD_ASSERT(memcpy(pbuffer, buffer, sizeof(char) * len));
	pbuffer[len] = '\0';
#if AT_CMD_ESCAPE_SUPPORT
	AT_CMD_ASSERT((len = at_cmd_str2escape(buffer, pbuffer, len, sizeof(pbuffer))) >= 0);
#endif
	do{
		switch (step)
		{
			case ATCMD_STEP_RES_NAME:
			{
				sh_size_t name_size = 0;
				char* cmd_name = NULL;
				cmd_name = strstr(pbuffer, "=");
				param = at_cmd_strstr(pbuffer, AT_CMD_CRLF);
				AT_CMD_ASSERT(cmd_name || param);
				if (cmd_name)
				{
					name_size = cmd_name - pbuffer;
					step = ATCMD_STEP_RES_PARAM;
					byte += 1;
				}
				else if(param)
				{
					name_size = param - pbuffer;
					crlf = pbuffer + name_size;
					step = ATCMD_STEP_RES_CRLF;
				}
				byte += name_size;
				cmd_name = pbuffer;
#if AT_CMD_NAME_DXX_SUPPORT
				at_cmd_str2upper(cmd_name, name_size);
#endif
				AT_CMD_ASSERT(cur_atcmd = at_cmd_find(at_cmd_admin, cmd_name, name_size));
			}break;
			case ATCMD_STEP_RES_PARAM:
			{
				int cnt = 0;
				int offset = 0;
				char* escape_chr = NULL;
				sh_size_t empty_cnt = 0;
				char* crlf_end = at_cmd_strstr(pbuffer, AT_CMD_CRLF);
				AT_CMD_ASSERT(crlf_end != (pbuffer + byte));
				do{
					param = pbuffer + byte + cnt;
					if (((',' == param[0]) && ('\\' != param[-1])) || (crlf_end == param))
					{
						if (crlf_end == param)
						{
							memcpy(buffer, pbuffer + byte, cnt);
							buffer[cnt] = '\0';
							at_cmd_admin->param.list[at_cmd_admin->param.number] = buffer;
							byte += strlen(buffer);
							escape_chr = buffer + (param - escape_chr - 1);
						}
						else
						{
							param[0] = '\0';
							at_cmd_admin->param.list[at_cmd_admin->param.number] = param - cnt;
							byte += (strlen(param - cnt) + 1);
						}
						if (0 == strcmp(param - cnt, ""))
						{
							at_cmd_admin->param.list[at_cmd_admin->param.number] = NULL;
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
				}while (crlf_end != param);
				AT_CMD_ASSERT(empty_cnt != at_cmd_admin->param.number);
				crlf = pbuffer + byte;
				byte += offset;
				step = ATCMD_STEP_RES_CRLF;
			}break;
			case ATCMD_STEP_RES_CRLF:
			{
				if (0 == strcmp(crlf, AT_CMD_CRLF))
				{
					byte += strlen(AT_CMD_CRLF);
					ret = AT_CMD_OK;
				}
			}goto end;
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
