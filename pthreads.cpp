#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sstream>
#include <cstdlib>

using namespace std;

int defGroup = 0;
static pthread_mutex_t bsem;
static int members;
static pthread_cond_t empty1[10], empty2 = PTHREAD_COND_INITIALIZER;
static int totalGroup1, totalGroup2, lockedp, lockedg, counter, other;
static int position[10];

class Thread_Log {
	static int Group;
public:
	static void *access_DBMS(void *user_info)
	{
		pthread_mutex_lock(&bsem);
		counter++;
		int *info = (int*)user_info;
		info[5] = counter;
		cout << "User " << info[5] << " from Group " << info[0] << " arrives to the DBMS" << endl;
		if (Group != info[0]) {
			lockedg++;
			cout << "User " << info[5] << " is waiting due to its group" << endl;
			pthread_cond_wait(&empty2, &bsem);
		}

		if (position[info[1]] != 0) {
			lockedp++;
			cout << "User " << info[5] << " is waiting: position " << info[1] << " of the database is being used by user " << position[info[1]] << endl;
			pthread_cond_wait(&empty1[info[1]], &bsem);
		}


		position[info[1]] = info[5];
		cout << "User " << info[5] << " is accessing the position " << info[1] << " of the database for " << info[3] << " second(s)" << endl;
		pthread_mutex_unlock(&bsem);

		sleep(info[3]);
		
		pthread_mutex_lock(&bsem);
		cout << "User " << info[5] << " finished its execution" << endl;
		position[info[1]] = 0;
		pthread_cond_signal(&empty1[info[1]]);
		members--;
		if (info[0] == Group && members == 0) {
			usleep(5);
			swapGroup();
		}
		pthread_mutex_unlock(&bsem);
		return NULL;
	}	
	static void printSum() {
		cout << "\nTotal Requests:" << endl;
		cout << "\tGroup 1: " << totalGroup1 << "\n\tGroup 2: " << totalGroup2 << endl;
		cout << "\nRequests that waited: " << endl;
		cout << "\tDue to its group: " << lockedg << "\n\tDue to a locked position: " << lockedp << endl;
	}
	void define(int cGroup) {
		Group = cGroup;
		if (cGroup == 1) {
			members = totalGroup1;
			other = totalGroup2;
			if (totalGroup1 == 0) {
				swapGroup();
			}
		}
		else {
			members = totalGroup2;
			other = totalGroup1;
			if (totalGroup2 == 0) {
				swapGroup();
			}
		}
	}
	void checkGroupTotal(int num, string inputLines[]) {
		int groupNum;
		for (int i = 0; i < num; i++)
		{
			stringstream stream(inputLines[i]);
			for (int j = 0; j < 1; j++)
			{
				stream >> groupNum;
				if (groupNum == 1)
					totalGroup1++;
				else if (groupNum == 2) 
					totalGroup2++;
			}
		}
	}
	static void swapGroup() {	
		cout << "\nAll users from Group " << Group << " finished their execution" << endl;
		if(Group == 1)
			Group = 2;
		else if (Group == 2)
			Group = 1;
		cout << "The users from Group " << Group << " start their execution" << endl << endl;
		pthread_cond_broadcast(&empty2);
	}
};
int Thread_Log::Group = defGroup;

int main()
{
	string line, inputLines[20];
	int groupStart = 0, numThreads, groupNum, position, time, sec;
	bool firstLoop = true;
	while (getline(cin, line)) {
		if (firstLoop == true) {
			stringstream start(line);
			start >> groupStart;
			firstLoop = false;
		}
		else {
			inputLines[numThreads] = line;
			numThreads++;
		}
	}
	
	Thread_Log log;
	log.checkGroupTotal(numThreads, inputLines);
	log.define(groupStart);
	pthread_t tid[numThreads];
	pthread_mutex_init(&bsem, NULL);
	//empty1 = new pthread_cond_t[10]; //Creat an array of condition varables.
	for (int i = 0; i < 10; i++) {
		pthread_cond_init(&empty1[i], NULL);
	}
	int user[numThreads][10];

	for (int i = 0; i<numThreads; i++)
	{
		stringstream stream(inputLines[i]);
		for (int j = 0; j < 4; j++)
		{
			if (j == 0) {
				stream >> groupNum;
				user[i][j] = groupNum;
			}
			else if (j == 1) {
				stream >> position;
				user[i][j] = position;
			}
			else if (j == 2) {
				stream >> time;
				user[i][j] = time;
			}
			else if (j == 3) {
				stream >> sec;
				user[i][j] = sec;
			}
		}
		sleep(time);
		if (pthread_create(&tid[i], NULL, log.access_DBMS, (void *)&user[i])) {
			fprintf(stderr, "Error creating thread\n");
			return 1;
		}
	}
	for (int i = 0; i < numThreads; i++)  // Wait for the other threads to finish.
		pthread_join(tid[i], NULL);
	log.printSum();
	return 0;
}