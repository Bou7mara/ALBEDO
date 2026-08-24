
suppressPackageStartupMessages({
  if (!requireNamespace("ggplot2", quietly = TRUE)) install.packages("ggplot2", repos = "https://cloud.r-project.org")
  if (!requireNamespace("dplyr", quietly = TRUE)) install.packages("dplyr", repos = "https://cloud.r-project.org")
  library(ggplot2)
  library(dplyr)
})

plot_duplication_distribution <- function(dup_df, output_dir = "benchmarks/analysis/results") {
  dir.create(output_dir, showWarnings = FALSE, recursive = TRUE)

  p <- ggplot(dup_df, aes(x = duplication_ratio, fill = scene)) +
    geom_histogram(alpha = 0.7, position = "identity", bins = 20) +
    geom_vline(xintercept = 2.0, linetype = "dashed", color = "#D9534F", linewidth = 1.0) +
    annotate("text", x = 2.05, y = 3, label = "Duplication Ceiling (2.0x)", color = "#D9534F", hjust = 0, fontface = "bold") +
    labs(
      title = "SBVH Reference Duplication Across Scene Configurations",
      subtitle = "Distribution of final reference count vs. original primitive count",
      x = "Reference Duplication Ratio (Final References / Original Primitives)",
      y = "Count of Test Configurations",
      fill = "Scene"
    ) +
    theme_minimal(base_size = 12) +
    theme(
      panel.grid.minor = element_blank()
    )

  out_plot <- file.path(output_dir, "sbvh_duplication_distribution.png")
  ggsave(out_plot, p, width = 9, height = 5, dpi = 300)
  message("Saved duplication distribution plot to: ", out_plot)
}
