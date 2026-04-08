#' @useDynLib gRips
#'
#' @importFrom Rcpp evalCpp
#'
#' @importFrom stats cov cov2cor logLik cov.wt AIC BIC coef nobs rnorm sigma
#'
#' @importMethodsFrom stats4 plot
#' @exportMethod plot
#'
#' @importFrom methods as
#'
#' @importFrom utils str
#'
#' @importFrom gRbase colmat2list rowmat2list combn_prim
#'     which.arr.index names2pairs is_inset
#'
#' @importFrom glasso glasso
#'
#' @importFrom MASS ginv
#'
#' @importFrom Matrix solve
#' 
#' @importFrom magrittr "%>%"
#' @export "%>%"
#' 
## #' @importMethodsFrom Rgraphviz plot
#'
#' @importFrom igraph erdos.renyi.game get.edgelist coreness
#'
#' @importFrom glue glue
#' 
NULL
