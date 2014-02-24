/*
 * pidController_step_initialize.c
 *
 * Code generation for function 'pidController_step_initialize'
 *
 * C source code generated on: Mon May 06 15:52:14 2013
 *
 */

/* Include files */
#include "rt_nonfinite.h"
#include "pidController_step.h"
#include "pidController_step_initialize.h"
#include "pidController.h"
#include "pidController_step_data.h"

/* Type Definitions */

/* Named Constants */

/* Variable Declarations */

/* Variable Definitions */

/* Function Declarations */

/* Function Definitions */
void pidController_step_initialize(void)
{
  rt_InitInfAndNaN(8U);
  pidController(&pid.p.Kd, &pid.p.Kp, &pid.p.Ki, &pid.xd, &pid.xi);
}

/* End of code generation (pidController_step_initialize.c) */
