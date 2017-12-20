#ifdef SWIG
%module i2tcl
%{
#include "i2tcl.hpp"
%}

%typemap(in,numinputs=0) Tcl_Interp *interp {
	$1 = interp;
}

#endif /* SWIG */

#include <tcl.h>

#ifndef I2TCL_H
#define I2TCL_H

void i2_register_command(const char *icmd, const char *tcmd, const char *mtype, Tcl_Interp *interp);
void *i2_new_frame(Tcl_Interp *interp);
void i2_free_frame(void *frame, Tcl_Interp *interp);
char *i2_eval(void *frame, const char *text, Tcl_Interp *interp);

#endif /* I2TCL_H */
