#ifndef __PID_H
#define __PID_H
#include <stdbool.h>
#include "config_param.h"

/********************************************************************************
 * PID驱动代码
 ********************************************************************************/
#define DEFAULT_PID_INTEGRATION_LIMIT 500.0 // 默认pid的积分限幅
#define DEFAULT_PID_OUTPUT_LIMIT 0.0				// 默认pid输出限幅，0为不限幅

typedef struct
{
	float desired;		 /*期望值*/
	float error;			 /*当前误差*/
	float prevError;	 /*上一次误差*/
	float integ;			 /*积分累计*/
	float deriv;			 /*微分值*/
	float kp;					 /*p增益*/
	float ki;					 /*i增益*/
	float kd;					 /*d增益*/
	float outP;				 /*p输出*/
	float outI;				 /*i输出*/
	float outD;				 /*d输出*/
	float iLimit;			 /*积分限幅,防止积分过大*/
	float outputLimit; /*总输出限幅,防止控制量过大*/
	float dt;					 /*采样周期dt*/
	float out;				 /*最终输出*/
} PidObject;

/*pid结构体初始化*/
void pidInit(PidObject *pid, const float desired, const pidInit_t pidParam, const float dt);
void pidSetIntegralLimit(PidObject *pid, const float limit); /*pid积分限幅设置*/
void pidSetOutputLimit(PidObject *pid, const float limit);
void pidSetDesired(PidObject *pid, const float desired); /*pid设置期望值*/
float pidUpdate(PidObject *pid, const float error);			 /*pid更新*/
float pidGetDesired(PidObject *pid);										 /*pid获取期望值*/
bool pidIsActive(PidObject *pid);												 /*pid状态*/
void pidReset(PidObject *pid);													 /*pid结构体复位*/
void pidSetError(PidObject *pid, const float error);		 /*pid偏差设置*/
void pidSetKp(PidObject *pid, const float kp);					 /*pid Kp设置*/
void pidSetKi(PidObject *pid, const float ki);					 /*pid Ki设置*/
void pidSetKd(PidObject *pid, const float kd);					 /*pid Kd设置*/
void pidSetDt(PidObject *pid, const float dt);					 /*pid dt设置*/

#endif /* __PID_H */
