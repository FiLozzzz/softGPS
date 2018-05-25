#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

pid_t pid, pid1, pid2, pid3, pid4, pid5, ppid;
void *shm_addr;//state, period, shift, shift_max;
char arg[512];

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
				//break;
				exit(1);
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

void readllh()
{
	pid1 = fork();
	if(pid1 == 0)
	{
		int fd = open("log.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		static char *argv4[] = {"str2str", "-c", "commands2.txt", "-in", "serial://ttyACM0:9600:8:n:1:off", "-out", "nmea.ubx"};
		dup2(fd, 1);
		dup2(fd, 2);
		execv("/usr/local/bin/str2str", argv4);
		exit(127);
	}
	else
	{
    	FILE *fp;
    	char c, line[81];
    	char *pch, *ptrs[80];
    	int i, nums = 0;
    	float lat = -1, lng, hgt, intpart, frac;
	

		//printf("pid = %d\n", pid);
		sleep(3);

		kill(pid1, SIGINT);
		kill(pid1, SIGINT);
		kill(pid1, SIGKILL);
		
    	fp = fopen("nmea.ubx", "rb");
    	if(fp == NULL)
   		{
			fprintf(stderr, "Failed to read the current location\n");
			return;
		}

    	while(1)
    	{
       		do {
            	fread(&c, sizeof(char), 1, fp);
        	}while(c != '$');
        
        	fread(line, sizeof(char), 6, fp);
        	line[6] = '\0';

        	if(!strcmp(line, "GPGGA,"))
        	{
           		fread(line, sizeof(char), 80, fp);
           		for(i=0; i<79; i++)
               		if(line[i] == 13 && line[i+1] == 10)
                   		break;
           		line[i] = '\0';

           		pch = strtok(line, ",");
            	ptrs[nums++] = pch;

            	while(pch != NULL)
            	{
                	pch = strtok(NULL, ",");
                	ptrs[nums++] = pch;
            	}

            	if(nums > 10)
            	{
                	frac = modff(atof(ptrs[1]) / 100, &intpart);
                	lat = intpart + frac * 100 / 60;
                	frac = modff(atof(ptrs[3]) / 100, &intpart);
                	lng = intpart + frac * 100 / 60;
                	hgt = atof(ptrs[10]);

                	if(strcmp(ptrs[2], "N"))
                	   lat *= (-1);
                	if(strcmp(ptrs[4], "E"))
                	   lng *= (-1);

					sprintf(arg, "gnome-terminal -x sh -c \"sudo ./run_bladerfGPS.sh -e rinex.nav -l %f,%f,%d; bash\"", lat, lng, (int)hgt);
                	//printf("\narg = %s\n", arg); 
                	break;
            	}
       	 	}
    	}    
		if(lat < 0)
			fprintf(stderr, "Failed to read the current location\n");
	}	
}

void start_str2str()
{
	struct tm *tmp;
	time_t curr_time;
	int flag = 1;

	while(1)
	{
		sleep(1);
		curr_time = time(NULL);
		tmp = gmtime(&curr_time);
		//if(0)
		//if(tmp->tm_min % 5 == 0 && tmp->tm_sec <= 20 && min != tmp->tm_min)
		//if(tmp->tm_min % 5 == 0 && tmp->tm_sec == 0)
		//if(tmp->tm_hour % 2 == 0 && tmp->tm_min == 0)
		//if(tmp->tm_min % 15 == 0)
		if(tmp->tm_hour % 2 == 0 || flag)
		//if(tmp->tm_min % 5 == 0 && flag)
		{
			//min = tmp->tm_min;
			flag = 0;
			str2str();
		}
	}
}

void str2str()
{
	pid = fork();
	if(pid == 0)
	{
		int fd = open("log.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		static char *argv2[] = {"str2str", "-c", "commands.txt", "-in", "serial://ttyACM0:9600:8:n:1:off", "-out", "test.ubx"};
		dup2(fd, 1);
		dup2(fd, 2);
		execv("/usr/local/bin/str2str", argv2);
		exit(127);
	}
	else
	{
		//sleep(120);
		//sleep(600);
		struct tm *tmp;
		time_t curr_time;
		while(1)
		{
			sleep(1);
			curr_time = time(NULL);
			tmp = gmtime(&curr_time);

			if(tmp->tm_hour % 2 == 1 && tmp->tm_min >= 59)
				break;
		}

		kill(pid, SIGINT);
		kill(pid, SIGINT);
		kill(pid, SIGKILL);
		
		/*pid = fork();
		
		if(pid == 0) {
			static char *argv3[] = {"convbin", "test.ubx"};
			execv("/usr/local/bin/convbin", argv3);
			exit(127);
		}
		else
		{
			FILE *fp, *op;
			char c;
			waitpid(pid, 0, 0);
			
			fp = fopen("test.nav", "r");
			if(fp != NULL)
			{
				op = fopen("rinex.nav", "w");
				while(1) {
					c = fgetc(fp);
					if(c == EOF) break;
					fputc(c, op);
				}
				fclose(fp);
				fclose(op);
				
			}
		}*/
	}
}

void start_convbin()
{
	struct tm *tmp;
	time_t curr_time;

	while(1)
	{
		sleep(1);
		curr_time = time(NULL);
		tmp = gmtime(&curr_time);
		if(tmp->tm_min % 15 == 14 && tmp->tm_sec == 0)
		{
			convbin();
		}
	}
}

int convbin()
{
	pid5 = fork();
	if(pid5 == 0)
	{
		static char *argv3[] = {"convbin", "test.ubx"};
		execv("/usr/local/bin/convbin", argv3);
		exit(127);
	}
	else
	{
		FILE *fp, *op;
		char line[520][82], tmp[20];
		int i, j, num = 0, start = -1;
		long t1, t2, t3;
		struct epoch {
			int y, m, d, hh, mm;
		}prev, min, e;
		prev.y = prev.m = prev.d = prev.hh = prev.mm = 0;

		waitpid(pid5, 0, 0);
			
		fp = fopen("test.nav", "r");
		if(fp != NULL)
		{
			op = fopen("rinex.nav", "w");
			while(1) {
				if(fgets(line[num++], 82, fp) == NULL) {
					num--;
					break;
				}
				if(start < 0)
					fputs(line[num-1], op);
				if(strncmp(line[num-1]+60, "END OF HEADER", 13) == 0)
					start = num;
			}
			fclose(fp);

			while(1)
			{
				min.m = min.d = min.hh = min.mm = 0;
				min.y = 100;
				t1 = ((((prev.y * 365 + prev.m) * 31) + prev.d) * 24 + prev.hh) * 60 + prev.mm;
				t2 = ((((min.y * 365 + min.m) * 31) + min.d) * 24 + min.hh) * 60 + min.mm;
				for(i=start; i<num; i+=8) 
				{
					strncpy(tmp, line[i]+3, 2);
					tmp[2] = 0;
					e.y = atoi(tmp);

					strncpy(tmp, line[i]+6, 2);
					tmp[2] = 0;
					e.m = atoi(tmp);

					strncpy(tmp, line[i]+9, 2);
					tmp[2] = 0;
					e.d = atoi(tmp);

					strncpy(tmp, line[i]+12, 2);
					tmp[2] = 0;
					e.hh = atoi(tmp);

					strncpy(tmp, line[i]+15, 2);
					tmp[2] = 0;
					e.mm = atoi(tmp);

					t3 = ((((e.y * 365 + e.m) * 31) + e.d) * 24 + e.hh) * 60 + e.mm;
					if(t1 < t3 && t3 < t2)
					{
						min = e;
						t2 = ((((min.y * 365 + min.m) * 31) + min.d) * 24 + min.hh) * 60 + min.mm;
					}
				}

				if(min.y == 100)
					break;

				for(i=start; i<num; i+=8)
				{
					strncpy(tmp, line[i]+3, 2);
					tmp[2] = 0;
					e.y = atoi(tmp);

					strncpy(tmp, line[i]+6, 2);
					tmp[2] = 0;
					e.m = atoi(tmp);

					strncpy(tmp, line[i]+9, 2);
					tmp[2] = 0;
					e.d = atoi(tmp);

					strncpy(tmp, line[i]+12, 2);
					tmp[2] = 0;
					e.hh = atoi(tmp);

					strncpy(tmp, line[i]+15, 2);
					tmp[2] = 0;
					e.mm = atoi(tmp);
					
					if(min.y == e.y && min.m == e.m && min.d == e.d && min.hh == e.hh && min.mm == e.mm)
					{
						time_t curr_time;
						struct tm *tmp2;
						curr_time = time(NULL);
						tmp2 = gmtime(&curr_time);

						if(tmp2->tm_year + 1900 == e.y + 2000)
							for(j=i; j<i+8; j++)
								fputs(line[j], op);
					}
				}

				prev = min;
			}
			fclose(op);
			return (num-start)/8;
		}
		return 0;
	}
}

int main(void)
{
	FILE *fp;
	int n, tmp[5];
	uid_t uid;
	key_t shm_id;
	char input[100];

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

	//readllh();

	//system("gnome-terminal -x sh -c \"sudo str2str -c commands.txt -in serial://ttyACM0 -out tcpsvr://:8887 ; bash\"");
	pid2 = fork();
	if(pid2 == 0)
		start_str2str();

	pid4 = fork();
	if(pid4 == 0)
		start_convbin();

	while(1) {
		printf("\n");
		printf("1. Start GPS Spoofer\n");
		printf("2. Extract GPS Nav Msg\n");
		printf("3. Time shift configuration\n");
		printf("4. Start time shift\n");
		printf("5. Pause time shift\n");
		printf("6. End time shift\n");
		printf("7. Read attack status\n");
		printf("8. Clear screen\n");
		printf("9. Jamming/Spoofing\n");
		printf("10. Exit\n");

		//scanf("%d", &n);
		gets(input);
		n = atoi(input);
		printf("\n");

		switch(n)
		{
			case 1:
				/*pid2 = fork();
				if(pid2 == 0)
					start_str2str();
				else
				{
					system("gnome-terminal -x sh -c \"sudo ./run_bladerfGPS.sh -e rinex.nav ; bash\"");
				}*/
				if(convbin() >= 5)
					//system(arg);
					system("gnome-terminal -x sh -c \"sudo ./run_bladerfGPS.sh -e rinex.nav ; bash\"");
				break;
			case 2:
				printf("The number of GPS satellites: %d\n", convbin());
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
				((int *)shm_addr)[0] = 3;
				break;
			case 10:
				break;
			default:
				printf("Enter 1~10\n");
				sleep(1);
		}

		if(n == 10)
			break;
	}
	
	shmdt(shm_addr);
	kill(pid, SIGINT);
	kill(pid, SIGKILL);
	kill(pid2, SIGINT);
	kill(pid2, SIGKILL);

	return 0;
}
