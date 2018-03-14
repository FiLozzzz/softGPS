#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>

pid_t pid, pid2, pid3, pid4, ppid;
void *shm_addr;//state, period, shift, shift_max;

void clearScreen()
{
	printf("\e[1;1H\e[2J");
}

void automation()
{
	time_t curr_time, start, diff;
	int i, n;
	int sce[100][4];
	FILE *fp;

	pid3 = fork();
	if(pid3 == 0)
	{
		start = time(NULL);
		fp = fopen("automation.txt", "r");
		if(fp == NULL)
			return;
		fscanf(fp,"%d",&n);

		if(n == 0)
			exit(1);

		for(i=0; i<n; i++)
			fscanf(fp,"%d %d %d %d",&sce[i][0], &sce[i][1], &sce[i][2], &sce[i][3]);
		fclose(fp);

		i = 0;
		while(1)
		{
			sleep(1);
			curr_time = time(NULL);
			diff = curr_time - start;
			if(diff >= sce[i][0] && i < n)
			{
				((int *)shm_addr)[0] = 1;
				((int *)shm_addr)[1] = sce[i][1];
				((int *)shm_addr)[2] = sce[i][2];
				((int *)shm_addr)[3] = sce[i][3];
				
				if(i < n)
					i++;
			}
			else if(i >= n)
				break;
		}
		/*while(1)
		{
			sleep(1);
			curr_time = time(NULL);
			diff = curr_time - start;
			if(diff >= 0 && diff < 3600)
			{
				((int *)shm_addr)[0] = 1;	
				((int *)shm_addr)[1] = 1;	
				((int *)shm_addr)[2] = -1;	
				((int *)shm_addr)[3] = 3600;	
			}
			else if(diff >= 7200 && diff < 10800)
			{
				((int *)shm_addr)[0] = 1;	
				((int *)shm_addr)[1] = 1;	
				((int *)shm_addr)[2] = 1;	
				((int *)shm_addr)[3] = 0;	
			}
			else if(diff >= 14400 && diff < 18000)
			{
				((int *)shm_addr)[0] = 1;	
				((int *)shm_addr)[1] = 1;	
				((int *)shm_addr)[2] = 1;	
				((int *)shm_addr)[3] = 3600;	
			}
			else if(diff >= 21600 && diff < 25200)
			{
				((int *)shm_addr)[0] = 1;	
				((int *)shm_addr)[1] = 1;	
				((int *)shm_addr)[2] = -1;	
				((int *)shm_addr)[3] = 0;	
			}
		}*/
	}
}

void start_str2str()
{
	struct tm *tmp;
	time_t curr_time;
	int min = -1;

	while(1)
	{
		sleep(30);
		curr_time = time(NULL);
		tmp = gmtime(&curr_time);
		//if(0)
		//if(tmp->tm_min % 5 == 0 && tmp->tm_sec <= 20 && min != tmp->tm_min)
		//if(tmp->tm_min % 5 == 0 && tmp->tm_sec == 0)
		//if(tmp->tm_hour % 2 == 0 && tmp->tm_min == 0)
		if(tmp->tm_min % 15 == 0)
		{
			//min = tmp->tm_min;
			str2str();
		}
	}
}

void str2str()
{
	pid = fork();
	if(pid == 0)
	{
		//int fd = open("log.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		static char *argv2[] = {"str2str", "-c", "commands.txt", "-in", "serial://ttyACM0:9600:8:n:1:off", "-out", "test.ubx"};
		//dup2(fd, 1);
		//dup2(fd, 2);
		execv("/usr/local/bin/str2str", argv2);
		exit(127);
	}
	else
	{
		//sleep(120);
		sleep(600);
		kill(pid, SIGINT);
		kill(pid, SIGINT);
		kill(pid, SIGKILL);
		
		pid = fork();
		
		if(pid == 0) {
			static char *argv3[] = {"convbin", "test.ubx"};
			execv("/usr/local/bin/convbin", argv3);
			exit(127);
		}
		else
			waitpid(pid, 0, 0);		
	}
}

int main(void)
{
	FILE *fp;
	int n, tmp[5];
	uid_t uid;
	key_t shm_id;

	//void *shm_addr;//state, period, shift, shift_max;

	uid = getuid();
	if(uid != 0)
	{
		printf("execute me as the superuser\n");
		exit(0);
	}

	/*fp = fopen("rinex/test.ubx", "r");
	if(fp == NULL)
	{
		printf("Generate UBX file...\n");
		str2str();
	}
	else
		fclose(fp);*/
	
	if((shm_id = shmget((key_t)8081, sizeof(int)*5, IPC_CREAT|0666)) == -1)
	{
		printf("shared memory allocation fails\n");
		return -1;
	}

	if((shm_addr = shmat(shm_id, (void *)0, 0)) == (void *)-1)
	{
		printf("shared memory attachment fails\n");
		return -1;
	}

	((int *)shm_addr)[0] = 0;
	((int *)shm_addr)[1] = 1;
	((int *)shm_addr)[2] = 0;
	((int *)shm_addr)[3] = 0;

	//system("gnome-terminal -x sh -c \"sudo str2str -c commands.txt -in serial://ttyACM0 -out tcpsvr://:8887 ; bash\"");

	while(1) {
		printf("\n");
		printf("1. Start GPS Spoofer\n");
		printf("2. Generate RINEX file (Duration: 2min)\n");
		printf("3. Time shift configuration\n");
		printf("4. Start time shift\n");
		printf("5. Pause time shift\n");
		printf("6. End time shift\n");
		printf("7. Read attack status\n");
		printf("8. Clear screen\n");
		printf("9. Exit\n");

		scanf("%d", &n);
		printf("\n");

		switch(n)
		{
			case 1:
				pid2 = fork();
				if(pid2 == 0)
					start_str2str();
				else
				{
					system("gnome-terminal -x sh -c \"sudo ./run_bladerfGPS.sh -e test.nav ; bash\"");
				}
				break;
			case 2:
				printf("Generate RINEX file...\n");
				str2str();
				break;
			case 3:
				printf("Period(s), shift step(ns), max abs shift(ns)\n");
				scanf("%d, %d, %d", &((int *)shm_addr)[1], &((int *)shm_addr)[2], &((int *)shm_addr)[3]);
				((int *)shm_addr)[3] = abs(((int *)shm_addr)[3]);
				printf("%d, %d, %d\n", ((int *)shm_addr)[1], ((int *)shm_addr)[2], ((int *)shm_addr)[3]);
				break;
			case 4:
				((int *)shm_addr)[0] = 1;
				automation();
				break;
			case 5:
				((int *)shm_addr)[0] = 2;
				break;
			case 6:
				((int *)shm_addr)[0] = 0;
				((int *)shm_addr)[4] = 0;
				break;
			case 7:
				printf("Status : %d\n", ((int *)shm_addr)[0]);
				printf("Period : %ds\n", ((int *)shm_addr)[1]);
				printf("Shift step : %dns\n", ((int *)shm_addr)[2]);
				printf("Max shift abs : %dns\n", ((int *)shm_addr)[3]);
				printf("Current shift : %dns\n", ((int *)shm_addr)[4]);
				break;
			case 8:
				clearScreen();
				break;	
			case 9:
				break;
			default:
				printf("Enter 1~9\n");
				sleep(1);
		}

		if(n == 9)
			break;
	}
	
	shmdt(shm_addr);
	kill(pid, SIGINT);
	kill(pid, SIGKILL);
	kill(pid2, SIGINT);
	kill(pid2, SIGKILL);

	return 0;
}
