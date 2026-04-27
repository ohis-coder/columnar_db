#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define BTREE_SIZE 128
#define VERIFY_FOR_PLACEMENT_SIZE 8
#define FIXED_OVERFLOW_SIZE 512
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

const char *roots[] = {"REALTIME_DB", "WAL", "LOG"};

// creating the root files
void create_root_files() {
  for (int i = 0; i < 3; i++) {
    if (mkdir(roots[i], 0777) == -1) {
      if (errno != EEXIST) {
        perror("[!] Error: File Structure already exists\n");
      }
    }
  }
  printf("[>] Root Database File Structure Initialized.\n");
}

// this is the metadata of every columnar bin, defines the strides, the type,
// and most def the name
// added padding so as to be on a cache line
typedef struct {
  char name[64];
  int size;
  int queryable;
  int overflow;
  char *bin_ptr;
  char padding[40];
} binInitializer __attribute__((aligned(128)));

// WENAAAAA, 23 APRIL STRESSFUL... I OVERWROTE THE WHOLE FUNC TO CREATER THE BIN
// AND MAD3 IT MODULAR SO THAT WE CAN JUST USE THIS FUNCTION EVERYTIM3 W3 WANT
// TO OP3N A FIL3
// IF YOU LOOK CLOSELY NOW IT TAKES THE DATA, THE PATH AND THE SIZE OF THE BIN
// THAT IS ALL WE NEED TO OPEN A FILE AT ANY PLACE THROUGHOUT THE CODEBASE
// THE PATH WORKS BECAUSE IN THE USER CUSTOMIZATION WE RENAMED THE BIN TO THE
// FULL PATH OF THE FOLDER SO ALL THE BINS ARE IN THEIR RIGHT FOLDER
char *DB_InitializerNormalBins(binInitializer *data, char *fullName,
                               int total_size) {
  int fd = open(fullName, O_RDWR | O_CREAT, 0666);
  // choosing ftruncate over posix_fallocate() for bootstrap companies
  // only pay for the space you use is the trick with ftruncate (sparse files)
  ftruncate(fd, total_size);
  data->bin_ptr =
      mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);
  return data->bin_ptr;
}

// this is the metadata that holds the ptr to the quick bin for user action
// and the searchable bin for admin actions
typedef struct {
  char *o_1_bin_ptr;
  char *btree_ptr;
  char padding[48];
} btree_ptr_management_packet __attribute__((aligned(64)));

// this is a very modular func that returns the values in the mgmt packet for
// access
// 23 APRIL ENTRY NOW THE FUNC CALLS THE FUCTION TO MAKE A BIN WITH ITS OWN
// SPECULATIONS
btree_ptr_management_packet DB_InitializerBTreeBins(binInitializer *data,
                                                    const char *folderPath) {
  char btree_full_path[256];
  // here we instantiate the pointer manager for this type of bin
  btree_ptr_management_packet bm; // bm for btree management
  // as you can see, i open the o(1) bin for external display
  snprintf(btree_full_path, sizeof(btree_full_path), "%s.bin", data->name);
  bm.o_1_bin_ptr =
      DB_InitializerNormalBins(data, btree_full_path, data->size * MAX_USERS);
  // here i open the b+tree bin for the admin actions like querying the bin
  // i added the folderPath so that the extra bins get put in  the right filder
  // not in the .c foder
  snprintf(btree_full_path, sizeof(btree_full_path), "%s/btree.bin",
           folderPath);
  bm.btree_ptr =
      DB_InitializerNormalBins(data, btree_full_path, BTREE_SIZE * MAX_USERS);
  return bm;
}

// this is also same as above, just holds the pointers for the normal bin
// and the ptr for the overflow bin and the ptr to know if it is an overflow
// so we just jump into the overflow or the underflow instantly
typedef struct {
  char *underflow_ptr;
  char *overflow_ptr;
  char *is_overflow_ptr;
  char padding[48];
} overflow_ptr_management_packet __attribute__((aligned(64)));

// 23 APRIL ENTRY NOW THE FUNC CALLS THE FUCTION TO MAKE A BIN WITH ITS OWN
// SPECULATIONS this is a very modular func that returns the values in the mgmt
// packet for access
overflow_ptr_management_packet
DB_InitializerOverflowBins(binInitializer *data, const char *folderPath) {
  char ovrflow_full_file_path[256];
  char is_ovrflow_full_file_path[256];
  // here we instantiate the pointer manager for this type of bin
  // idk if you noticed but there is a lot of copy and past because i am mading
  // the code as modular as possible and i know what i am doing, this is not AI
  // slop
  overflow_ptr_management_packet om; // om for overflow management
  snprintf(ovrflow_full_file_path, sizeof(ovrflow_full_file_path), "%s.bin",
           data->name);
  om.underflow_ptr = DB_InitializerNormalBins(data, ovrflow_full_file_path,
                                              data->size * MAX_USERS);
  // we open the overflow bin automoatically
  // i added the folderPath so that the extra bins get put in  the right filder
  // not in the .c foder
  snprintf(ovrflow_full_file_path, sizeof(ovrflow_full_file_path),
           "%s/overflow.bin", folderPath);
  om.overflow_ptr = DB_InitializerNormalBins(data, ovrflow_full_file_path,
                                             FIXED_OVERFLOW_SIZE * MAX_USERS);
  // for future use, here we can just check this bin to route to the right bin
  // i added the folderPath so that the extra bins get put in  the right filder
  // not in the .c foder
  snprintf(is_ovrflow_full_file_path, sizeof(is_ovrflow_full_file_path),
           "%s/is_overflow.bin", folderPath);
  om.is_overflow_ptr = DB_InitializerNormalBins(
      data, is_ovrflow_full_file_path, VERIFY_FOR_PLACEMENT_SIZE * MAX_USERS);
  return om;
}

typedef struct {
  char metadataArray[2094];
} masterArray;

void user_customization(binInitializer *data, masterArray *registry) {
  char folderPath[128];
  char filePath[128];
  int i = 0;
  int num_cols = 0;
  char input[10];
  char buffer[32];

  printf("Welcome to crazydev's C columnar db, where OLAP can perform OLTP "
         "queries like no man's business\n");
  printf(
      "How many columns will your object have? [expects an int, max number of "
      "columns is 1024] : \n");

  // gets the user input for how many cols they need.. must be int, can't be
  // greater than 1024
  if (fgets(input, sizeof(input), stdin) != NULL) {
    if (!is_num(input)) {
      printf(" [!] Error: Use an integer e.g 1,2,3...1024\n");
    } else {
      int temp_cols = strtol(input, NULL, 10);
      if (temp_cols > MAX_COLS) {
        printf("[!] Error: Column number cannot be greater than 1024\n");
        return;
      }
      num_cols = strtol(input, NULL, 10);
      printf("[>] Success: Initializing DB for %d columns\n", num_cols);
    }
  }

  // to be completed.... this is the main loop that creates all the bins
  // according to their types
  for (i = 0; i < num_cols; i++) {
    // APRIL 27 very crazy thing here but check this we create a container that
    // acts like the struct but in that we create a contiguous block of the same
    // struct inside so we have a list of the struct object, dont you love C
    binInitializer *current_slot = &data[i];

    current_slot->overflow = 0;
    current_slot->queryable = 0;
    char name_full_path[512];

    // here, the user starts to customize every columnar bin according to what
    // they contain
    printf("What is the name of column? (max lenght of name is 63 chars): \n");
    if (fgets(current_slot->name, sizeof(current_slot->name), stdin) != NULL) {
      // custom check to ensure the name is not empty
      if (strlen(current_slot->name) == 0 || strlen(current_slot->name) >= 64) {
        printf("[!] Error: Column name is either empty or more than 63 chars "
               "long");
        return;
      } else {
        // the fgets func is known for notoriously storing the \n as a char to
        // we just rip the index and replace w/ \0 if seen
        current_slot->name[strcspn(current_slot->name, "\n")] = 0;
        // we create a path for the bin, we want them in the root files so we
        // pass that path to the creation process
        snprintf(folderPath, sizeof(folderPath), "%s/%s", roots[0],
                 current_slot->name);
        mkdir(folderPath, 0777);
        // 23 APRIL THIS IS THE MAGIC THAT MAKES THE FOLDER SYSTEM PERFECT, WE
        // RELACE THE NAME WITH THE PATH BEFORE CALLING FOR ANYTHING TO BE
        // CREATED
        // 27 APRIL,so from what i can see here, there are 2 differeent paths
        // one for bins that dont know where they are going and need direction
        // or automatic and one that wants to land where the code is being
        // executed  (name_full_path)
        snprintf(filePath, sizeof(filePath), "%s/%s", folderPath,
                 current_slot->name);
        strncpy(current_slot->name, filePath, sizeof(current_slot->name) - 1);
        snprintf(name_full_path, sizeof(name_full_path), "%s.bin",
                 current_slot->name);

        printf("What is the size of each col.. use a power of two eg. "
               "2,4,8,...512 (preferably 64 for max efficiency)\n");
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
          if (!is_num(buffer)) {
            printf("Please input an Integer\n");
          }
          current_slot->size = strtol(buffer, NULL, 10);
          if ((current_slot->size & (current_slot->size - 1)) != 0) {
            printf("Please Input a Power of Two\n");
            return;
          }

          printf("is this bin Queryable? (0 for No | 1 for Yes)\n");
          if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            current_slot->queryable = strtol(buffer, NULL, 10);

            if (current_slot->queryable == 0) {
              printf("does this bin require overflow handling? (0 for No | 1 "
                     "for Yes)\n");
              if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
                current_slot->overflow = strtol(buffer, NULL, 10);
              }
            }
          }
        }
      }
    }
    if (current_slot->queryable == 1) {
      printf("[>] Loading: Setting up your env.\n");
      DB_InitializerBTreeBins(current_slot, folderPath);
    } else if (current_slot->overflow == 1) {
      printf("[>] Loading: Setting up your env.\n");
      DB_InitializerOverflowBins(current_slot, folderPath);
    } else {
      DB_InitializerNormalBins(current_slot, name_full_path,
                               current_slot->size * MAX_USERS);
      printf("[>] Loading: Setting up your env.\n");
    }
    strncpy(current_slot->name, name_full_path, sizeof(current_slot->name) - 1);
  }
  printf("[>] Success: Your Env is all set up!\n");
  memcpy(registry->metadataArray, data, num_cols * sizeof(*data));
}

int main() {
  print_crazy_dev_welcome();
  create_root_files();

  binInitializer *data = malloc(sizeof(binInitializer));
  if (data == NULL)
    return 1;
  masterArray *registry = malloc(sizeof(masterArray));
  if (registry == NULL)
    return 1;

  user_customization(data, registry);
  free(data);
  free(registry);
  // pause here today!
  return 0;
}
