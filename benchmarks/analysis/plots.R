# plots.R
# Generates publication-ready ggplot2 faceted comparison charts with IQR error bars.

suppressPackageStartupMessages({
  if (!requireNamespace("ggplot2", quietly = TRUE)) install.packages("ggplot2", repos = "https://cloud.r-project.org")
  if (!requireNamespace("dplyr", quietly = TRUE)) install.packages("dplyr", repos = "https://cloud.r-project.org")
  library(ggplot2)
  library(dplyr)
})

plot_throughput_comparison <- function(df) {
  summary_df <- df %>%
    group_by(scene, ray_set, as_variant) %>%
    summarise(
      median_mrays = median(mrays_per_sec),
      lo = quantile(mrays_per_sec, 0.25),
      hi = quantile(mrays_per_sec, 0.75),
      .groups = "drop"
    )

  ggplot(summary_df, aes(x = as_variant, y = median_mrays, fill = as_variant)) +
    geom_col(alpha = 0.85, width = 0.7) +
    geom_errorbar(aes(ymin = lo, ymax = hi), width = 0.25, linewidth = 0.8, color = "#222222") +
    facet_grid(ray_set ~ scene, scales = "free_y") +
    labs(
      title = "Ray Traversal Throughput by Acceleration Structure",
      subtitle = "Median throughput (MRay/s) across 5 runs with IQR (25th-75th percentile) error bars",
      y = "Throughput (MRay/s - Higher is Better)",
      x = NULL
    ) +
    theme_minimal(base_size = 12) +
    theme(
      legend.position = "none",
      axis.text.x = element_text(angle = 35, hjust = 1, vjust = 1, size = 9),
      strip.background = element_rect(fill = "#EFEFEF", color = NA),
      strip.text = element_text(face = "bold", size = 10),
      panel.grid.minor = element_blank()
    )
}

plot_build_time_comparison <- function(df) {
  summary_df <- df %>%
    distinct(scene, as_variant, rep_id, build_time_ms) %>%
    group_by(scene, as_variant) %>%
    summarise(
      median_build = median(build_time_ms),
      lo = quantile(build_time_ms, 0.25),
      hi = quantile(build_time_ms, 0.75),
      .groups = "drop"
    )

  ggplot(summary_df, aes(x = as_variant, y = median_build, fill = as_variant)) +
    geom_col(alpha = 0.85, width = 0.7) +
    geom_errorbar(aes(ymin = lo, ymax = hi), width = 0.25, linewidth = 0.8, color = "#222222") +
    facet_wrap(~ scene, scales = "free_y") +
    scale_y_log10() +
    labs(
      title = "BVH Construction Time (Log Scale)",
      subtitle = "Median construction wall-clock time (ms) with IQR error bars",
      y = "Build Time (ms - Lower is Better, Log10 Scale)",
      x = NULL
    ) +
    theme_minimal(base_size = 12) +
    theme(
      legend.position = "none",
      axis.text.x = element_text(angle = 35, hjust = 1, vjust = 1, size = 9),
      strip.background = element_rect(fill = "#EFEFEF", color = NA),
      strip.text = element_text(face = "bold", size = 10),
      panel.grid.minor = element_blank()
    )
}

plot_node_count_comparison <- function(df) {
  summary_df <- df %>%
    distinct(scene, as_variant, node_count, avg_fanout)

  ggplot(summary_df, aes(x = as_variant, y = node_count, fill = as_variant)) +
    geom_col(alpha = 0.85, width = 0.7) +
    geom_text(aes(label = paste0(round(avg_fanout, 1), "x")), vjust = -0.4, size = 3.2) +
    facet_wrap(~ scene, scales = "free_y") +
    labs(
      title = "Acceleration Structure Topology & Node Count",
      subtitle = "Total node count (bars) and average child fan-out per node (labels)",
      y = "Total Nodes (Lower is More Compact)",
      x = NULL
    ) +
    theme_minimal(base_size = 12) +
    theme(
      legend.position = "none",
      axis.text.x = element_text(angle = 35, hjust = 1, vjust = 1, size = 9),
      strip.background = element_rect(fill = "#EFEFEF", color = NA),
      strip.text = element_text(face = "bold", size = 10),
      panel.grid.minor = element_blank()
    )
}

generate_all_plots <- function(df, output_dir = "benchmarks/analysis/results") {
  dir.create(output_dir, showWarnings = FALSE, recursive = TRUE)

  p_throughput <- plot_throughput_comparison(df)
  ggsave(file.path(output_dir, "throughput_comparison.png"), p_throughput, width = 11, height = 7, dpi = 300)

  p_build <- plot_build_time_comparison(df)
  ggsave(file.path(output_dir, "build_time_comparison.png"), p_build, width = 10, height = 6, dpi = 300)

  p_nodes <- plot_node_count_comparison(df)
  ggsave(file.path(output_dir, "node_count_comparison.png"), p_nodes, width = 10, height = 6, dpi = 300)

  message("Generated all plots in: ", output_dir)
}
