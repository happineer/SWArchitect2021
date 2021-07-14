#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int main(int argc, char *argv[])
{
    int status;
    int child_pid;
    pid_t pid;
    int reboot_cnt = -1;

    printf("[Availability] =============================================\n");
    printf("[Availability] Monitor Arguments: Argv[1](%s), Argv[2](%s)\n", argv[1], argv[2]);
    printf("[Availability] =============================================\n");

    while(1) {
        pid = fork();
        reboot_cnt++;

        if (pid < 0) {
            printf("[Error] fork failed!\n");
        }
        else if (pid == 0) { // child
            if (reboot_cnt > 0) {
                printf("[Availability] =============================================\n");
                printf("[Availability] The ImageProcessingApplication is restarted !\n");
                printf("[Availability] =============================================\n");
            }
            execl("LgFaceRecDemoTCP", "LgFaceRecDemoTCP", argv[1], argv[2], NULL);
            exit(1);
        }
        else { // parent
            printf("[Availability] =============================================\n");
            printf("[Availability] Parent process is going to wait!\n");
            printf("[Availability] =============================================\n");
            child_pid = wait(&status);

            printf("[Availability] =============================================\n");
    	    printf("[Availability] Child exit value : %d\n", WEXITSTATUS(status));
            printf("[Availability] =============================================\n");
            if (WEXITSTATUS(status) == 3) {
                printf("Start to rescan\n");
                system("./rescan.sh");
            }

            // delay 3s before image processing application is restarted.
            sleep(3);
        }
    }

    return 0;
}

