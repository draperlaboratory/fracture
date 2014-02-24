/*
 * pidController_step_types.h
 *
 * Code generation for function 'pidController_step'
 *
 * C source code generated on: Mon May 06 15:52:14 2013
 *
 */

#ifndef __PIDCONTROLLER_STEP_TYPES_H__
#define __PIDCONTROLLER_STEP_TYPES_H__

/* Type Definitions */
#ifndef typedef_pidParam
#define typedef_pidParam
typedef struct
{
    real_T Kd;
    real_T Kp;
    real_T Ki;
} pidParam;
#endif /*typedef_pidParam*/
#ifndef typedef_b_pidController
#define typedef_b_pidController
typedef struct
{
    pidParam p;
    real_T xd;
    real_T xi;
} b_pidController;
#endif /*typedef_b_pidController*/

#endif
/* End of code generation (pidController_step_types.h) */
