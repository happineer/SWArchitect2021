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

    printf("Monitor Arguments: Argv[1](%s), Argv[2](%s)\n", argv[1], argv[2]);

    while(1) {
        pid = fork();

        if (pid < 0) {
            printf("[Error] fork failed!\n");
        }
        else if (pid == 0) { // child
            //execl("LgFaceRecDemoTCP", "LgFaceRecDemoTCP", "8000", "friends_1280x720_12fps.smjpeg", NULL);
            execl("LgFaceRecDemoTCP", "LgFaceRecDemoTCP", argv[1], argv[2], NULL);
            exit(1);
        }
        else { // parent
            printf("Parent process is going to wait!\n");
            child_pid = wait(&status);
    	    printf("child exit value : %d\n", WEXITSTATUS(status));
            if (WEXITSTATUS(status) == 3) {
                printf("Start to rescan\n");
                system("./rescan.sh");
            }
            else {
                printf("-----------------------------------\n");
                printf("[DEBUG] Child(LgFaceRecDemoTCP) is killed\n");
                printf("[DEBUG] Child(LgFaceRecDemoTCP) pid = %d\n", child_pid);
            }
        }
    }

    return 0;
}

