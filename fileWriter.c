#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include "utils.h"
#include "verify.h"
#include "sem_name.h"

char* filename = NULL;
sem_t *semafour;

void *doStuff() {
    int number;
    fpos_t fp;

    if(sem_wait(semafour) != 0) {
        printf("sem_wait error in FileWriter.\n");
        sem_close(semafour);
        pthread_exit(NULL);
    }
    //^other threads wait as to not corrupt the file
    //corruption might've occurred if a thread attempted to read while this one was writing
    FILE* stream = open_file(filename, "r+");
    //r+ signifies read+write


    if(fseek(stream, 0, SEEK_END) == -1) {
        printf("Fseek error in fileWriter\n");
        sem_close(semafour);
        pthread_exit(NULL);
    }
    fgetpos(stream, &fp);
    if(fp == 2L) {
        //edge case for initial reader/writer
        //I know it's 2L due to previous testing
        number = 0;
    } else {
        //algorithm to get to the last inputted number
        long rollback = -2;
        fseek(stream, rollback, SEEK_END);
        while(getc(stream) != '\n') {
            rollback--;
            fseek(stream, rollback, SEEK_END);
        }
        if(fscanf(stream, "%i", &number) == EOF) {
            printf("Fscanf error in fileWriter\n");
            sem_close(semafour);
            pthread_exit(NULL);
        }

        //no need to change fp b/c fscanf moves it forward
    }

    //increment number by 1
    number++;
    //no real need to error check this one

    //now writing
    if(fseek(stream, 0L, SEEK_END) == -1) {
        printf("Fseek Error in fileWriter.\n");
        sem_close(semafour);
        pthread_exit(NULL);
    }
    //write at end of file so change fp to SEEK_END
    if(fprintf(stream, "%i\n", number) < 0) {
        printf("FprintF Error in FileWriter.\n");
        sem_close(semafour);
        pthread_exit(NULL);
    }
    //put number and a \n into file to set it up for the next thread

    //this thread no longer needs access to the file
    close_file(stream);
    //this thread no longer needs to be the only thread running
    if(sem_post(semafour) != 0) {
        printf("sem_post error\n");
        pthread_exit(NULL);
    }
    //this thread no longer needs to exist
    //this following line can not be error checked with a return value
      pthread_exit(0);
}


int main(int argc, char** argv)
{
	// NOTE: use these variables for your incoming arguments
    int num_threads;
	//parsing arguments
    if(argv[1] != NULL & argv[2] != NULL) {
        num_threads = atoi(argv[1]);
        filename = argv[2];
        //printf("number of threads = %i\n", num_threads);
    } else {
        printf("Invalid Input. Please Specify Number of Threads and Name of File\n");
        exit(EXIT_FAILURE);
    }

    pthread_t threads[num_threads];

	// open/create named semaphore
    semafour = sem_open(SEM_NAME,O_CREAT, S_IRUSR | S_IWUSR, 1);
    if(semafour == SEM_FAILED) {
        printf("Failed to open semaphore.\n");
        exit(EXIT_FAILURE);
    }

	//spawn threads to do the work and wait for them to finish
    for(int i = 0; i < num_threads; i++) {
        if(pthread_create(&threads[i], NULL, doStuff, NULL) != 0) {
            printf("Error creating threads\n");
            exit(EXIT_FAILURE);
        }
    }
    for(int i = 0; i < num_threads; i++) {
        if(pthread_join(threads[i], NULL) != 0) {
            printf("Error joining threads\n");
            exit(EXIT_FAILURE);
        }
    }
    if(sem_close(semafour) == -1) {
        printf("Error Closing Semaphore in fileWriter.");
        exit(EXIT_FAILURE);
    }

    //fileWriter's done.
    /////////////////////////////////////////////////////////////////////////////////////
    // IMPORTANT: The following function call is intended to test the functionality
    //            of your program.  Do NOT remove this function call from this file.
    //            BE SURE THAT ALL OF YOU CHILD PROCESSES HAVE TERMINATED BEFORE THIS POINT.
    /////////////////////////////////////////////////////////////////////////////////////
#ifdef FILEWRITER_STANDALONE_MODE
    verify_file_contents(1, num_threads, filename);
#endif

    return 0;
}
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
