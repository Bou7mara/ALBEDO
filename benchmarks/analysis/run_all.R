
script_dir <- tryCatch({
  dirname(sys.frame(1)$ofile)
}, error = function(e) {
  "benchmarks/analysis"
})

source(file.path(script_dir, "load_results.R"))
source(file.path(script_dir, "plots.R"))
source(file.path(script_dir, "significance_tests.R"))
source(file.path(script_dir, "sah_correlation.R"))
source(file.path(script_dir, "duplication_analysis.R"))

run_pipeline <- function(csv_path = NULL, output_dir = "benchmarks/analysis/results") {
  cat("\n=================================================================================\n")
  cat("               ALBEDO BENCHMARK STATISTICAL ANALYSIS PIPELINE                    \n")
  cat("=================================================================================\n\n")

  if (is.null(csv_path)) {
    csv_path <- find_latest_benchmark_csv("benchmark_results", type = "reps")
  }
  cat("Loading benchmark data from:", csv_path, "\n\n")

  df <- load_benchmark_results(csv_path)
  cat("Loaded", nrow(df), "benchmark records across", length(unique(df$scene)), "scenes and", length(unique(df$as_variant)), "AS variants.\n\n")

  cat("Generating ggplot2 comparative visualizations...\n")
  generate_all_plots(df, output_dir)
  cat("Visualizations written to:", output_dir, "\n\n")

  cat("Running paired Wilcoxon signed-rank tests (BH-adjusted)...\n")
  sig_summary <- run_significance_tests(df, baseline = "Binned-SAH (Serial)", output_dir = output_dir)
  cat("Significant comparisons (alpha = 0.05, BH-corrected):\n")
  if (nrow(sig_summary) > 0) {
    sig_subset <- sig_summary %>%
      filter(is_significant) %>%
      select(scene, ray_set, candidate, metric, speedup, p_adj_bh)
    print(as.data.frame(sig_subset))
  } else {
    cat("No comparisons met significance criteria.\n")
  }
  cat("\n")

  cat("Evaluating monotonic rank correlation...\n")
  corr_results <- run_sah_correlation(df, output_dir)
  cat("Spearman rank correlation rho:", round(corr_results$spearman_rho, 3), "\n\n")

  cat("=================================================================================\n")
  cat("Analysis complete! All artifacts saved to:", output_dir, "\n")
  cat("=================================================================================\n\n")
}

if (!interactive()) {
  run_pipeline()
}
