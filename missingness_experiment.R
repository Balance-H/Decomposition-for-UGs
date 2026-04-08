# Run missingness vs F_norm experiment
RUN_EXPERIMENTS <- FALSE
source("main_precision.r")

library(dplyr)
library(ggplot2)


res_lst <- lapply(seq_along(des_lst), function(i) {
  train_network(des_lst[[i]][1, ], folder_path,  global_args) #data_lst_local,
})


all_df <- do.call(rbind, res_lst)

result <- all_df %>%
  group_by(network, mask_fraction,  method, dat, approach) %>%
  summarise(
    F_norm     = round(mean(sigma_diff, na.rm = TRUE), 6), 
    n_reps     = n(),
    .groups    = 'drop'
  ) %>%
  arrange(mask_fraction, network, dat, method, approach)

options(pillar.sigfig = 7)
print(result)

write.csv(result, "result_sorted_by_mask.csv", row.names = FALSE)
