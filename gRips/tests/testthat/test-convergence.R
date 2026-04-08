context("gRips - convergence")

##############################

## Fishers iris data


SS <- structure(c(0.686, -0.042, 1.274, 0.516, -0.042, 0.19, -0.33, 
                  -0.122, 1.274, -0.33, 3.116, 1.296, 0.516, -0.122, 1.296, 0.581
                  ), .Dim = c(4L, 4L), .Dimnames = list(c("a", "b", "c", "d"), 
                                                        c("a", "b", "c", "d")))

KK <- structure(c(5.063, -2.098, 0, -4.946, -2.098, 7.532, 1.557, 0, 
                  0, 1.557, 4.796, -10.362, -4.946, 0, -10.362, 29.235),
                .Dim = c(4L, 
                         4L), .Dimnames = list(c("a", "b", "c", "d"),
                                               c("a", "b", "c", "d")))

Sig <- solve(KK)

EE <- structure(c(1L, 2L, 2L, 3L, 1L, 4L, 3L, 4L), .Dim = c(2L, 4L),
                .Dimnames = list(NULL, NULL))

## test_that("convergence", {
##     expect_equal(max_abs           (SS),                 max_abs_           (SS))
##     expect_equal(max_abs_diag      (SS),                 max_abs_diag_      (SS))
##     expect_equal(max_abs_diag_diff (SS, Sig),            max_abs_diag_diff_ (SS, Sig))
##     expect_equal(max_diag_diff     (SS, Sig),            max_diag_diff_     (SS, Sig))
##     expect_equal(max_abs_diff_on_emat (Sig, SS, EE),     max_abs_diff_on_emat_ (SS, Sig, EE))
##     expect_equal(max_diff_on_emat     (Sig, SS, EE),     max_diff_on_emat_     (SS, Sig, EE))
##     expect_equal(max_abs_diff_on_EK(Sig, SS, EE, KK),    max_abs_diff_on_EK_(SS, Sig, EE, KK))    
## }
##)


