#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>


void exec_command(char* command) {
    char* program = strtok(command," ");
    char *args[16]={program};
    int i=1;
    while((args[i++]=strtok(NULL," ")));

    // TODO: search the path instead of running "program directly"
    execv(program,args);

    fprintf(stderr,"dsh: command not found: %s\n",program);
    exit(0);
}

void run(char*);
void run_pipeline(char* head, char* tail) { 
    fprintf(stderr,"Uh-oh, I don't know how to do pipes.");
}

void run_sequence(char* head, char* tail) { 
    fprintf(stderr,"Uh-oh, I don't know how to do sequences.");
}

void run(char *line) {
    char *sep;
    if((sep=strstr(line,";"))) {
        *sep=0;        
        run_sequence(line,sep+1);
    }
    else if((sep=strstr(line,"|"))) {
        *sep=0;        
        run_pipeline(line,sep+1);
    }
    else {
        if(!fork()) 
         exec_command(line);
        else wait(0);        
    }
}

int main(int argc, char** argv) {
    char *line=0;
    size_t size=0;

    char folder[100];
    snprintf(folder,100,"%s/.dsh",getenv("HOME"));
    mkdir(folder,0755);

    // TODO: need to create the appropriate session folder
    //       to put our <N>.stdout and <N>.stderr files in.
    
    printf("dsh> ");

    // handy copies of original file descriptors
    int origin=dup(0);
    int origout=dup(1);
    int origerr=dup(2);

    while(getline(&line,&size,stdin) > 0) {

        // TODO: temporarily redirect stdio fds to
        //       files. This will be inherited by children.

        line[strlen(line)-1]=0; // kill the newline
        run(line);

        // TODO: restore the stdio fds before interacting
        //       with the user again

        printf("dsh> ");
   }
}