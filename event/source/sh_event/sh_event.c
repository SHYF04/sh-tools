#include "sh_event.h"
#include <string.h>

static sh_event_admin_t sh_event_admin;
static sh_event_t* sh_event_handle2event_find(sh_event_u16_t handle);
static sh_event_size_t sh_event_handle2index_find(sh_event_u16_t handle);
static sh_event_handle_t sh_event_handle_alloc(void);
static sh_event_bool_t sh_event_handle_free(sh_event_handle_t handle);

static sh_event_handle_t sh_event_handle_alloc(void)
{
	for (sh_event_u32_t i = 0; i < SH_EVENT_HANDLE_BITMAP_SIZE; i++)
	{
		if (sh_event_admin.handle_bitmap[i] != 0xFFFFFFFF)
		{
			sh_event_u32_t max_bit = (i == SH_EVENT_HANDLE_BITMAP_SIZE - 1) ?
				(SH_EVENT_NUM_MAX - i * 32) : 32;
			for (sh_event_u8_t j = 0; j < max_bit; j++)
			{
				if ((sh_event_admin.handle_bitmap[i] & (1 << j)) == 0)
				{
					sh_event_admin.handle_bitmap[i] |= (1 << j);
					return (i * 32 + j + 1);
				}
			}
		}
	}
	return 0;
}

static sh_event_bool_t sh_event_handle_free(sh_event_handle_t handle)
{
	if (handle == 0 || handle > SH_EVENT_NUM_MAX)
	{
		return SH_EVENT_FALSE;
	}
	sh_event_u32_t index = (handle - 1) / 32;
	sh_event_u8_t bit = (handle - 1) % 32;
	if ((sh_event_admin.handle_bitmap[index] & (1 << bit)) == 0)
	{
		return SH_EVENT_FALSE;
	}
	sh_event_admin.handle_bitmap[index] &= ~(1 << bit);
	return SH_EVENT_TRUE;
}

static sh_event_t* sh_event_handle2event_find(sh_event_u16_t handle)
{
	SH_EVENT_ASSERT_RET(handle, NULL);
	for (sh_event_size_t i = 0; i < sh_event_admin._register.number; i++)
	{
		if (sh_event_admin._register.event[i].handle == handle)
		{
			return (sh_event_admin._register.event + i);
		}
	}
	return NULL;
}

static sh_event_size_t sh_event_handle2index_find(sh_event_u16_t handle)
{
	SH_EVENT_ASSERT_RET(handle, SH_EVENT_NUM_MAX);
	for (sh_event_size_t i = 0; i < sh_event_admin._register.number; i++)
	{
		if (sh_event_admin._register.event[i].handle == handle)
		{
			return (i);
		}
	}
	return SH_EVENT_NUM_MAX;
}

#if (SH_EVENT_TIMER_SUPPORT)
void sh_event_tick_inc(sh_event_tick_t tick)
{
	sh_event_admin.tick += tick;
}
#endif

#if (SH_EVENT_TIMER_SUPPORT)
sh_event_tick_t sh_event_tick_get(void)
{
	return (sh_event_admin.tick);
}
#endif

void sh_event_set(sh_event_handle_t handle)
{
	sh_event_t* event = sh_event_handle2event_find(handle);
	SH_EVENT_ASSERT(event);
	SH_EVENT_ASSERT(SH_EVENT == event->mode);
	event->type.normal.signal = 1;
}

void sh_event_clear(sh_event_handle_t handle)
{
	sh_event_t* event = sh_event_handle2event_find(handle);
	SH_EVENT_ASSERT(event);
	SH_EVENT_ASSERT(SH_EVENT == event->mode);
	event->type.normal.signal = 0;
}

void sh_event_scheduler(void)
{
	SH_EVENT_ASSERT(sh_event_admin.activate);//未初始化成功则不启动调度器
	SH_EVENT_ASSERT(sh_event_admin._register.number <= SH_EVENT_NUM_MAX);
	//中断安全操作执行

	if (sh_event_admin._register.number)//判断是否有事件触发
	{
		sh_event_t* event_temp = 0;
		sh_event_size_t event_num = 0;
		sh_event_t* event_exe[SH_EVENT_NUM_MAX] = { 0 };
		for (sh_event_size_t i = 0; i < sh_event_admin._register.number; i++)//整理需要执行的event
		{
			switch (sh_event_admin._register.event[i].mode)
			{
#if (SH_EVENT_TIMER_SUPPORT)
			case SH_EVENT_TIMER:
			{
				if ((sh_event_tick_get() - sh_event_admin._register.event[i].type.timer.offset) >= sh_event_admin._register.event[i].type.timer.cycle)
				{
					SH_EVENT_ASSERT_GOON(sh_event_admin._register.event + i);
					sh_event_admin._register.event[i].type.timer.offset = sh_event_tick_get();
					event_exe[event_num] = sh_event_admin._register.event + i;
					event_num++;
				}
			}break;
#endif
			case SH_EVENT:
			{
				if (sh_event_admin._register.event[i].type.normal.signal)
				{
					SH_EVENT_ASSERT_GOON(sh_event_admin._register.event + i);
					event_exe[event_num] = sh_event_admin._register.event + i;
					event_num++;
				}
			}break;
			default:
				break;
			}
		}
		if (event_num)
		{
			//进行优先级排列——冒泡排序
			for (sh_event_size_t j = 0; j < (event_num - 1); j++)
			{
				for (sh_event_size_t i = 0; i < (event_num - 1) - j; i++)
				{
					if (event_exe[i]->priority == event_exe[i + 1]->priority)
					{
						if (event_exe[i]->mode != event_exe[i + 1]->mode)//若前后优先级相同则timer event优先
						{
							if (SH_EVENT_TIMER_PRIORITY == event_exe[i + 1]->mode)//若后排的是定时器事件那么则先执行
							{
								event_temp = event_exe[i];
								event_exe[i] = event_exe[i + 1];
								event_exe[i + 1] = event_temp;
							}
						}
					}
					else//优先级不同进行交换
					{
						if (event_exe[i]->priority > event_exe[i + 1]->priority)//数字越小优先级越高
						{
							event_temp = event_exe[i];
							event_exe[i] = event_exe[i + 1];
							event_exe[i + 1] = event_temp;
						}
					}
				}
			}
			for (sh_event_size_t i = 0; i < event_num; i++)//开始执行
			{
				if (event_exe[i]->cb)
				{
					event_exe[i]->cb(event_exe[i]->handle, event_exe[i]->param.data, event_exe[i]->param.len);
				}
			}
		}
	}
}

sh_event_bool_t sh_event_register(sh_event_handle_t* handle, sh_event_u8_t priority,
	sh_event_cb_t cb, void* data, sh_event_size_t len)
{
	SH_EVENT_ASSERT_RET(handle, SH_EVENT_FALSE);
	SH_EVENT_ASSERT_RET(sh_event_admin._register.number < SH_EVENT_NUM_MAX, SH_EVENT_FALSE);
	SH_EVENT_ASSERT_RET(priority < SH_EVENT_PRIORITY_MAX, SH_EVENT_FALSE);

	sh_event_handle_t handle_alloc = sh_event_handle_alloc();
	SH_EVENT_ASSERT_RET(handle_alloc, SH_EVENT_FALSE);
	*handle = handle_alloc;//输出handle

	sh_event_admin._register.event[sh_event_admin._register.number].mode = SH_EVENT;
	sh_event_admin._register.event[sh_event_admin._register.number].type.normal.signal = 0;
	sh_event_admin._register.event[sh_event_admin._register.number].priority = priority;
	sh_event_admin._register.event[sh_event_admin._register.number].cb = cb;
	sh_event_admin._register.event[sh_event_admin._register.number].param.data = data;
	sh_event_admin._register.event[sh_event_admin._register.number].param.len = len;
	sh_event_admin._register.event[sh_event_admin._register.number].handle = *handle;
	sh_event_admin._register.number++;

	return SH_EVENT_TRUE;
}

#if (SH_EVENT_TIMER_SUPPORT)
sh_event_bool_t sh_event_timer_register(sh_event_handle_t* handle, sh_event_u8_t priority,
	sh_event_size_t cycle, sh_event_cb_t cb, void* data, sh_event_size_t len)
{
	SH_EVENT_ASSERT_RET(handle, SH_EVENT_FALSE);
	SH_EVENT_ASSERT_RET(sh_event_admin._register.number < SH_EVENT_NUM_MAX, SH_EVENT_FALSE);
	SH_EVENT_ASSERT_RET(priority < SH_EVENT_PRIORITY_MAX, SH_EVENT_FALSE);

	sh_event_handle_t handle_alloc = sh_event_handle_alloc();
	SH_EVENT_ASSERT_RET(handle_alloc, SH_EVENT_FALSE);
	*handle = handle_alloc;//输出handle

	sh_event_admin._register.event[sh_event_admin._register.number].mode = SH_EVENT_TIMER;
	sh_event_admin._register.event[sh_event_admin._register.number].type.timer.offset = 0;
	sh_event_admin._register.event[sh_event_admin._register.number].type.timer.cycle = cycle;
	sh_event_admin._register.event[sh_event_admin._register.number].priority = priority;
	sh_event_admin._register.event[sh_event_admin._register.number].cb = cb;
	sh_event_admin._register.event[sh_event_admin._register.number].param.data = data;
	sh_event_admin._register.event[sh_event_admin._register.number].param.len = len;
	sh_event_admin._register.event[sh_event_admin._register.number].handle = *handle;
	sh_event_admin._register.number++;

	return SH_EVENT_TRUE;
}
#endif

sh_event_bool_t sh_event_unregister(sh_event_handle_t* handle)
{
	SH_EVENT_ASSERT_RET(handle, SH_EVENT_FALSE);
	SH_EVENT_ASSERT_RET(*handle, SH_EVENT_FALSE);
	SH_EVENT_ASSERT_RET(sh_event_admin._register.number > 0, SH_EVENT_FALSE);

	sh_event_t* event = sh_event_handle2event_find(*handle);
	SH_EVENT_ASSERT_RET(event, SH_EVENT_FALSE);

	sh_event_size_t event_index = sh_event_handle2index_find(*handle);
	SH_EVENT_ASSERT_RET(event_index < sh_event_admin._register.number, SH_EVENT_FALSE);

	memset(event, 0x0, sizeof(sh_event_t));
	sh_event_size_t i = 0;
	for (i = event_index; i < (sh_event_admin._register.number - 1); i++)
	{
		memcpy(&sh_event_admin._register.event[i], &sh_event_admin._register.event[i + 1], sizeof(sh_event_t));
	}
	memset(&sh_event_admin._register.event[i], 0x0, sizeof(sh_event_t));
	sh_event_admin._register.number--;

	SH_EVENT_ASSERT_RET(sh_event_handle_free(*handle), SH_EVENT_FALSE);
	*handle = 0;//将handle清除
	return SH_EVENT_TRUE;
}

sh_event_bool_t sh_event_init(void)
{
	SH_EVENT_ASSERT_RET(SH_EVENT_NUM_MAX <= (sh_event_handle_t)-1, SH_EVENT_FALSE);
	memset(&sh_event_admin, 0x0, sizeof(sh_event_admin_t));
	sh_event_admin.activate = SH_EVENT_TRUE;
	return sh_event_admin.activate;
}