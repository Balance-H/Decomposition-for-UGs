#include <R.h>
#include <Rinternals.h>
#include <stdlib.h> // for NULL
#include <R_ext/Rdynload.h>

/* FIXME: 
   Check these declarations against the C/Fortran source code.
*/

/* .Call calls */
extern SEXP _gRips_clone_(void *);
extern SEXP _gRips_conips_ggm_(void *, void *, void *, void *, void *, void *, void *, void *, void *, void *);
extern SEXP _gRips_covips_ggm_(void *, void *, void *, void *, void *, void *, void *, void *, void *, void *);
extern SEXP _gRips_ggm_logL_(void *, void *, void *);
extern SEXP _gRips_inv_qr_(void *);
extern SEXP _gRips_ncd_ggm_(void *, void *, void *, void *, void *, void *, void *, void *, void *, void *);
extern SEXP _gRips_RcppExport_registerCCallable(void);

static const R_CallMethodDef CallEntries[] = {
    {"_gRips_clone_",                       (DL_FUNC) &_gRips_clone_,                        1},
    {"_gRips_conips_ggm_",                  (DL_FUNC) &_gRips_conips_ggm_,                  10},
    {"_gRips_covips_ggm_",                  (DL_FUNC) &_gRips_covips_ggm_,                  10},
    {"_gRips_ggm_logL_",                    (DL_FUNC) &_gRips_ggm_logL_,                     3},
    {"_gRips_inv_qr_",                      (DL_FUNC) &_gRips_inv_qr_,                       1},
    {"_gRips_ncd_ggm_",                     (DL_FUNC) &_gRips_ncd_ggm_,                     10},
    {"_gRips_RcppExport_registerCCallable", (DL_FUNC) &_gRips_RcppExport_registerCCallable,  0},
    {NULL, NULL, 0}
};

void R_init_gRips(DllInfo *dll)
{
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}
