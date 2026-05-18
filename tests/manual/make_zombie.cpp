#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    pid_t pid = fork();

    if (pid > 0) {
        // Parent process
        printf("Roditeljski proces (PID: %d) spava.\n", getpid());
        printf("Zombi proces je kreiran (Child PID: %d).\n", pid);
        printf("Da ga pratis u watchdog-u, kucaj: add process %d\n", pid);
        printf("Pritisni Ctrl+C da zavrsis i ocistis zombija.\n");
        
        // Spavaj beskonacno kako bi child ostao zombi
        while (1) {
            sleep(1);
        }
    } else if (pid == 0) {
        // Child process odma zavrsava
        exit(0);
    } else {
        perror("Fork failed");
        return 1;
    }

    return 0;
}
