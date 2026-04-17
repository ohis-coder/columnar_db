#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_COLS 1024
#define MAX_USERS 16384
void print_crazy_dev_welcome() {
  // ANSI Color Codes
  const char *CYAN = "\033[1;36m";
  const char *YELLOW = "\033[1;33m";
  const char *RESET = "\033[0m";

  const char *logo[] = {" ___________________    _____  _______________.___. "
                        "________  _______________   ____ ",
                        " \\_   ___ \\______   \\  /  _  \\ \\____    /\\__  | "
                        "  | \\______ \\ \\_   _____/\\   \\ /   / ",
                        " /    \\  \\/|       _/ /  / \\  \\  /     /  /   |   "
                        "|  |    |  \\ |    __)_  \\   Y   /  ",
                        " \\     \\___|    |   \\/    |    \\/     /_  \\____  "
                        " |  |    `   \\|        \\  \\     /   ",
                        "  \\______  /____|_  /\\____|__  /_______ \\ / "
                        "______| /_______  /_______  /   \\___/    ",
                        "         \\/       \\/         \\/        \\/ \\/     "
                        "           \\/        \\/             ",
                        "                                 ________ __________  "
                        "                               ",
                        "                                 \\______ \\\\______  "
                        " \\                                ",
                        "                                  |    |  \\|    |  "
                        "_/                                ",
                        "                                  |    `   \\    |   "
                        "\\                                ",
                        "                                 /_______  /______  / "
                        "                               ",
                        "                                         \\/       "
                        "\\/                                "};

  printf("%s", CYAN);
  for (int i = 0; i < 12; i++) {
    printf("%s\n", logo[i]);
    usleep(20000);
  }

  printf("\n%s", YELLOW);
  printf("[ KERNEL: V1.0 ] [ SHARD_SIZE: 16384 ] [ ARCH: COLUMNAR_SPLAY ]\n");
  printf(
      "Nigeria's first free columnar c db.. founded solo by crazydev @cxx_23 "
      "on X\n");
  printf("%s\n", RESET);

  printf("---------------------------------------------------------------------"
         "----------\n\n");
}

// helper function to check if user does not input an int when an int is needed
int is_num(const char *str) {
  while (*str) {
    if (!isdigit(*str) && *str != '\n')
      return 0;
    str++;
  }
  return 1;
}

// this is the metadata of every columnar bin, defines the strides, the type,
// and most def the name
typedef struct {
  char name[64];
  int size;
  int queryable;
  int overflow;
  char padding[48];
} binInitializer;

// this opens a bin with part information in the metadata struct binInitializer
// basically normal bins with no extra bins for special functions
char *DB_InitializerNormalBins(binInitializer *data) {
  int fd = open(data->name, O_RDWR | O_CREAT, 0666);
  int total_size = data->size * MAX_USERS;
  // choosing ftruncate over posix_fallocate() for bootstrap companies
  // only pay for the space you use is the trick with ftruncate (sparse files)
  ftruncate(fd, total_size);
  char *raw_ptr =
      mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);
  return raw_ptr;
}

void user_customization(binInitializer *data, int i) {
  char folderPath[128];
  char filePath[128];

  // here, the user starts to customize every columnar bin according to what
  // they contain
  printf("What is the name of column? (max lenght of name is 63 chars): \n");
  if (fgets(data->name, sizeof(data->name), stdin)) != NULL) {
      // custom check to ensure the name is not empty
      if (strnlen(data->name) == 0) {
        printf("[!] Error: Column name cannot be empty.");
        return;
      } else {
        // the fgets func is known for notoriously storing the \n as a char to
        // we just rip the index and replace w/ \0 if seen
        data->name[strcspn(data->name, "\n")] = 0;
        // we create a path for the bin, we want them in the root files so we
        // pass that path to the creation process
        sprintf(folderPath, sizeof(folderPath), "%s/%s", roots[i], data->name);
        mkdir(folderPath, 0777);
        sprintf(filePath, sizeof(filePath), "%s/%s", folderPath, data->name);
        strncpy(data->name, filePath, sizeof(data->name) - 1);
        // char *ptr = DB_InitializerNormalBins(data->name);
      }
    }
}

int main() {
  print_crazy_dev_welcome();

  // creating the root files
  const char *roots[] = {"REALTIME_DB", "WAL", "LOG"};

  for (int i = 0; i < 3; i++) {
    if (mkdir(roots[i], 0777) == -1) {
      if (errno != EEXIST) {
        printf("[!] Error: File Strucure already exists\n");
        return 1;
      }
    }
  }
  printf("[>] Root Database File Structure Initialized.\n");

  int i;
  char input[10];
  int num_cols = 0;

  printf("Welcome to crazydev's C columnar db, where OLAP can perform OLTP "
         "queries like no man's business\n");
  printf("How many columns does your object have? [expects an int, max no of "
         "columns is 1024] : \n");

  // gets the user input for how many cols they need.. must be int, can't be
  // greater than 1024
  if (fgets(input, sizeof(input), stdin)) {
    if (!is_num(input)) {
      printf(" [!] Error: Use an integer e.g 1,2,3...1024\n");
    } else {
      int temp_cols = strtol(input, NULL, 10);
      if (temp_cols > MAX_COLS) {
        printf("[!] Error: Column number cannot be greater than 1024\n");
        return 0;
      }
      num_cols = strtol(input, NULL, 10);
      printf("[>] Success: Initializing DB for %d columns\n", num_cols);
    }
  }

  binInitializer currentSession;

  /*
  for (i; i < num_cols; i++) {
    return 0;
  }
  */
  return 0;
}
