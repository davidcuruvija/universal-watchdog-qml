#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    pid_t pid = fork();

    if (pid > 0) {
        // Parent process
        printf("Parent process (PID: %d) is sleeping.\n", getpid());
        printf("Zombie process created (Child PID: %d).\n", pid);
        printf("To monitor it in the watchdog, type: add process %d\n", pid);
        printf("Press Ctrl+C to terminate and clean up the zombie.\n");
        
        // Sleep infinitely so the child remains a zombie
        while (1) {
            sleep(1);
        }
    } else if (pid == 0) {
        // Child process exits immediately
        exit(0);
    } else {
        perror("Fork failed");
        return 1;
    }

    return 0;
}
