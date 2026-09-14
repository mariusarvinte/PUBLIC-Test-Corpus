// © 2026 Massachusetts Institute of Technology
// MIT License

#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stddef.h>

#define MAX_TOKEN_LENGTH 256
#define MAX_BUFFER_SIZE 8192

typedef enum {
    TOKEN_EOF = 0,
    TOKEN_WORD,
    TOKEN_NUMBER,
    TOKEN_PUNCTUATION,
    TOKEN_WHITESPACE,
    TOKEN_NEWLINE,
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD,
    TOKEN_OPERATOR,
    TOKEN_STRING,
    TOKEN_COMMENT,
    TOKEN_ERROR
} token_type_t;

typedef struct {
    token_type_t type;
    char value[MAX_TOKEN_LENGTH];
    size_t length;
    int line;
    int column;
} token_t;

// Function pointer types for tokenizer operations
typedef token_t (*tokenizer_next_fn)(void);
typedef token_t (*tokenizer_peek_fn)(void);
typedef void (*tokenizer_reset_fn)(void);
typedef int (*tokenizer_load_fn)(const char *text);
typedef void (*tokenizer_get_stats_fn)(size_t *lines, size_t *tokens, size_t *chars);

// Tokenizer operations structure
typedef struct {
    tokenizer_next_fn next_token;
    tokenizer_peek_fn peek_token;
    tokenizer_reset_fn reset;
    tokenizer_load_fn load_text;
    tokenizer_get_stats_fn get_stats;
} tokenizer_ops_t;

// Get tokenizer operations (returns function pointers)
tokenizer_ops_t get_tokenizer_ops(void);

// Individual functions (can also be called directly)
token_t tokenizer_next_token(void);
token_t tokenizer_peek_token(void);
void tokenizer_reset(void);
int tokenizer_load_text(const char *text);
void tokenizer_get_stats(size_t *lines, size_t *tokens, size_t *chars);

#endif // TOKENIZER_H
