// © 2026 Massachusetts Institute of Technology
// MIT License

#ifndef ANALYZER_H
#define ANALYZER_H

#include "tokenizer.h"

typedef struct {
    size_t word_count;
    size_t number_count;
    size_t keyword_count;
    size_t operator_count;
    size_t comment_count;
    size_t string_count;
    size_t line_count;
    size_t char_count;
} analysis_result_t;

// Initialize analyzer with tokenizer operations
void analyzer_init(tokenizer_ops_t ops);

// Perform analysis using function pointers
analysis_result_t analyze_text(const char *text);

// Get detailed token statistics
void print_token_distribution(void);

// Analyze code complexity
int calculate_complexity_score(void);

// Find patterns in the text
void find_patterns(const char *pattern);

#endif // ANALYZER_H
