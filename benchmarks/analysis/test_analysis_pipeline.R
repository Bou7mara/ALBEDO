# test_analysis_pipeline.R
# Validates the statistical pipeline on synthetic data with known ground-truth effects.

source("benchmarks/analysis/load_results.R")
source("benchmarks/analysis/significance_tests.R")

test_synthetic_pipeline <- function() {
  cat("Running synthetic-data pipeline validation...\n")

  # 1. Synthesize data with a known real difference (Scene A) and known zero difference (Scene B)
  synthetic_data <- data.frame(
    scene = c(rep("Scene_A_Different", 10), rep("Scene_B_Identical", 10)),
    as_variant = rep(c(rep("Binned-SAH (Serial)", 5), rep("Wide BVH8 (8-wide AVX2)", 5)), 2),
    ray_set = "Primary (Coherent)",
    rep_id = rep(1:5, 4),
    primitives = 10000,
    ray_count = 80000,
    build_time_ms = c(rep(100, 5), rep(105, 5), rep(100, 5), rep(100, 5)),
    query_time_ms = c(rep(50, 5), rep(25, 5), rep(50, 5), rep(50, 5)),
    mrays_per_sec = c(c(1.6, 1.62, 1.58, 1.61, 1.59), c(3.2, 3.22, 3.18, 3.21, 3.19),
                      c(1.6, 1.61, 1.59, 1.60, 1.62), c(1.6, 1.61, 1.59, 1.60, 1.62)),
    memory_bytes = 100000,
    node_count = 5000,
    avg_fanout = c(rep(2.0, 5), rep(5.0, 5), rep(2.0, 5), rep(5.0, 5)),
    hits = 80000,
    stringsAsFactors = FALSE
  )

  tmp_csv <- tempfile(fileext = ".csv")
  write_csv(synthetic_data, tmp_csv)

  df <- load_benchmark_results(tmp_csv)
  res <- run_significance_tests(df, baseline = "Binned-SAH (Serial)", output_dir = tempdir())

  # Check that Scene A was detected as significantly faster and Scene B was not
  scene_a_query <- res %>% filter(scene == "Scene_A_Different", metric == "mrays_per_sec")
  scene_b_query <- res %>% filter(scene == "Scene_B_Identical", metric == "mrays_per_sec")

  stopifnot(
    "Scene A must show speedup ~ 2.0x" = abs(scene_a_query$speedup - 2.0) < 0.05,
    "Scene A query p-value must be < 0.05" = scene_a_query$raw_p_value < 0.05,
    "Scene B query speedup must be ~ 1.0x" = abs(scene_b_query$speedup - 1.0) < 0.05
  )

  unlink(tmp_csv)
  cat("Synthetic-data validation passed successfully!\n\n")
}

if (!interactive()) {
  test_synthetic_pipeline()
}
