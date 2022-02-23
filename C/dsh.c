#include<stdio.h> 
#include<unistd.h> 
#include<stdlib.h> 
#include<string.h> 
#include<fcntl.h> 
#include<sys/stat.h> 
#include<sys/types.h> 
#include<sys/wait.h> 

void exec_command(char* command) {
    char* program = strtok(command, " ");
    char* args[16] = { program };
    int i = 1;
    while ((args[i++] = strtok(NULL, " ")));

    // Parse >,<, and 2> tokens
    for (int k = 0; k < i - 1; k++) {
        // Check the tokens
        if (strcmp(args[k], ">") == 0) {
            int outFD = open(args[k + 1], O_CREAT | O_RDWR, 0755);
            dup2(outFD, 1);
            args[k] = 0;
            args[k + 1] = 0;
            k++;
            close(outFD);
        }
        else if (strcmp(args[k], "<") == 0) {
            int inFD = open(args[k + 1], O_RDONLY, 0755);
            dup2(inFD, 0);
            args[k] = 0;
            args[k + 1] = 0;
            k++;
            close(inFD);
        }
        else if (strcmp(args[k], "2>") == 0) {
            int errFD = open(args[k + 1], O_CREAT | O_RDWR, 0755);
            dup2(errFD, 2);
            args[k] = 0;
            args[k + 1] = 0;
            k++;
            close(errFD);
        }
    }

    // TODO: search the path instead of running "program  directly" get PATH environment 
    char* ret = strtok(getenv("PATH"), ":");
    char path[100];


    // parsing the string to locate the folder  
    while(ret != NULL) { 
        // create path string 
        snprintf(path, 100, "%s/%s", ret, program);
        ret = strtok(NULL, ":");
        // run the program 
        execve(path, args, NULL);
    }

    //execvp(program, args);
    // execv(program, args); 
    fprintf(stderr, "dsh: command not found: %s\n", program);  
    exit(0);
}

void run(char*);

void run_pipeline(char* head, char* tail) {
    int pipeFDs[2];
    pipe(pipeFDs);

    if (!fork()) {
        dup2(pipeFDs[1], 1);
        close(pipeFDs[0]);
        close(pipeFDs[1]);
        exec_command(head);
        perror(" p1 didn't execute!\n");
        // exit(1); 
    }

    // Don't want to fork again because it'll create another child
    dup2(pipeFDs[0], 0);
    close(pipeFDs[0]);
    close(pipeFDs[1]);
    // This is recursive to the last '|'
    run(tail);
    // Wait for child process end 
    wait(0);
    wait(0);

    // fprintf(stderr,"Uh-oh, I don't know how to do pipes."); 
}

void run_sequence(char* head, char* tail) {
    //fprintf(stderr,"Uh-oh, I don't know how to do sequence."); 
    run(head);
    run(tail);
} 

void run(char* line) {
    char* sep;

    if ((sep = strstr(line, ";"))) {
        *sep = 0;
        run_sequence(line, sep + 1);
    } else if ((sep = strstr(line, "|"))) {
        *sep = 0;
        run_pipeline(line, sep + 1);
    } else {
        if (!fork()) {
            exec_command(line);
        } else { 
            wait(0); 
        }
    }
}
 
int main(int argc, char** argv) {
    char* line = 0;
    size_t size = 0;
    char folder[100];
    int counter = 0;
    snprintf(folder, 100, "%s/.dsh", getenv("HOME"));  
    mkdir(folder, 0755);
    
    // TODO: need to create the appropriate session folder  
    // to put our <N>.stdout and <N>.stderr files in.   
    while (1) { 
        snprintf(folder, 100, "%s/.dsh/%d", getenv("HOME"), counter);
        int ret = mkdir(folder, 0755);
        if (ret == -1) {
            counter++;
        } else {
            break;
        }
    }

    printf("dsh> ");
    fflush(stdout);

    // handy copies of original file descriptors  
    int origin = dup(0); 
    int origout = dup(1);
    int origerr = dup(2);

    // stdout and stderr file counters 
    int stdout_counter = 0;
    int stderr_counter = 0;

    while (getline(&line, &size, stdin) > 0) {
        // TODO: temporarily redirect stdio fds to files. This will be
        // inherited by children. 

        // declare a new file path to create propeyly stdout and  stderr files  
        char file[256];
        int outfd, errfd;
        snprintf(file, 256, "%s/%d.stdout", folder, stdout_counter);
        outfd = open(file, O_CREAT | O_RDWR, 0755);

        if (outfd > 0) {
            dup2(outfd, 1);
            close(outfd);
        }

        snprintf(file, 256, "%s/%d.stderr", folder, stderr_counter);
        errfd = open(file, O_CREAT | O_RDWR, 0755);

        if (errfd > 0) {
            dup2(errfd, 2);
            close(errfd);
        }

        // increment counter 
        stdout_counter++;
        stderr_counter++;

        line[strlen(line) - 1] = 0; // kill the newline  
        run(line); 

        // TODO: restore the stdio fds before interacting with the user again 
        dup2(origin, 0);
        dup2(origout, 1);
        dup2(origerr, 2);
        printf("dsh> ");
    }
    // End of main()
}
