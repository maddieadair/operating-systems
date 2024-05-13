#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <vector>
#include <sstream>

using namespace std;

static int nPatrons = 0; // number of customers that have to wait
static int nFreeClerks;  // number of free clerks

pthread_mutex_t alone;
pthread_cond_t freeClerk = PTHREAD_COND_INITIALIZER;

struct patron
{
    string name;
    int delayTime;
    int serviceTime;
};

void *child_thread(void *arg)
{
    // Extract patron info
    struct patron *argptr;
    string name;
    int delayTime;
    int serviceTime;
    argptr = (struct patron *)arg;

    name = argptr->name;
    serviceTime = argptr->serviceTime;
    delayTime = argptr->delayTime;

    // Patron arrives at the post office
    cout << name << " arrives at post office." << endl;

    pthread_mutex_lock(&alone);

    // if there are no free clerks then put the patron in timeout
    // and increase num of patrons that waited
    if (nFreeClerks == 0)
    {
        nPatrons++;
        pthread_cond_wait(&freeClerk, &alone);
    }

    // else if there are free clerks then decrease num of free clerks (patron gets service)
    nFreeClerks--;
    cout << name << " gets service." << endl;
    pthread_mutex_unlock(&alone);

    // patron gets service for serviceTime amount of time
    sleep(serviceTime);

    // lock mutex to then signal freeClerk and unlock again
    pthread_mutex_lock(&alone);
    nFreeClerks++;                                      // increase number of free clerks
    cout << name << " leaves the post office." << endl; // patron leaves the post office
    pthread_cond_signal(&freeClerk);                    // signal that clerk is free
    pthread_mutex_unlock(&alone);                       // unlock mutex for next patron

    pthread_exit((void *)0); // terminate thread
}

int main(int argc, char *argv[])
{
    // create a mutex
    pthread_mutex_init(&alone, NULL);

    vector<patron> allPatrons;

    int patronCount = 0;

    // get number of clerks from command line
    nFreeClerks = atoi(argv[1]);

    cout << "-- The post office has today " << nFreeClerks << " clerk(s) on duty." << endl;

    // read in patron info
    string line;
    while (getline(cin, line))
    {
        stringstream ss(line);
        patron p;

        ss >> p.name >> p.delayTime >> p.serviceTime;

        allPatrons.push_back(p);
        patronCount++;
    }

    // create array of patron threads
    pthread_t tid[patronCount];

    // create thread for each patron and sleep for delayTime amount of time
    for (int i = 0; i < allPatrons.size(); i++)
    {
        sleep(allPatrons.at(i).delayTime);
        pthread_create(&tid[i], NULL, child_thread, (void *)&allPatrons[i]);
    }

    // wait for the completion of a specific thread
    for (int n = 0; n < patronCount; n++)
    {
        pthread_join(tid[n], NULL);
    }

    // print report
    cout << endl;
    cout << "SUMMARY REPORT" << endl;
    cout << patronCount << " patron(s) went to the post office." << endl;
    cout << nPatrons << " patron(s) had to wait before getting service." << endl;
    cout << patronCount - nPatrons << " patron(s) did not have to to wait." << endl << endl;

    return 0;
}