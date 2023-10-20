#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/wait.h>
#include "utils.h"
#include "verify.h"
#include "sem_name.h"



int main(int argc, char** argv)
{
	// NOTE: use these variables for your incoming arguments
    int num_procs = 0;      // DO NOT MODIFY THIS VARIABLE NAME
    int num_threads = 0;    // DO NOT MODIFY THIS VARIABLE NAME
    char* filename = NULL;  // DO NOT MODIFY THIS VARIABLE NAME
    char*  numTString;
    sem_t* sema4;


    //parse arguments
    int opt;
    int len;
    const char *last_four;
    while((opt = getopt(argc, argv, "p:t:f:")) != -1) {
        switch(opt)
        {
            case 'p':
                num_procs = atoi(optarg);
                if(num_procs <= 0) {
                    printf("Invalid Number of Processes, please input a positive integer value after -p.\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 't':
                num_threads = atoi(optarg);
                if(num_threads <= 0) {
                    printf("Invalid Number of Threads, please input a positive integer value after -t.\n");
                    exit(EXIT_FAILURE);
                }
                numTString = optarg;
                break;
            case 'f':
                len = strlen(optarg);
                last_four = &optarg[len-4];
                if(strcmp(last_four, ".txt") != 0) {
                    printf("Invalid FileName, please input a .txt file after -f\n");
                    exit(EXIT_FAILURE);
                }
                filename = optarg;
                break;
            case '?':
                printf("Invalid Input String\n");
                printf("Format should be main -p {num_proc} -t {num_threads} -f {fileName}\n");
                exit(EXIT_FAILURE);
        }

    }


    //open and write initial '0' to file
    FILE* stream = open_file(filename, "w+");
    fprintf(stream, "0\n");
    close_file(stream);
	//open/create named semaphore (set your YCP username as SEM_NAME in sem_name.h)
    sema4 = sem_open(SEM_NAME,O_CREAT,S_IRUSR | S_IWUSR, 1);
    if(sema4 == SEM_FAILED) {
        printf("Error Creating Semafour in Main Process");
        exit(EXIT_FAILURE);
    }



    //fork off child processes and wait for them to finish
    for(int i = 0; i < num_procs; i++) {
        int pid = fork();
        if(pid == -1) {
            perror("Forking error=");
            sem_close(sema4);
            sem_unlink(SEM_NAME);
            exit(EXIT_FAILURE);
        } else if(pid == 0) {
            execlp("./fileWriter", "fileWriter", numTString, filename, NULL);
        }
    }
    for(int i = 0; i < num_procs; i++) {
        wait(NULL);
    }

    //clean up and close named semaphore
    if(sem_close(sema4) == -1) {
        printf("Error Closing Semaphore in Main.");
        sem_unlink(SEM_NAME);
        exit(EXIT_FAILURE);
    }
    if(sem_unlink(SEM_NAME) == -1) {
        printf("Error Unlinking Semaphore in Main.");
        exit(EXIT_FAILURE);
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // IMPORTANT: The following function calls are intended to test the functionality
    //            of your program.  Do NOT remove these function calls from this file.
    //            BE SURE THAT ALL OF YOU CHILD PROCESSES HAVE TERMINATED BEFORE THIS POINT.
    /////////////////////////////////////////////////////////////////////////////////////
    verify_file_contents(num_procs, num_threads, filename);
    verify_semaphore_cleanup(SEM_NAME);

    return 0;
}

