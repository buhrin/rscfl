#include <stdio.h>
#include <stdlib.h>
#include <rscfl/user/res_api.h>

#define ADVANCED_QUERY_PRINT(...)                                              \
  result = rscfl_advanced_query(rhdl, __VA_ARGS__);                            \
  if (result != NULL) {                                                        \
    printf("timestamp: %llu\nvalue: %f\nsubsystem_name: %s\n",                 \
           result->timestamp, result->value, result->subsystem_name);          \
  } else {                                                                     \
    printf("result was null\n");                                               \
  }

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
  // int err;
  // struct accounting acct;

  // err = rscfl_acct(rhdl);
  // if(err) fprintf(stderr, "Error accounting for system call 1 [interest]\n");
  // FILE *fp = fopen("rscfl_file", "w");

  // rscfl_read_and_store_data(rhdl, "{\"extra_data\":\"yes\"}");

  // // err = rscfl_acct(rhdl);
  // // if(err) fprintf(stderr, "Error accounting for system call 2 [interest]\n");
  // // rscfl_read_and_store_data(rhdl);
  // fclose(fp);

  /*
   * Querying database
   */
  // sleep(2); // sleep on main thread to give other threads the chance to do their work
  // char *string1 = rscfl_query_measurements(rhdl, "select * from \"cpu.cycles\"");
  // if (string1){
  //   printf("InfluxDB:\nselect * from \"cpu.cycles\"\n%s\n", string1);
  //   free(string1);
  // }
  // mongoc_cursor_t *cursor = query_extra_data(rhdl, "{}", NULL);
  // if (cursor != NULL){
  //   char *string2;
  //   printf("MongoDB:\n");
  //   while (rscfl_get_next_json(cursor, &string2)){
  //     printf("Document from MongoDB:\n%s\n", string2);
  //     rscfl_free_json(string2);
  //   }
  //   mongoc_cursor_destroy(cursor);
  // }

  /*
   * Advanced queries
   */
  // query_result_t *result;
  // ADVANCED_QUERY_PRINT("cpu.cycles", COUNT, NULL)
  // ADVANCED_QUERY_PRINT("cpu.cycles", COUNT, NULL, 1517262516171014, 0)
  // ADVANCED_QUERY_PRINT("cpu.cycles", COUNT, NULL, 1517262516171014)
  // ADVANCED_QUERY_PRINT("cpu.cycles", COUNT, NULL, 0, 1517262516171014)
  // ADVANCED_QUERY_PRINT("cpu.cycles", COUNT, "Filesystem")
  // ADVANCED_QUERY_PRINT("cpu.cycles", COUNT, "Filesystem", 1517262516171014, 0)
  // ADVANCED_QUERY_PRINT("cpu.cycles", COUNT, "Filesystem", 1517262516171014)
  // ADVANCED_QUERY_PRINT("cpu.cycles", COUNT, "Filesystem", 0, 1517262516171014)
  // ADVANCED_QUERY_PRINT("cpu.cycles", MAX, NULL, 1517262516171014, 0)
  // ADVANCED_QUERY_PRINT("cpu.cycles", MAX, "Filesystem", 1517262516171014, 0)
  // ADVANCED_QUERY_PRINT("cpu.cycles", MAX, NULL, 0, 1517262516171014)
  // ADVANCED_QUERY_PRINT("cpu.cycles", MAX, "Filesystem", 0, 1517262516171014)

  // char *extra_data = rscfl_get_extra_data(rhdl, 1517262516171014);
  // printf("data:\n%s\n", extra_data);
  // rscfl_free_json(extra_data);

  timestamp_array_t array;
  array = rscfl_get_timestamps(rhdl, "{\"extra_data\":\"yes\"}");
  int i;
  for(i = 0; i < array.length; i++){
    printf("timestamp %d: %llu\n", i, array.ptr[i]);
  }
  rscfl_free_timestamp_array(array);

  rscfl_persistent_storage_cleanup(rhdl);
  return 0;
}
