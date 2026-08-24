
suppressPackageStartupMessages({
  if (!requireNamespace("dplyr", quietly = TRUE)) install.packages("dplyr", repos = "https://cloud.r-project.org")
  if (!requireNamespace("tidyr", quietly = TRUE)) install.packages("tidyr", repos = "https://cloud.r-project.org")
  if (!requireNamespace("readr", quietly = TRUE)) install.packages("readr", repos = "https://cloud.r-project.org")
  library(dplyr)
  library(tidyr)
  library(readr)
})

run_significance_tests <- function(df, baseline = "Binned-SAH (Serial)", output_dir = "benchmarks/analysis/results") {
  dir.create(output_dir, showWarnings = FALSE, recursive = TRUE)

  scenes <- unique(df$scene)
  ray_sets <- unique(df$ray_set)
  variants <- setdiff(unique(df$as_variant), baseline)

  results <- list()

  for (sc in scenes) {
    for (rs in ray_sets) {
      subset_df <- df %>% filter(scene == sc, ray_set == rs)

      base_data <- subset_df %>% filter(as_variant == baseline) %>% arrange(rep_id)
      if (nrow(base_data) == 0) {

        base_variant_inst <- "TLAS (Instanced BVH)"
        base_data <- subset_df %>% filter(as_variant == base_variant_inst) %>% arrange(rep_id)
        if (nrow(base_data) == 0) next
        current_baseline <- base_variant_inst
      } else {
        current_baseline <- baseline
      }

      for (cand in setdiff(unique(subset_df$as_variant), current_baseline)) {
        cand_data <- subset_df %>% filter(as_variant == cand) %>% arrange(rep_id)

        if (nrow(cand_data) == nrow(base_data) && nrow(base_data) >= 3) {

          w_query <- wilcox.test(cand_data$mrays_per_sec, base_data$mrays_per_sec, paired = TRUE, exact = FALSE)

          w_build <- wilcox.test(cand_data$build_time_ms, base_data$build_time_ms, paired = TRUE, exact = FALSE)

          results[[length(results) + 1]] <- data.frame(
            scene = as.character(sc),
            ray_set = as.character(rs),
            baseline = current_baseline,
            candidate = as.character(cand),
            metric = "mrays_per_sec",
            baseline_median = median(base_data$mrays_per_sec),
            candidate_median = median(cand_data$mrays_per_sec),
            speedup = median(cand_data$mrays_per_sec) / median(base_data$mrays_per_sec),
            raw_p_value = w_query$p.value,
            stringsAsFactors = FALSE
          )

          results[[length(results) + 1]] <- data.frame(
            scene = as.character(sc),
            ray_set = as.character(rs),
            baseline = current_baseline,
            candidate = as.character(cand),
            metric = "build_time_ms",
            baseline_median = median(base_data$build_time_ms),
            candidate_median = median(cand_data$build_time_ms),
            speedup = median(base_data$build_time_ms) / median(cand_data$build_time_ms),
            raw_p_value = w_build$p.value,
            stringsAsFactors = FALSE
          )
        }
      }
    }
  }

  if (length(results) == 0) {
    warning("No paired test comparisons could be formed.")
    return(data.frame())
  }

  test_df <- bind_rows(results)

  test_df$p_adj_bh <- p.adjust(test_df$raw_p_value, method = "BH")
  test_df$is_significant <- test_df$p_adj_bh < 0.05

  out_csv <- file.path(output_dir, "significance_summary.csv")
  write_csv(test_df, out_csv)
  message("Saved paired significance test summary to: ", out_csv)

  return(test_df)
}
