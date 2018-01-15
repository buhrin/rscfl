#include <stdio.h>
#include <stdlib.h>
#include <rscfl/user/res_api.h>

int main(int argc, char** argv) {
  rscfl_handle rhdl;

  rhdl = rscfl_init("test", 1);

  if(rhdl == NULL) {
    fprintf(stderr,
      "Unable to talk to rscfl kernel module.\n"
      "Check that:\n"
      "\t - rscfl is loaded\n"
      "\t - you have R permissions on /dev/rscfl-data\n"
      "\t - you have RW permissions on /dev/rscfl-ctrl\n");
    return 1;
  }
  int err;
  struct accounting acct;
// declare interest in accounting the next system call
  err = rscfl_acct(rhdl);
  if(err) fprintf(stderr, "Error accounting for system call 1 [interest]\n");
  FILE *fp = fopen("rscfl_file", "w");

  rscfl_read_and_store_data(rhdl, "{\"extra_data\":\"yes\"}");

  err = rscfl_acct(rhdl);
  if(err) fprintf(stderr, "Error accounting for system call 2 [interest]\n");
  fclose(fp);
//  rscfl_read_and_store_data(rhdl);

  rscfl_cleanup(rhdl);
  return 0;
}
