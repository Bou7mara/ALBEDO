# load_results.R
# Loads and validates raw benchmark CSV outputs from the ALBEDO benchmark harness.

suppressPackageStartupMessages({
  if (!requireNamespace("readr", quietly = TRUE)) install.packages("readr", repos = "https://cloud.r-project.org")
  if (!requireNamespace("dplyr", quietly = TRUE)) install.packages("dplyr", repos = "https://cloud.r-project.org")
  library(readr)
  library(dplyr)
})

load_benchmark_results <- function(csv_path) {
  if (!file.exists(csv_path)) {
    stop(paste("Benchmark CSV file does not exist:", csv_path))
  }
  
  df <- read_csv(csv_path, show_col_types = FALSE)
  
  # Integrity assertions
  stopifnot(
    "build_time_ms column missing" = "build_time_ms" %in% colnames(df),
    "query_time_ms or rays_per_sec missing" = ("query_time_ms" %in% colnames(df) || "mrays_per_sec" %in% colnames(df)),
    "All build times must be positive" = all(df$build_time_ms > 0),
    "All query times must be positive" = all(df$query_time_ms > 0),
    "Zero NAs allowed in as_variant" = !anyNA(df$as_variant)
  )
  
  # Standardize factor levels for consistent chart ordering
  variant_levels <- c(
    "Binned-SAH (Serial)",
    "Binned-SAH (Parallel)",
    "Wide BVH4 (4-wide SIMD)",
    "Wide BVH8 (8-wide AVX2)",
    "TLAS (Instanced BVH)",
    "Flat BVH4 (Over Instances)",
    "SBVH (Spatial Splits)"
  )
  
  df <- df %>%
    mutate(
      as_variant = factor(as_variant, levels = intersect(variant_levels, unique(as_variant))),
      scene = factor(scene),
      ray_set = factor(ray_set)
    )
  
  return(df)
}

find_latest_benchmark_csv <- function(directory = "benchmark_results", type = "reps") {
  pattern <- paste0("_", type, "\\.csv$")
  files <- list.files(directory, pattern = pattern, full.names = TRUE)
  if (length(files) == 0) {
    # Fallback to any evaluation CSV
    files <- list.files(directory, pattern = "as_evaluation_.*\\.csv$", full.names = TRUE)
  }
  if (length(files) == 0) {
    stop(paste("No benchmark CSV files found in:", directory))
  }
  # Return the most recently modified file
  files[order(file.info(files)$mtime, decreasing = TRUE)][1]
}
