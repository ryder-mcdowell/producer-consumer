#include <stdio.h>
#include <cstdlib>
#include <fcntl.h>
#include <vector>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
using namespace std;

class Input {
public:
    int producersCount;
    int consumersCount;
    int maximumConsumables;
    int itemsToProduce;
};

class Segment {
public:
    int start;
    int end;
};

class Consumable {
public:
    int id;
    int sleepTime;
};

sem_t sem1;
sem_t sem2;
pthread_mutex_t mutex;
vector<Consumable> consumables;
int consumableIDCounter = 0;
int consumerIDCounter = 0;

void checkInputArgs(int argc, char **argv);
Input storeInputArgs(int argc, char **argv);
vector<Segment> createSegments(int itemsToProduce, int producersCount);
void *produce(void *args);
void *consume(void *args);


int main(int argc, char **argv) {

    int errorCheck;

    checkInputArgs(argc, argv);
    Input input = storeInputArgs(argc, argv);

    sem_init(&sem1, 0, 0);
    sem_init(&sem2, 0, input.maximumConsumables);

    vector<Segment> segments = createSegments(input.itemsToProduce, input.producersCount);
    vector<pthread_t> producerThreads;

    //start producers
    for (int i = 0; i < input.producersCount; i++) {
        pthread_t p;
        errorCheck = pthread_create(&p, NULL, produce, (void *) &segments[i]);
        if (errorCheck > 0) {
            perror("ERROR");
            exit(1);
        }
        producerThreads.push_back(p);
    }
    //start consumers
    for (int i = 0; i < input.consumersCount; i++) {
        pthread_t c;
        errorCheck = pthread_create(&c, NULL, consume, (void *) &segments[i]);
        if (errorCheck > 0) {
            perror("ERROR");
            exit(1);
        }
    }
    //wait on producers to finish producing
    for (int i = 0; i < input.producersCount; i++) {
        errorCheck = pthread_join(producerThreads[i], NULL);
        if (errorCheck > 0) {
            perror("ERROR");
            exit(1);
        }
    }
    fprintf(stdout, "DONE PRODUCING!!\n");

    //keep program running to allow stray consumers to finish and print
    while(1) { }
}


//exits if input arguments do not follow usage
void checkInputArgs(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "usage: ./a.out num_prod num_cons buf_size num_items\n");
        exit(1);
    }
    if (atoi(argv[1]) < 1 || atoi(argv[2]) < 1 || atoi(argv[3]) < 1 || atoi(argv[4]) < 1) {
        fprintf(stderr, "error: all input arguments must be greater than 0\n");
        exit(1);
    }
}

//stores and return input arguments in Input object
Input storeInputArgs(int argc, char **argv) {
    Input input;
    input.producersCount = atoi(argv[1]);
    input.consumersCount = atoi(argv[2]);
    input.maximumConsumables = atoi(argv[3]);
    input.itemsToProduce = atoi(argv[4]);
    return input;
}

//create segments for work ranges for producers, last segment gets remainder
vector<Segment> createSegments(int itemsToProduce, int producersCount) {
    vector<Segment> segments;

    //get segment length
    int segmentLength = itemsToProduce / producersCount;

    //create segments of length
    for (int i = 0; i < producersCount; i++) {
        Segment s;
        s.start = segmentLength * i;
        if (i != producersCount - 1) {
            s.end = segmentLength * (i + 1);
        } else {
            //last segment gets remainder
            s.end = itemsToProduce;
        }
        segments.push_back(s);
    }
    return segments;
}

//thread function for producers
void *produce(void *arg) {
    //simulate "work"
    usleep(rand() % (700 - 300 + 1) + 300);

    //turn argument back into a Segment
    Segment *s = (Segment *) arg;

    //create consumbable and add it to global buffer
    for (int i = s->start; i < s->end; i++) {
        sem_wait(&sem2);

        Consumable c;
        c.id = consumableIDCounter;
        consumableIDCounter += 1;
        c.sleepTime = rand() % (900 - 200 + 1) + 200;

        pthread_mutex_lock(&mutex);
        consumables.push_back(c);
        pthread_mutex_unlock(&mutex);

        sem_post(&sem1);
    }
}

//thread function for consumers
void *consume(void *arg) {
    int sleepTime;

    int id = consumerIDCounter;
    consumerIDCounter += 1;

    //grab consumable from front of list, erase it, and sleep to simulate "work" time
    while(1) {
        sem_wait(&sem1);
        pthread_mutex_lock(&mutex);
        Consumable consumable = consumables.front();
        fprintf(stdout, "%3d: Consuming     %4d\n", id, consumable.id);
        sleepTime = consumable.sleepTime;
        consumables.erase(consumables.begin());
        pthread_mutex_unlock(&mutex);
        usleep(sleepTime);
        sem_post(&sem2);
    }
}
