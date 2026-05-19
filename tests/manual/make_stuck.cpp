#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    printf("Parent process (PID: %d).\n", getpid());
    printf("This process will enter the 'D' (Disk Sleep/Uninterruptible) state for 60 seconds.\n");
    printf("To monitor it in the watchdog, type: add process %d\n", getpid());
    
    // vfork pauses the parent in 'D' state until the child calls exec() or _exit()
    pid_t pid = vfork();
    
    if (pid == 0) {
        // Child process
        sleep(60); // Keeps the parent in D state for 60 seconds
        _exit(0);
    } else if (pid > 0) {
        // Parent process wakes up here after 60 seconds
        printf("Parent woke up from D state!\n");
        printf("Now sleeping normally for 30 seconds so you can see the ALIVE state...\n");
        sleep(30);
    } else {
        perror("vfork failed");
        return 1;
    }

    printf("Program finished.\n");
    return 0;
}
