#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <time.h>

int main(void)
{
	struct tm *tmp;
	pid_t pid;
	time_t curr_time;

	while(1)
	{
		curr_time = time(NULL);
		tmp = gmtime(&curr_time);
		if(tmp->tm_min % 5 == 4 && tmp->tm_sec == 0)
		{
			printf("str2str\n");
			pid = fork();
			if(pid == 0)
			{
				static char *argv2[] = {"str2str", "-c", "/home/syssec/usrpGPS/rinex/commands.txt", "-in", "file:///dev/ttyACM0#ubx", "-out", "file:///home/syssec/usrpGPS/rinex/test.ubx"};      
				execv("/home/syssec/Downloads/rtklib-qt-rtklib_2.4.3_qt/app/str2str/gcc/str2str", argv2);
				exit(127);
			}
			else
			{
				sleep(120);
				kill(pid, SIGINT);
				printf("Done!\n");
			}
		}
	}
	return 0;
}
