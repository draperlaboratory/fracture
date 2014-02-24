/*
 * pidController_step.c
 *
 * Code generation for function 'pidController_step'
 *
 * C source code generated on: Mon May 06 15:52:14 2013
 *
 */

/* Include files */
#include "rt_nonfinite.h"
#include "pidController_step.h"
#include "pidController_step_data.h"

/* Type Definitions */

/* Named Constants */

/* Variable Declarations */

/* Variable Definitions */

/* Function Declarations */

/* Function Definitions */
real_T pidController_step(real_T errVal)
{
  real_T out;

  /* ************************************************************** */
  /*  Draper Laboratory */
  /*  555 Technology Square */
  /*  Cambridge, MA 02139 */
  /*  */
  /*  Class name  : pidController */
  /*  Method name : step */
  /*  Created on  : 5/6/2013 3:15:37 PM */
  /*  Author      : pam2573 */
  /*  */
  /* ************************************************************** */
  /*  IO list [Kind, Name, Type, Notes]  */
  /*  out, out, double, PID controller output */
  /*  in, errVal, ,  */
  /*  End IO List */
  /* ************************************************************** */
  /*  PID controller step function */
  /*  Integrator state */
  pid.xi += errVal;

  /*  Output update */
  out = (pid.p.Kp * errVal + pid.p.Kd * (errVal - pid.xd)) + pid.p.Ki * pid.xi;

  /*  Derivative state update */
  pid.xd = errVal;

  /*  End of function: pidController_step */
  return out;
}

/* End of code generation (pidController_step.c) */
