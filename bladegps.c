#define _CRT_SECURE_NO_WARNINGS

#include "bladegps.h"
#include <uhd.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>

#include <uhd/types/sensors.h>
#include <uhd/config.h>
#include <uhd/error.h>

// for _getch used in Windows runtime.
#ifdef WIN32
#include <conio.h>
#include "getopt.h"
#else
#include <unistd.h>
#endif

int flag = 0;

void init_sim(sim_t *s)
{
	//s->tx.dev = NULL;
	pthread_mutex_init(&(s->tx.lock), NULL);
	//s->tx.error = 0;

	pthread_mutex_init(&(s->gps.lock), NULL);
	//s->gps.error = 0;
	s->gps.ready = 0;
	pthread_cond_init(&(s->gps.initialization_done), NULL);

	s->status = 0;
	s->head = 0;
	s->tail = 0;
	s->sample_length = 0;

	pthread_cond_init(&(s->fifo_write_ready), NULL);
	pthread_cond_init(&(s->fifo_read_ready), NULL);

	s->time = 0.0;
}

size_t get_sample_length(sim_t *s)
{
	long length;

	length = s->head - s->tail;
	if (length < 0)
		length += FIFO_LENGTH;

	return((size_t)length);
}

size_t fifo_read(int16_t *buffer, size_t samples, sim_t *s)
{
	size_t length;
	size_t samples_remaining;
	int16_t *buffer_current = buffer;

	length = get_sample_length(s);

	if (length < samples)
		samples = length;

	length = samples; // return value

	samples_remaining = FIFO_LENGTH - s->tail;

	if (samples > samples_remaining) {
		memcpy(buffer_current, &(s->fifo[s->tail * 2]), samples_remaining * sizeof(int16_t) * 2);
		s->tail = 0;
		buffer_current += samples_remaining * 2;
		samples -= samples_remaining;
	}

	memcpy(buffer_current, &(s->fifo[s->tail * 2]), samples * sizeof(int16_t) * 2);
	s->tail += (long)samples;
	if (s->tail >= FIFO_LENGTH)
		s->tail -= FIFO_LENGTH;

	return(length);
}

bool is_finished_generation(sim_t *s)
{
	return s->finished;
}

int is_fifo_write_ready(sim_t *s)
{
	int status = 0;

	s->sample_length = get_sample_length(s);
	if (s->sample_length < NUM_IQ_SAMPLES)
		status = 1;

	return(status);
}

/*void *rinex_task(void *arg)
{
	sim_t *s = (sim_t *)arg;


out:
	return NULL;
}*/

void *tx_task(void *arg)
{
	sim_t *s = (sim_t *)arg;
	size_t samples_populated;
				
	s->status = uhd_set_thread_priority(1.0f, true);
	if(s->status != 0)
	{
		fprintf(stderr, "failed to set thread priority\n");
		goto out;
	}
	
	time_t full_secs, full_secs2, full_orig, cnt;
	double frac_secs, frac_secs2, frac_orig;
			
	/*s->status = uhd_usrp_get_time_last_pps(s->tx.usrp, 0, &full_secs, &frac_secs);
	do {
		s->status = uhd_usrp_get_time_last_pps(s->tx.usrp, 0, &full_secs2, &frac_secs2);
	}while(full_secs == full_secs2 && frac_secs == frac_secs2);
	s->status = uhd_usrp_set_time_next_pps(s->tx.usrp, 0, (double)0, 0);

	full_secs = frac_secs = 0;*/

	//full_secs = 0;
	//cnt = 2;

	while (1) {
		int16_t *tx_buffer_current = s->tx.buffer;
		unsigned int buffer_samples_remaining = SAMPLES_PER_BUFFER;

		while (buffer_samples_remaining > 0) {
			
			pthread_mutex_lock(&(s->gps.lock));
			while (get_sample_length(s) == 0)
			{
				pthread_cond_wait(&(s->fifo_read_ready), &(s->gps.lock));
			}
//			assert(get_sample_length(s) > 0);

			samples_populated = fifo_read(tx_buffer_current,
				buffer_samples_remaining,
				s);
			pthread_mutex_unlock(&(s->gps.lock));

			pthread_cond_signal(&(s->fifo_write_ready));
#if 0
			if (is_fifo_write_ready(s)) {
				/*
				printf("\rTime = %4.1f", s->time);
				s->time += 0.1;
				fflush(stdout);
				*/
			}
			else if (is_finished_generation(s))
			{
				goto out;
			}
#endif
			// Advance the buffer pointer.
			buffer_samples_remaining -= (unsigned int)samples_populated;
			tx_buffer_current += (2 * samples_populated);
		}

		if(!flag)
		{
			uhd_sensor_value_handle gpstime;
			//int gps_time;

			s->status = uhd_sensor_value_make_from_bool(&gpstime, "temp", 0, "true", "false");
			s->status = uhd_usrp_get_mboard_sensor(s->tx.usrp, "gps_time", 0, &gpstime);
			//printf("%d\n", s->status);
			s->status = uhd_sensor_value_to_int(gpstime, &full_secs); 
			printf("gps_time = %d\n", full_secs);
			s->status = uhd_usrp_set_time_next_pps(s->tx.usrp, full_secs + 1, 0, 0);
			sleep(1);
			full_secs++;
			uhd_sensor_value_free(&gpstime);
			flag = 1;
		}
		/*if(frac_secs == 0)
		{
			s->status = uhd_usrp_set_time_next_pps(s->tx.usrp, full_secs+1, 0, 0);
			if(s->status != 0) {
				fprintf(stderr, "Failed to set time next pps\n");
				goto out;
			}
		}*/

		// If there were no errors, transmit the data buffer.
		//bladerf_sync_tx(s->tx.dev, s->tx.buffer, SAMPLES_PER_BUFFER, NULL, TIMEOUT_MS);
		size_t num_samps_sent;
		uint64_t num_acc_samps = 0;
		
		frac_secs += 0.01;
		if(frac_secs >= 1.0)
		{
			frac_secs = 0;
			full_secs += 1;
		}

		s->status = uhd_tx_metadata_make(&s->tx.md, true, full_secs+1, frac_secs, false, false);
		if (s->status != 0) {
			fprintf(stderr, "Failed to create metadata\n");
			goto out;
		}
		
		/*s->status = uhd_tx_streamer_send(s->tx.tx_streamer, (const void **)&(s->tx.buffer), SAMPLES_PER_BUFFER, &s->tx.md, (float)TIMEOUT_MS / 1000, &num_samps_sent);
		if(s->status != 0)
		{
			fprintf(stderr, "Failed to send samples to USRP\n");
			goto out;
		}*/

		while(1) {
			if(num_acc_samps >= SAMPLES_PER_BUFFER) break;
			
			/*if(!flag)
			{
				s->status = uhd_usrp_set_time_unknown_pps(s->tx.usrp, 0, 0);
				s->status = uhd_usrp_get_time_last_pps(s->tx.usrp, 0, &full_orig, &frac_orig);
				//printf("%d, %f\n", full_orig, frac_orig);
				flag = 1;
			}
			else
			{
				s->status = uhd_usrp_get_time_last_pps(s->tx.usrp, 0, &full_secs2, &frac_secs2);
				if(full_orig != full_secs2 || frac_orig != frac_secs2)
				{
					//s->status = uhd_usrp_set_time_next_pps(s->tx.usrp, cnt, 0, 0);
					//cnt++;
					//printf("%d, %f\n", full_secs2, frac_secs2);
					full_orig = full_secs2;
					frac_orig = frac_secs2;
				}
			}*/
		
			s->status = uhd_tx_streamer_send(s->tx.tx_streamer, (const void **)&(s->tx.buffer), SAMPLES_PER_BUFFER, &s->tx.md, (float)0.01, &num_samps_sent);	
			
			if(s->status != 0)
			{
				fprintf(stderr, "Failed to send samples to USRP\n");
				goto out;
			}

			num_acc_samps += num_samps_sent;
		}

		if (is_fifo_write_ready(s)) {
			/*
			printf("\rTime = %4.1f", s->time);
			s->time += 0.1;
			fflush(stdout);
			*/
		}
		else if (is_finished_generation(s))
		{
			goto out;
		}

	}
out:
	return NULL;
}

int start_tx_task(sim_t *s)
{
	int status;

	status = pthread_create(&(s->tx.thread), NULL, tx_task, s);

	return(status);
}

int start_gps_task(sim_t *s)
{
	int status;

	status = pthread_create(&(s->gps.thread), NULL, gps_task, s);

	return(status);
}

/*int start_rinex_task(sim_t *s)
{
	int status;

	status = pthread_create(&(s->rinex_thread), NULL, rinex_task, s);

	return(status);
}*/

void usage(void)
{
	printf("Usage: bladegps [options]\n"
		"Options:\n"
		"  -e <gps_nav>     RINEX navigation file for GPS ephemerides (required)\n"
		"  -u <user_motion> User motion file (dynamic mode)\n"
		"  -g <nmea_gga>    NMEA GGA stream (dynamic mode)\n"
		"  -l <location>    Lat,Lon,Hgt (static mode) e.g. 35.274,137.014,100\n"
		"  -t <date,time>   Scenario start time YYYY/MM/DD,hh:mm:ss\n"
		"  -T <date,time>   Overwrite TOC and TOE to scenario start time\n"
		"  -d <duration>    Duration [sec] (max: %.0f)\n"
		"  -x <XB number>   Enable XB board, e.g. '-x 200' for XB200\n"
		"  -a <tx_vga1>     TX VGA1 (default: %d)\n"
		"  -i               Interactive mode: North='%c', South='%c', East='%c', West='%c'\n"
		"  -I               Disable ionospheric delay for spacecraft scenario\n",
		((double)USER_MOTION_SIZE)/10.0, 
		TX_VGA1,
		NORTH_KEY, SOUTH_KEY, EAST_KEY, WEST_KEY);

	return;
}

int main(int argc, char *argv[])
{
	sim_t s;
	char *devstr = NULL;
	int xb_board=0;

	int result;
	double duration;
	datetime_t t0;

	char *device_args = "";

	int txvga1 = TX_VGA1;

	if (argc<3)
	{
		usage();
		exit(1);
	}
	s.finished = false;

	s.opt.navfile[0] = 0;
	s.opt.umfile[0] = 0;
	s.opt.g0.week = -1;
	s.opt.g0.sec = 0.0;
	s.opt.iduration = USER_MOTION_SIZE;
	s.opt.verb = TRUE;
	s.opt.nmeaGGA = FALSE;
	s.opt.staticLocationMode = TRUE; // default user motion
	s.opt.llh[0] = 40.7850916 / R2D;
	s.opt.llh[1] = -73.968285 / R2D;
	s.opt.llh[2] = 100.0;
	s.opt.interactive = FALSE;
	s.opt.timeoverwrite = FALSE;
	s.opt.iono_enable = TRUE;

	while ((result=getopt(argc,argv,"a:e:u:g:l:T:t:d:x:a:iI"))!=-1)
	{
		switch (result)
		{
		case 'a':
			device_args = strdup(optarg);
			break;
		case 'e':
			strcpy(s.opt.navfile, optarg);
			break;
		case 'u':
			strcpy(s.opt.umfile, optarg);
			s.opt.nmeaGGA = FALSE;
			s.opt.staticLocationMode = FALSE;
			break;
		case 'g':
			strcpy(s.opt.umfile, optarg);
			s.opt.nmeaGGA = TRUE;
			s.opt.staticLocationMode = FALSE;
			break;
		case 'l':
			// Static geodetic coordinates input mode
			// Added by scateu@gmail.com
			s.opt.nmeaGGA = FALSE;
			s.opt.staticLocationMode = TRUE;
			sscanf(optarg,"%lf,%lf,%lf",&s.opt.llh[0],&s.opt.llh[1],&s.opt.llh[2]);
			s.opt.llh[0] /= R2D; // convert to RAD
			s.opt.llh[1] /= R2D; // convert to RAD
			break;
		case 'T':
			s.opt.timeoverwrite = TRUE;
			if (strncmp(optarg, "now", 3)==0)
			{
				time_t timer;
				struct tm *gmt;
				
				time(&timer);
				gmt = gmtime(&timer);

				t0.y = gmt->tm_year+1900;
				t0.m = gmt->tm_mon+1;
				t0.d = gmt->tm_mday;
				t0.hh = gmt->tm_hour;
				t0.mm = gmt->tm_min;
				t0.sec = (double)gmt->tm_sec;

				date2gps(&t0, &s.opt.g0);

				break;
			}
		case 't':
			sscanf(optarg, "%d/%d/%d,%d:%d:%lf", &t0.y, &t0.m, &t0.d, &t0.hh, &t0.mm, &t0.sec);
			if (t0.y<=1980 || t0.m<1 || t0.m>12 || t0.d<1 || t0.d>31 ||
				t0.hh<0 || t0.hh>23 || t0.mm<0 || t0.mm>59 || t0.sec<0.0 || t0.sec>=60.0)
			{
				printf("ERROR: Invalid date and time.\n");
				exit(1);
			}
			t0.sec = floor(t0.sec);
			date2gps(&t0, &s.opt.g0);
			break;
		case 'd':
			duration = atof(optarg);
			if (duration<0.0 || duration>((double)USER_MOTION_SIZE)/10.0)
			{
				printf("ERROR: Invalid duration.\n");
				exit(1);
			}
			s.opt.iduration = (int)(duration*10.0+0.5);
			break;
		case 'x':
			xb_board=atoi(optarg);
			break;
		case 'i':
			s.opt.interactive = TRUE;
			break;
		case 'I':
			s.opt.iono_enable = FALSE; // Disable ionospheric correction
			break;
		case ':':
		case '?':
			usage();
			exit(1);
		default:
			break;
		}
	}

	if (s.opt.navfile[0]==0)
	{
		printf("ERROR: GPS ephemeris file is not specified.\n");
		exit(1);
	}

	if (s.opt.umfile[0]==0 && !s.opt.staticLocationMode)
	{
		printf("ERROR: User motion file / NMEA GGA stream is not specified.\n");
		printf("You may use -l to specify the static location directly.\n");
		exit(1);
	}

	if(uhd_set_thread_priority(1.0f, true)) {
		fprintf(stderr, "Unable to set thread priority. Continuing anyway.\n");
	}

	// Initialize simulator
	init_sim(&s);

	// Allocate TX buffer to hold each block of samples to transmit.
	s.tx.buffer = (int16_t *)malloc(SAMPLES_PER_BUFFER * sizeof(int16_t) * 2); // for 16-bit I and Q samples
	
	if (s.tx.buffer == NULL) {
		fprintf(stderr, "Failed to allocate TX buffer.\n");
		goto out;
	}

	// Allocate FIFOs to hold 0.1 seconds of I/Q samples each.
	s.fifo = (int16_t *)malloc(FIFO_LENGTH * sizeof(int16_t) * 2); // for 16-bit I and Q samples

	if (s.fifo == NULL) {
		fprintf(stderr, "Failed to allocate I/Q sample buffer.\n");
		goto out;
	}

	// Initializing device.
	printf("Opening and initializing device...\n");

	//s.status = bladerf_open(&s.tx.dev, devstr);
	s.status = uhd_usrp_make(&s.tx.usrp, device_args);
	if (s.status != 0) {
		//fprintf(stderr, "Failed to open device: %s\n", bladerf_strerror(s.status));
		fprintf(stderr, "Failed to open device\n");
		goto out;
	}

	s.status = uhd_tx_streamer_make(&s.tx.tx_streamer);
	if (s.status != 0) {	
		fprintf(stderr, "Failed to create TX streamer\n");
		goto out;
	}
	
	//(handle, has_time_spec, full_secs, frac_secs, start_of_burst, end_of_burst)
	s.status = uhd_tx_metadata_make(&s.tx.md, true, 0.0, 0.1, false, false);
	if (s.status != 0) {
		fprintf(stderr, "Failed to create metadata\n");
		goto out;
	}

	/*if(xb_board == 200) {
		printf("Initializing XB200 expansion board...\n");

		s.status = bladerf_expansion_attach(s.tx.dev, BLADERF_XB_200);
		if (s.status != 0) {
			fprintf(stderr, "Failed to enable XB200: %s\n", bladerf_strerror(s.status));
			goto out;
		}

		s.status = bladerf_xb200_set_filterbank(s.tx.dev, BLADERF_MODULE_TX, BLADERF_XB200_CUSTOM);
		if (s.status != 0) {
			fprintf(stderr, "Failed to set XB200 TX filterbank: %s\n", bladerf_strerror(s.status));
			goto out;
		}

		s.status = bladerf_xb200_set_path(s.tx.dev, BLADERF_MODULE_TX, BLADERF_XB200_BYPASS);
		if (s.status != 0) {
			fprintf(stderr, "Failed to enable TX bypass path on XB200: %s\n", bladerf_strerror(s.status));
			goto out;
		}

		//For sake of completeness set also RX path to a known good state.
		s.status = bladerf_xb200_set_filterbank(s.tx.dev, BLADERF_MODULE_RX, BLADERF_XB200_CUSTOM);
		if (s.status != 0) {
			fprintf(stderr, "Failed to set XB200 RX filterbank: %s\n", bladerf_strerror(s.status));
			goto out;
		}

		s.status = bladerf_xb200_set_path(s.tx.dev, BLADERF_MODULE_RX, BLADERF_XB200_BYPASS);
		if (s.status != 0) {
			fprintf(stderr, "Failed to enable RX bypass path on XB200: %s\n", bladerf_strerror(s.status));
			goto out;
		}
	}

	if(xb_board == 300) {
		fprintf(stderr, "XB300 does not support transmitting on GPS frequency\n");
		goto out;
	}
	*/

	//s.status = bladerf_set_frequency(s.tx.dev, BLADERF_MODULE_TX, TX_FREQUENCY);
	uhd_tune_request_t tune_request = {
			.target_freq = TX_FREQUENCY,
			.rf_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO,
			.dsp_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO
	};
	
	uhd_tune_result_t tune_result;
	size_t channel = 0;

	s.status = uhd_usrp_set_tx_freq(s.tx.usrp, &tune_request, channel, &tune_result);
	if (s.status != 0) {
		//fprintf(stderr, "Faield to set TX frequency: %s\n", bladerf_strerror(s.status));
		fprintf(stderr, "Failed to set TX frequency\n");
		goto out;
	} 
	else {
		printf("Setting TX frequency: %u Hz\n", TX_FREQUENCY);
	}

	double freq;
	s.status = uhd_usrp_get_tx_freq(s.tx.usrp, channel, &freq);
	if (s.status != 0) {
		fprintf(stderr, "Failed to get TX frequency\n");
		goto out;
	}
	else {
		printf("Actual TX frequency: %f Hz\n", freq);
	}

	double rate = TX_SAMPLERATE;

	//s.status = bladerf_set_sample_rate(s.tx.dev, BLADERF_MODULE_TX, TX_SAMPLERATE, NULL);
	s.status = uhd_usrp_set_tx_rate(s.tx.usrp, rate, channel);

	if (s.status != 0) {
		fprintf(stderr, "Failed to set TX sample rate\n");
		goto out;
	}
	else {
		printf("Setting TX sample rate: %u sps\n", TX_SAMPLERATE);
	}
	
	s.status = uhd_usrp_get_tx_rate(s.tx.usrp, channel, &rate);

	if (s.status != 0) {
		fprintf(stderr, "Failed to get TX sample rate\n");
		goto out;
	}
	else {
		printf("Actual TX sample rate: %f sps\n", rate);
	}

	/*s.status = bladerf_set_bandwidth(s.tx.dev, BLADERF_MODULE_TX, TX_BANDWIDTH, NULL);
	if (s.status != 0) {
		fprintf(stderr, "Failed to set TX bandwidth: %s\n", bladerf_strerror(s.status));
		goto out;
	}
	else {
		printf("TX bandwidth: %u Hz\n", TX_BANDWIDTH);
	}

	//s.status = bladerf_set_txvga1(s.tx.dev, TX_VGA1);
	s.status = bladerf_set_txvga1(s.tx.dev, txvga1);
	if (s.status != 0) {
		fprintf(stderr, "Failed to set TX VGA1 gain: %s\n", bladerf_strerror(s.status));
		goto out;
	}
	else {
		//printf("TX VGA1 gain: %d dB\n", TX_VGA1);
		printf("TX VGA1 gain: %d dB\n", txvga1);
	}

	s.status = bladerf_set_txvga2(s.tx.dev, TX_VGA2);
	if (s.status != 0) {
		fprintf(stderr, "Failed to set TX VGA2 gain: %s\n", bladerf_strerror(s.status));
		goto out;
	}
	else {
		printf("TX VGA2 gain: %d dB\n", TX_VGA2);
	}*/

	double gain = 20;

	s.status = uhd_usrp_set_tx_gain(s.tx.usrp, gain, 0, "");

	if (s.status != 0) {
		fprintf(stderr, "Failed to set TX gain\n");
		goto out;
	}
	else {
		printf("Setting TX gain: %f sps\n", gain);
	}
	
	s.status = uhd_usrp_get_tx_gain(s.tx.usrp, channel, "", &gain);

	if (s.status != 0) {
		fprintf(stderr, "Failed to get TX gain\n");
		goto out;
	}
	else {
		printf("Actual TX gain: %f sps\n", gain);
	}

	double bw = 2.5e6;
	s.status = uhd_usrp_set_tx_bandwidth(s.tx.usrp, bw, 0);
	if (s.status != 0) {
		fprintf(stderr, "Failed to set TX BW\n");
		goto out;
	}
	else {
		printf("Setting TX BW: %f Hz\n", bw);
	}
	
	s.status = uhd_usrp_get_tx_bandwidth(s.tx.usrp, channel, &bw);

	if (s.status != 0) {
		fprintf(stderr, "Failed to get TX BW\n");
		goto out;
	}
	else {
		printf("Actual TX BW: %f Hx\n", bw);
	}



	uhd_stream_args_t stream_args = {
		.cpu_format = "sc16",
		.otw_format = "sc16",
		.args = "",
		.channel_list = &channel,
		.n_channels = 1
	};

	s.status = uhd_usrp_get_tx_stream(s.tx.usrp, &stream_args, s.tx.tx_streamer);
	if (s.status != 0) {
		fprintf(stderr, "Failed to set up streamer\n");
		goto out;
	}

	size_t samps_per_buff = SAMPLES_PER_BUFFER;
	s.status = uhd_tx_streamer_max_num_samps(s.tx.tx_streamer, &samps_per_buff);
	if (s.status != 0) {
		fprintf(stderr, "Failed to set up buffer");
		goto out;
	}
	
	s.status = uhd_usrp_set_time_source(s.tx.usrp, "gpsdo", 0);
	s.status = uhd_usrp_set_clock_source(s.tx.usrp, "gpsdo", 0);
	if(s.status != 0) {
		fprintf(stderr, "Failed to set time source\n");
		goto out;
	}

	uhd_sensor_value_handle sensor;
	char val_str[80];
	char *pch, *ptrs[80];
	int nums = 0;
	float lat, lng, hgt, intpart, frac;
	s.status = uhd_sensor_value_make_from_bool(&sensor, "temp", 0, "true", "false");
	uhd_usrp_get_mboard_sensor(s.tx.usrp, "gps_gpgga", 0, &sensor);
	uhd_sensor_value_to_pp_string(sensor, val_str, 128);
	printf("\n%s\n", val_str);

	pch = strtok(val_str, ",");
	ptrs[nums++] = pch;

	while(pch != NULL)
	{
		pch = strtok(NULL, ",");
		ptrs[nums++] = pch;
	}

	frac = modff(atof(ptrs[2]) / 100, &intpart);
	lat = intpart + frac * 100 / 60;
	frac = modff(atof(ptrs[4]) / 100, &intpart);
	lng = intpart + frac * 100 / 60;
	hgt = atof(ptrs[11]);

	if(strcmp(ptrs[3], "N")) lat *= (-1);
	if(strcmp(ptrs[5], "E")) lng *= (-1);
			
	s.opt.llh[0] = lat / R2D; // convert to RAD
	s.opt.llh[1] = lng / R2D; // convert to RAD
	s.opt.llh[2] = hgt;

	// Start GPS task.
	s.status = start_gps_task(&s);
	if (s.status < 0) {
		fprintf(stderr, "Failed to start GPS task.\n");
		goto out;
	}
	else
		printf("Creating GPS task...\n");

	// Wait until GPS task is initialized
	pthread_mutex_lock(&(s.tx.lock));
	while (!s.gps.ready)
		pthread_cond_wait(&(s.gps.initialization_done), &(s.tx.lock));
	pthread_mutex_unlock(&(s.tx.lock));

	// Fillfull the FIFO.
	if (is_fifo_write_ready(&s))
		pthread_cond_signal(&(s.fifo_write_ready));

	// Configure the TX module for use with the synchronous interface.
	/*s.status = bladerf_sync_config(s.tx.dev,
			BLADERF_MODULE_TX,
			BLADERF_FORMAT_SC16_Q11,
			NUM_BUFFERS,
			SAMPLES_PER_BUFFER,
			NUM_TRANSFERS,
			TIMEOUT_MS);

	if (s.status != 0) {
		fprintf(stderr, "Failed to configure TX sync interface: %s\n", bladerf_strerror(s.status));
		goto out;
	}

	// We must always enable the modules *after* calling bladerf_sync_config().
	s.status = bladerf_enable_module(s.tx.dev, BLADERF_MODULE_TX, true);
	if (s.status != 0) {
		fprintf(stderr, "Failed to enable TX module: %s\n", bladerf_strerror(s.status));
		goto out;
	}*/

	//s.tx.usrp->set_time_source("external");
	//s.tx.usrp->set_time_unknown_pps(time_spec_t(0.0));


	/*s.status = start_rinex_task(&s);
	if (s.status < 0) {
		fprintf(stderr, "Failed to start RINEX task.\n");
		goto out;
	}
	else
		printf("Creating RINEX task...\n");*/

	// Start TX task
	s.status = start_tx_task(&s);
	if (s.status < 0) {
		fprintf(stderr, "Failed to start TX task.\n");
		goto out;
	}
	else
		printf("Creating TX task...\n");

	// Running...
	printf("Running...\n");
	printf("Press 'Ctrl+C' to abort.\n");
	
	// Wainting for TX task to complete.
	pthread_join(s.tx.thread, NULL);
	printf("\nDone!\n");

	// Disable TX module and shut down underlying TX stream.
	/*s.status = bladerf_enable_module(s.tx.dev, BLADERF_MODULE_TX, false);
	if (s.status != 0)
		fprintf(stderr, "Failed to disable TX module: %s\n", bladerf_strerror(s.status));
	*/
	
out:
	// Free up resources
	if (s.tx.buffer != NULL)
		free(s.tx.buffer);

	if (s.fifo != NULL)
		free(s.fifo);

	printf("Closing device...\n");
	//bladerf_close(s.tx.dev);

	return(0);
}
