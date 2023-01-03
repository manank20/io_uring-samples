#include <stdio.h>
#include <unistd.h>
#include <liburing.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#define BUFFER_SIZE 4096

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Error: missing file argument\n");
    return 1;
  }

  // Open the file
  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Error opening file: %s\n", strerror(errno));
    return 1;
  }

  // Initialize the io_uring structure and create the ring buffer
  struct io_uring ring;
  int queue_size = 32;
  int ret = io_uring_queue_init(queue_size, &ring, 0);
  if (ret < 0) {
    fprintf(stderr, "Error initializing io_uring: %s\n", strerror(-ret));
    return 1;
  }

  // Allocate a buffer for the read operation
  char* buffer = (char*)malloc(BUFFER_SIZE);
  if (buffer == NULL) {
    fprintf(stderr, "Error allocating buffer\n");
    return 1;
  }

  // Prepare the read operation
  struct io_uring_sqe* sqe = io_uring_get_sqe(&ring);
  if (sqe == NULL) {
    fprintf(stderr, "Error getting sqe\n");
    return 1;
  }
  io_uring_prep_read(sqe, fd, buffer, BUFFER_SIZE, 0);

  // Submit the read operation
  ret = io_uring_submit(&ring);
  if (ret < 0) {
    fprintf(stderr, "Error submitting operations: %s\n", strerror(-ret));
    return 1;
  }

  while (1) {
    // Wait for a completed operation
    struct io_uring_cqe* cqe;
    ret = io_uring_wait_cqe(&ring, &cqe);
    if (ret < 0) {
      fprintf(stderr, "Error waiting for completion: %s\n", strerror(-ret));
      return 1;
    }

    // Process the completed operation
    int bytes_read = cqe->res;
    if (bytes_read < 0) {
      fprintf(stderr, "Error reading from file: %s\n", strerror(-bytes_read));
      return 1;
    } else if (bytes_read == 0 ) {
      // End of file
      break;
    }

    // Write the data to stdout
    write(1, buffer, bytes_read);
    if(bytes_read < BUFFER_SIZE)
      break;

    // Release the completed operation
    io_uring_cqe_seen(&ring,cqe);
  }
  
  free(buffer);
  return 0;

}
