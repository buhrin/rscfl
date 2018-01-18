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

  /*
   * Storing data into database
   */
  int err;
  struct accounting acct;

  err = rscfl_acct(rhdl);
  if(err) fprintf(stderr, "Error accounting for system call 1 [interest]\n");
  FILE *fp = fopen("rscfl_file", "w");

  rscfl_read_and_store_data(rhdl, "{\"extra_data\":\"yes\"}");

  // err = rscfl_acct(rhdl);
  // if(err) fprintf(stderr, "Error accounting for system call 2 [interest]\n");
  // fclose(fp);
  // rscfl_read_and_store_data(rhdl);

  /*
   * Querying database
   */
  sleep(2); // sleep on main thread to give other threads the chance to do their work
  char *string1 = rscfl_query_measurements(rhdl, "select * from \"cpu.cycles\"");
  if (string1){
    printf("InfluxDB:\nselect * from \"cpu.cycles\"\n%s\n", string1);
    free(string1);
  }
  mongoc_cursor_t *cursor = query_extra_data(rhdl, "{}", NULL);
  char *string2;
  printf("MongoDB:\n");
  while (rscfl_get_next_json(cursor, &string2)){
    printf("Document from MongoDB:\n%s\n", string2);
    bson_free(string2);
  }
  mongoc_cursor_destroy(cursor);
  rscfl_cleanup(rhdl);
  return 0;
}
