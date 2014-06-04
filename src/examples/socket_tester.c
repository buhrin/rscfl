#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <res_user/res_api.h>
#include <costs.h>

int main(int argc, char *argv[])
{
  int DEBUG = 0;
  int sock_domain = AF_LOCAL;
  int sock_type = SOCK_RAW;
  int sock_proto = 0;

  int socfd_1;
  int socfd_2;
  int socfd_3;

  if (DEBUG)
    printf("Opening sockets\n");

  rscfl_handle relay_f_data;
  struct accounting acct = {0};

// Open 3 sockets
  if ( !(relay_f_data = rscfl_init())) {
    printf("rscfl: Error initialising. Errno=%d\n", errno);
    return -1;
  }

  if ( (socfd_1 = socket(sock_domain, sock_type, sock_proto)) < 0) {
    goto soc_err;
  }

  if (rscfl_acct_next(relay_f_data)) {
    fprintf(stderr, "rscfl: acct_next errno=%d\n", errno);
    return -1;
  }

  if ( (socfd_2 = socket(sock_domain, sock_type, sock_proto)) < -1 ) {
    goto soc_err;
  }

  if (!rscfl_read_acct(relay_f_data, &acct)) {
    printf("Acct: %llu\n", acct.cpu.cycles);
  }
  else {
    fprintf(stderr, "rscfl: read_acct failed\n");
  }

  if ( (socfd_3 = socket(sock_domain, sock_type, sock_proto)) < -1 ) {
    goto soc_err;
  }

  if (DEBUG)
    printf("Closing sockets\n");

  // Be a good citizen
  close(socfd_1);
  close(socfd_2);
  close(socfd_3);
  return 0;

soc_err:
  fprintf(stderr, "rscfl: error creating sockets\n");
  return -1;
}
