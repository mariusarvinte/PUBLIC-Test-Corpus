// © 2026 Massachusetts Institute of Technology
// MIT License

#include <stdio.h>
#include <stdlib.h>

int forward_goto_example(int x) {
  if (x < 0) {
    goto error;
  }

  printf("Processing: %d\n", x);
  return x * 2;

error:
  fprintf(stderr, "Error: negative input\n");
  return -1;
}

FILE* open_with_cleanup(const char *filename) {
  FILE* fp = fopen(filename, "r");
  if (!fp) {
    goto cleanup;
  }

  char buffer[100];
  while (fgets(buffer, sizeof(buffer), fp)) {
      printf("%s", buffer);
  }

  if (ferror(fp)) {
      goto cleanup;
  }

  return fp;

cleanup:
  fprintf(stderr, "Error: opening or processing file %s\n", filename);
  if(fp) fclose(fp);
  return NULL;
}

int driver(int num, const char* filename) {
  int res = forward_goto_example(num);
  if (res == -1) {
      return -1;
  } else {
      printf("Goto output: %d\n", res);
  }

  FILE* out = open_with_cleanup(filename);
  if (out == NULL) {
      return -2;
  } else {
     fclose(out);
  }

  return 0;
}
