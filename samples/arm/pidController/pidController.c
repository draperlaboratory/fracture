/*
 * pidController.c
 *
 * Code generation for function 'pidController'
 *
 * C source code generated on: Mon May 06 15:52:14 2013
 *
 */

/* Include files */
#include "rt_nonfinite.h"
#include "pidController_step.h"
#include "pidController.h"

/* Type Definitions */

/* Named Constants */

/* Variable Declarations */

/* Variable Definitions */

/* Function Declarations */

/* Function Definitions */
void pidController(real_T *this_p_Kd, real_T *this_p_Kp, real_T *this_p_Ki,
                   real_T *this_xd, real_T *this_xi)
{
  /* ************************************************************** */
  /*  Draper Laboratory */
  /*  555 Technology Square */
  /*  Cambridge, MA 02139 */
  /*   */
  /*  Generated from Enterprise Architect */
  /*  Class name : pidController */
  /*  Created on : 5/6/2013 3:15:37 PM */
  /*  Author     : pam2573 */
  /*  */
  /* ************************************************************** */
  /*  ************************************************ */
  /*  Aggregation: pidParam */
  /*  Parameters */
  *this_p_Kd = 0.0;
  *this_p_Kp = 0.0;
  *this_p_Ki = 0.0;

  /*  ************************************************ */
  /*  Attributes from Current Class: pidController */
  /*  Derivative state []. */
  *this_xd = 0.0;

  /*  Integral state []. */
  *this_xi = 0.0;

  /*  Check if code need to be generated for OCTAVE or MATLAB */
  /*  Never inline constructor */
  /*  Specifies name to use for the C structure */
  /*  End of constructor: pidController */
}

/* End of code generation (pidController.c) */
