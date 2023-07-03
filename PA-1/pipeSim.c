#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {

    printf("I'm SHELL process, with PID: %d - Main command is: man ls | grep -A 1 -e \"-R\" > output.txt\n", (int) getpid());

    //anonymous file for shell-grep communication
    int fd_shell_grep[2];
    pipe(fd_shell_grep);

    int rc = fork();

    if (rc < 0) {
        //fork failed
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        //child process: grep
        
        close(fd_shell_grep[0]); //close unused read end of anonymous file

        //anonymous file for man-grep communication
        int fd_grep_man[2]; 
        pipe(fd_grep_man);

        rc = fork();

        if (rc > 0) {
            wait(NULL); //waiting for grandchild/man

            printf("I'm GREP process, with PID: %d - My command is: grep -A 1 -e \"-R\"\n", (int) getpid());

            close(fd_grep_man[1]); //closing writing end of anonymous file
            dup2(fd_grep_man[0], STDIN_FILENO); //to read result of man
            dup2(fd_shell_grep[1], STDOUT_FILENO); //to send the result to shell

            //execution of grep command
            char *myargs[6];
            myargs[0] = strdup("grep");
            myargs[1] = strdup("-A");
            myargs[2] = strdup("1");
            myargs[3] = strdup("-e");
            myargs[4] = strdup("-R");
            myargs[5] = NULL;
            execvp(myargs[0], myargs);

        } else if (!rc) {
            //grandchild process: man
            printf("I'm MAN process, with PID: %d - My command is: man ls\n", (int) getpid());

            close(fd_shell_grep[1]); //closing unrelated writing end 
            close(fd_grep_man[0]); //closing reading end for anonymous file
            dup2(fd_grep_man[1], STDOUT_FILENO); //to be able to write to anonymous file

            //execution of man ls command
            char *myargs[3];
            myargs[0] = strdup("man");
            myargs[1] = strdup("ls");
            myargs[2] = NULL;
            execvp(myargs[0], myargs);

        } else {
            fprintf(stderr, "fork failed\n");
            exit(1);
        }
    } else {
        //parent process: shell
        wait(NULL); //waiting for grep/child process to finish its execution

        close(fd_shell_grep[1]); //closing writing end of anonymous file
        dup2(fd_shell_grep[0], STDIN_FILENO); //to be able to read from anonymous file
                                                    //closing stdin

        char buff[1024];
        ssize_t n;

        n = read(STDIN_FILENO, buff, sizeof(buff)); //read from anonymous file

        int output = open("./output.txt", O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU); //opening output.txt to write the result
        write(output, buff, strlen(buff)-1); //writing the result

        printf("I'm SHELL process, with PID:%d - execution is completed, you can find the results in output.txt\n", (int) getpid());

    }
}