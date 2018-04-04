#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include "rscfl/cJSON.h"
#include <rscfl/user/res_api.h>

#define ADVANCED_QUERY_PRINT(...)                                              \
  result = rscfl_advanced_query_with_function(rhdl, __VA_ARGS__);              \
  if (result != NULL) {                                                        \
    printf("timestamp: %llu\nvalue: %f\nsubsystem_name: %s\n",                 \
           result->timestamp, result->value, result->subsystem_name);          \
  } else {                                                                     \
    printf("result was null\n");                                               \
  }

void fn(rscfl_handle rhdl, void* p)
{
  rscfl_token *tkn = p;
  if (tkn != NULL){
    rscfl_free_token(rhdl, tkn);
  }
}

int main(int argc, char** argv) {
  rscfl_handle rhdl;
  rhdl = rscfl_init("time_no_queue", 1);

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
  int err, i, token_index;
  struct accounting acct;
  rscfl_token *tkns[30] = {0};
  // unsigned long long timestamps[15];

  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 150000000;
  for (i = 0; i < 100; i++){
    // if (!(i % 10)){
    //   printf("Iteration %d\n", i);
    // }
    // fopen
    token_index = (3*i) % 30;
    if (rscfl_get_token(rhdl, &(tkns[token_index]))) {
      fprintf(stderr, "Failed to get new token\n");
      break;
    }
    err = rscfl_acct(rhdl, tkns[token_index], ACCT_START | TK_RESET_FL);
    if (err)
      fprintf(stderr, "Error accounting for system call 1 [interest], loop %d\n", i);
    FILE* fp = fopen("rscfl_file", "w");
    err = rscfl_acct(rhdl, tkns[token_index], TK_STOP_FL);
    if (err)
      fprintf(stderr, "Error stopping accounting for system call 2 [interest], loop %d\n", i);
    // timestamps[3*i] = get_timestamp();
    rscfl_read_and_store_data(rhdl, NULL, tkns[token_index], &fn, tkns[token_index]);

    nanosleep(&ts, NULL);

    // // getuid
    // token_index = (3*i + 1) % 30;
    // if (rscfl_get_token(rhdl, &(tkns[token_index]))){
    //   fprintf(stderr, "Failed to get new token\n");
    //   break;
    // }
    // err = rscfl_acct(rhdl, tkns[token_index], ACCT_START | TK_RESET_FL);
    // if (err)
    //   fprintf(stderr, "Error accounting for system call 2 [interest], loop %d\n", i);
    // getuid();
    // err = rscfl_acct(rhdl, tkns[token_index], TK_STOP_FL);
    // if (err)
    //   fprintf(stderr, "Error stopping accounting for system call 2 [interest], loop %d\n", i);
    // timestamps[3*i + 1] = get_timestamp();
    // rscfl_read_and_store_data(rhdl, "{\"function\":\"getuid\"}", tkns[token_index], &fn, tkns[token_index]);

    // nanosleep(&ts, NULL);

    // fclose
    token_index = (3*i + 1) % 30;
    if (rscfl_get_token(rhdl, &(tkns[token_index]))){
      fprintf(stderr, "Failed to get new token\n");
      break;
    }
    err = rscfl_acct(rhdl, tkns[token_index], ACCT_START | TK_RESET_FL);
    if (err)
      fprintf(stderr, "Error accounting for system call 3 [interest], loop %d\n", i);
    fclose(fp);
    if (err)
      fprintf(stderr, "Error stopping accounting for system call 2 [interest], loop %d\n", i);
    // timestamps[3*i + 1] = get_timestamp();
    rscfl_read_and_store_data(rhdl, NULL, tkns[token_index], &fn, tkns[token_index]);

    nanosleep(&ts, NULL);
  }

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

  // char *result_no_function = rscfl_advanced_query(rhdl, "cpu.cycles", NULL);
  // if (result_no_function != NULL){
  //   printf("%s\n", result_no_function);
  //   free(result_no_function);
  // } else {
  //   fprintf(stderr, "result from advanced query is null\n");
  // }

  // char *extra_data = rscfl_get_extra_data(rhdl, 1517262516171014);
  // printf("data:\n%s\n", extra_data);
  // rscfl_free_json(extra_data);
  sleep(2);
  char* result = rscfl_query_measurements(rhdl, "select measurement_id from \"cpu.cycles\" where subsystem =~ /File/");
  if (result) {
    printf("\nresult:%s\n", result);

    cJSON *response_json = cJSON_Parse(result);
    free(result);

    cJSON *results_array = cJSON_GetObjectItem(response_json, "results");
    cJSON *result_json = cJSON_GetArrayItem(results_array, 0);
    cJSON *error = cJSON_GetObjectItem(result_json, "error");
    if (error != NULL) {
      if (cJSON_IsString(error) && (error->valuestring != NULL)) {
        fprintf(stderr,
                "Error occured.\nError: %s\n",
                error->valuestring);
      }
      cJSON_Delete(response_json);
      return -1;
    }
    unsigned long long db;
    unsigned long long rs;
    cJSON *series_array = cJSON_GetObjectItem(result_json, "series");
    cJSON *series_json = cJSON_GetArrayItem(series_array, 0);
    cJSON *values_array = cJSON_GetObjectItem(series_json, "values");
    int index = 0;
    while (true) {
      cJSON *value_pair = cJSON_GetArrayItem(values_array, index);
      if (value_pair == NULL) break;
      cJSON *timestamp_db = cJSON_GetArrayItem(value_pair, 0);
      cJSON *timestamp_rscfl = cJSON_GetArrayItem(value_pair, 1);
      if (cJSON_IsNumber(timestamp_db))
      {
        db = (unsigned long long) timestamp_db->valuedouble;
      } else {
        printf("error %d, db\n", index);
      }
      if (cJSON_IsNumber(timestamp_rscfl))
      {
        rs = (unsigned long long) timestamp_rscfl->valuedouble;
      } else {
        printf("error %d, rs\n", index);
      }
      printf("%llu\n", db - rs);
      index++;
    }
    cJSON_Delete(response_json);
  } else {
    printf("no result\n");
  }

  // int j;
  // for(j = 0; j < 10; j++){
  //   printf("timestamps[%d]=%llu", j, timestamps[j]);
  // }

  rscfl_persistent_storage_cleanup(rhdl);
  return 0;
}
