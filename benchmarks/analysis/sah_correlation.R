# sah_correlation.R
# Evaluates monotonic correlation between static tree metrics and measured traversal throughput.

suppressPackageStartupMessages({
  if (!requireNamespace("ggplot2", quietly = TRUE)) install.packages("ggplot2", repos = "https://cloud.r-project.org")
  if (!requireNamespace("dplyr", quietly = TRUE)) install.packages("dplyr", repos = "https://cloud.r-project.org")
  library(ggplot2)
  library(dplyr)
})

run_sah_correlation <- function(df, output_dir = "benchmarks/analysis/results") {
  dir.create(output_dir, showWarnings = FALSE, recursive = TRUE)

  summary_df <- df %>%
    group_by(scene, ray_set, as_variant) %>%
    summarise(
      node_count = first(node_count),
      avg_fanout = first(avg_fanout),
      median_mrays = median(mrays_per_sec),
      .groups = "drop"
    )

  # Monotonic rank correlation between fan-out / node compaction and throughput
  corr_fanout <- cor.test(summary_df$avg_fanout, summary_df$median_mrays, method = "spearman")
  
  p_corr <- ggplot(summary_df, aes(x = avg_fanout, y = median_mrays, color = as_variant, shape = ray_set)) +
    geom_point(size = 3.5, alpha = 0.85) +
    facet_wrap(~ scene, scales = "free") +
    labs(
      title = "Traversal Throughput vs. Branching Factor",
      subtitle = paste0("Spearman rank rho = ", round(corr_fanout$estimate, 3), " (p = ", format.pval(corr_fanout$p.value, digits = 3), ")"),
      x = "Average Child Fan-Out per Node",
      y = "Traversal Throughput (MRay/s)",
      color = "AS Variant",
      shape = "Workload"
    ) +
    theme_minimal(base_size = 12) +
    theme(
      strip.background = element_rect(fill = "#EFEFEF", color = NA),
      strip.text = element_text(face = "bold", size = 10),
      panel.grid.minor = element_blank()
    )

  out_plot <- file.path(output_dir, "fanout_throughput_correlation.png")
  ggsave(out_plot, p_corr, width = 10, height = 6, dpi = 300)
  message("Saved correlation plot to: ", out_plot)

  return(list(spearman_rho = corr_fanout$estimate, p_value = corr_fanout$p.value))
}
