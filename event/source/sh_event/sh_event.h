#ifndef __SH_EVENT_H__
#define __SH_EVENT_H__

#include <stdint.h>
#include <stdbool.h>

#define SH_EVENT_NUM_MAX			16		//最大事件个数-根据资源调整-不允许 > (sh_event_handle_t)-1
#define SH_EVENT_PRIORITY_MAX		32		//优先级最大级数

#define SH_EVENT_TIMER_SUPPORT	1			//0-不支持定时器事件 1-支持定时器事件

#if (SH_EVENT_TIMER_SUPPORT)
#define SH_EVENT_TIMER_PRIORITY		SH_EVENT_TIMER	//当定时器和普通事件优先级相同时：定时器事件 > 普通事件 @sh_event_mode_t
#else
#define SH_EVENT_TIMER_PRIORITY		SH_EVENT
#endif

#define SH_EVENT_ASSERT(expr)			do{ if(! (expr)) {return;} }while(0)
#define SH_EVENT_ASSERT_GOTO(expr, go)	do{ if(! (expr)) {goto go;} }while(0)
#define SH_EVENT_ASSERT_GOON(expr)		do{ if(! (expr)) {continue;} }while(0)
#define SH_EVENT_ASSERT_RET(expr, ret)	do{ if(! (expr)) {return (ret);} }while(0)

#define SH_EVENT_HANDLE_BITMAP_SIZE		((SH_EVENT_NUM_MAX + 31) / 32)

typedef uint8_t		sh_event_u8_t;
typedef uint16_t	sh_event_u16_t;
typedef uint32_t	sh_event_u32_t;
typedef uint32_t	sh_event_size_t;

typedef uint32_t		sh_event_tick_t;
typedef sh_event_u8_t	sh_event_handle_t;

typedef void (*sh_event_cb_t) (sh_event_handle_t handle, void* param, sh_event_size_t len);

typedef enum __SH_EVENT_BOOL
{
	SH_EVENT_FALSE = 0,
	SH_EVENT_TRUE
}sh_event_bool_t;

typedef enum __SH_EVENT_MODE
{
	SH_EVENT = 0,
#if (SH_EVENT_TIMER_SUPPORT)
	SH_EVENT_TIMER,
#endif
}sh_event_mode_t;

typedef struct __SH_EVENT
{
	sh_event_handle_t handle;		//当前event句柄
	sh_event_u8_t mode;				//模式
	sh_event_u8_t priority;			//优先级
	sh_event_cb_t cb;				//回调处理
	union
	{
	#if (SH_EVENT_TIMER_SUPPORT)
		struct
		{
			sh_event_size_t offset;	//上一次tick offset
			sh_event_size_t cycle;	//周期调用
		}timer;
	#endif
		struct
		{
			sh_event_u8_t signal;	//普通事件触发信号
		}normal;
	}type;
	struct
	{
		void* data;
		sh_event_size_t len;
	}param;
}sh_event_t;

typedef struct __SH_EVENT_ADMIN
{
	sh_event_bool_t activate;
#if (SH_EVENT_TIMER_SUPPORT)
	sh_event_tick_t tick;
#endif
	sh_event_u32_t handle_bitmap[SH_EVENT_HANDLE_BITMAP_SIZE];	//handle位图-用于生成唯一handle

	struct
	{
		sh_event_t event[SH_EVENT_NUM_MAX];
		sh_event_size_t number;
	}_register;		//注册列表

}sh_event_admin_t;
/**
 * @brief sh_event_init 初始化事件调度器及检查参数配置正确性
 *
 * @note 初始化在其他事件相关操作函数之前调用
 *
 * @param[in] none
 * @param[out] none
 * @return none
 */
sh_event_bool_t sh_event_init(void);

/**
 * @brief sh_event_scheduler 事件调度器处理函数
 *
 * @note 将其放在while大循环常调用即可
 *
 * @param[in] none
 * @param[out] none
 * @return none
 */
void sh_event_scheduler(void);


/**
 * @brief sh_event_set 设置触发普通事件
 *
 * @note 仅用于普通事件
 *
 * @param[in] handle 用于触发指定事件参数
 * @param[out] none
 * @return none
 */
void sh_event_set(sh_event_handle_t handle);
void sh_event_set_isr(sh_event_handle_t handle);

/**
 * @brief sh_event_clear 清除触发普通事件
 *
 * @note 仅用于普通事件
 *
 * @param[in] handle 用于清除触发指定事件参数
 * @param[out] none
 * @return none
 */
void sh_event_clear(sh_event_handle_t handle);
void sh_event_clear_isr(sh_event_handle_t handle);

/**
 * @brief sh_event_tick_get 获取定时器事件tick数
 *
 * @note 仅用于定时器事件
 *
 * @param[in]  none
 * @param[out] none
 * @return
 *	- sh_event_tick_t 获取定时器事件tick数
 */
#if (SH_EVENT_TIMER_SUPPORT)
sh_event_tick_t sh_event_tick_get(void);
#endif
/**
 * @brief sh_event_tick_inc 增加定时器事件tick数
 *
 * @note 仅用于定时器事件，在1ms定时器中常调用即可
 *
 * @param[in]  tick tick倍率
 * @param[out] none
 * @return
 *	- none
 */
#if (SH_EVENT_TIMER_SUPPORT)
void sh_event_tick_inc(sh_event_tick_t tick);
#endif

/**
 * @brief sh_event_unregister 注销指定事件，注销事件将添加进注销列表在调度器中统一注销
 *
 * @note 用于所有事件
 *
 * @param[in]
 *	- handle 注销指定事件
 * @param[out] none
 * @return
 *	- SH_EVENT_FALSE 失败
 *	- SH_EVENT_TRUE 成功
 */
sh_event_bool_t sh_event_unregister(sh_event_handle_t* handle);
sh_event_bool_t sh_event_unregister_isr(sh_event_handle_t* handle);

/**
 * @brief sh_event_register 注册普通事件
 *
 * @note 仅用于普通事件注册
 *
 * @param[in]
 *	- priority 事件优先级
 *	- cb 事件回调函数
 *	- data 事件参数
 *	- len 事件参数大小
 * @param[out]
 *	- handle 分配的handle
 * @return
 *	- SH_EVENT_FALSE 失败
 *	- SH_EVENT_TRUE 成功
 */
sh_event_bool_t sh_event_register(sh_event_handle_t* handle, sh_event_u8_t priority,
	sh_event_cb_t cb, void* data, sh_event_size_t len);
sh_event_bool_t sh_event_register_isr(sh_event_handle_t* handle, sh_event_u8_t priority,
	sh_event_cb_t cb, void* data, sh_event_size_t len);

/**
 * @brief sh_event_timer_register 注册定时器事件
 *
 * @note 仅用于定时器事件注册
 *
 * @param[in]
 *	- priority 事件优先级
 *	- cycle 定时触发周期 cycle
 *	- cb 事件回调函数
 *	- data 事件参数
 *	- len 事件参数大小
 * @param[out]
 *	- handle 分配的handle
 * @return
 *	- SH_EVENT_FALSE 失败
 *	- SH_EVENT_TRUE 成功
 */
#if (SH_EVENT_TIMER_SUPPORT)
sh_event_bool_t sh_event_timer_register(sh_event_handle_t* handle, sh_event_u8_t priority,
	sh_event_size_t cycle, sh_event_cb_t cb, void* data, sh_event_size_t len);
sh_event_bool_t sh_event_timer_register_isr(sh_event_handle_t* handle, sh_event_u8_t priority,
	sh_event_size_t cycle, sh_event_cb_t cb, void* data, sh_event_size_t len);
#endif

#endif