#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    if (argc != 2){
        write(2, "Error message\n", strlen("Error message\n"));
    } else {
        int x = atoi(argv[1]);
        // if (x = 0){
        //     write(2, "Error sleep time!\n", strlen("Error sleep time!\n"));
        // } else {
        //     sleep(x);
        // }
        sleep(x);
    }
    exit();
}