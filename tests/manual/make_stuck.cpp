#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    printf("Roditeljski proces (PID: %d).\n", getpid());
    printf("Ovaj proces ce uci u 'D' (Disk Sleep/Uninterruptible) stanje na 60 sekundi.\n");
    printf("Da ga pratis u watchdog-u, kucaj: add process %d\n", getpid());
    
    // vfork pauzira roditelja u 'D' stanju dok dete ne pozove exec() ili _exit()
    pid_t pid = vfork();
    
    if (pid == 0) {
        // Child process
        sleep(60); // Drzi roditelja u D stanju 60 sekundi
        _exit(0);
    } else if (pid > 0) {
        // Parent process se budi ovde posle 60 sekundi
        printf("Roditelj se probudio iz D stanja!\n");
        printf("Sada cu spavati normalno jos 30 sekundi da mozes da vidis ALIVE stanje...\n");
        sleep(30);
    } else {
        perror("vfork failed");
        return 1;
    }

    printf("Kraj programa.\n");
    return 0;
}
