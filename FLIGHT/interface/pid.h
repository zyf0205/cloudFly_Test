#ifndef __PID_H
#define __PID_H
#include <stdbool.h>
#include "config_param.h"

/********************************************************************************
 * PID��������
 ********************************************************************************/
#define DEFAULT_PID_INTEGRATION_LIMIT 500.0 // Ĭ��pid�Ļ����޷�
#define DEFAULT_PID_OUTPUT_LIMIT 0.0				// Ĭ��pid����޷���0Ϊ���޷�

typedef struct
{
	float desired;		 /*����ֵ*/
	float error;			 /*��ǰ���*/
	float prevError;	 /*��һ�����*/
	float integ;			 /*�����ۼ�*/
	float deriv;			 /*΢��ֵ*/
	float kp;					 /*p����*/
	float ki;					 /*i����*/
	float kd;					 /*d����*/
	float outP;				 /*p���*/
	float outI;				 /*i���*/
	float outD;				 /*d���*/
	float iLimit;			 /*�����޷�,��ֹ���ֹ���*/
	float outputLimit; /*������޷�,��ֹ����������*/
	float dt;					 /*��������dt*/
	float out;				 /*�������*/
} PidObject;

/*pid�ṹ���ʼ��*/
void pidInit(PidObject *pid, const float desired, const pidInit_t pidParam, const float dt);
void pidSetIntegralLimit(PidObject *pid, const float limit); /*pid�����޷�����*/
void pidSetOutputLimit(PidObject *pid, const float limit);
void pidSetDesired(PidObject *pid, const float desired); /*pid��������ֵ*/
float pidUpdate(PidObject *pid, const float error);			 /*pid����*/
float pidGetDesired(PidObject *pid);										 /*pid��ȡ����ֵ*/
bool pidIsActive(PidObject *pid);												 /*pid״̬*/
void pidReset(PidObject *pid);													 /*pid�ṹ�帴λ*/
void pidSetError(PidObject *pid, const float error);		 /*pidƫ������*/
void pidSetKp(PidObject *pid, const float kp);					 /*pid Kp����*/
void pidSetKi(PidObject *pid, const float ki);					 /*pid Ki����*/
void pidSetKd(PidObject *pid, const float kd);					 /*pid Kd����*/
void pidSetDt(PidObject *pid, const float dt);					 /*pid dt����*/

#endif /* __PID_H */
