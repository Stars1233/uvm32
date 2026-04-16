#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uvm32.h"
#include "gdbstub.h"

static uint8_t *read_file(const char* filename, int *len) {
  FILE* f = fopen(filename, "rb");
  uint8_t *buf = NULL;

  if (f == NULL) {
    fprintf(stderr, "error: can't open file '%s'.\n", filename);
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  size_t file_size = ftell(f);
  rewind(f);

  if (NULL == (buf = malloc(file_size))) {
    fclose(f);
    return NULL;
  }

  size_t result = fread(buf, sizeof(uint8_t), file_size, f);
  if (result != file_size) {
    fprintf(stderr, "error: while reading file '%s'\n", filename);
    free(buf);
    fclose(f);
    return NULL;
  }
  fclose(f);

  *len = file_size;
  return buf;
}

void usage(const char *name) {
  fprintf(stderr, "%s filename.bin\n", name);
  exit(1);
}

int main(int argc, char *argv[]) {
  uvm32_state_t vmst;
  const char *rom_filename = NULL;
  int romlen = 0;    

  if (2 == argc) {
    rom_filename = argv[1];
  } else {
    usage(argv[0]);
  }

  uint8_t *rom = read_file(rom_filename, &romlen);
  if (NULL == rom) {
    fprintf(stderr,"file read failed!\n");
    return 1;
  }

  uvm32_init(&vmst);

  if (!uvm32_load(&vmst, rom, romlen)) {
    fprintf(stderr, "load failed!\n");
    return 1;
  }

  /* Attach gdbstub to vmst */
  struct gdb_state state;
  gdb_sys_init_state(&state, &vmst, NULL);

  while(1) {
    gdb_main(&state);
  }

  free(rom);
  return 0;
}
