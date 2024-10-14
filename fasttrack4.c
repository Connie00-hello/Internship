#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <float.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

/*
	currentTime
	print the current date and time
*/
int currentTime() {
    time_t timer;
    char datetime[26];
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(datetime, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    fprintf(stdout, "%lu", (unsigned long)time(NULL));
    //printf("%s", datetime);

    return 0;
}

/* 
	boxMuller
	generate normal random variate using the Box-Muller method 
	mean m, standard deviation s
*/
double boxMuller(double mu, double sigma) {

	static double z0, z1;
	static int previous;

	if (previous) {
		previous = 0;
		return z1 * sigma + mu;
	} else {
		double u1, u2;
		do {
			u1 = rand() * (1.0 / RAND_MAX);
			u2 = rand() * (1.0 / RAND_MAX);
		} while ( u1 > 1.0 );

		z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
		z1 = sqrt(-2.0 * log(u1)) * sin(2.0 * M_PI * u2);
		
		previous = 1;
		
		return z0 * sigma + mu;
	}
}

/*
	logCurve
	Devices reporting failure will see values drift upwards over a sigmoid logistic curve
	x-value x, target K, slope B
*/
float logCurve(int x, float K, float B) {
     float y;

     // Get sigmoid value
     y = (K - 1) / (1 + exp((double) -B * x));

     return y;
}

/*
	main
	parse command line arguments and control program flow
*/
int main (int argc, char *argv[]) {
	errno = 0;
	
	/* read the argument list */
	// argv[0] is the program name

	// argv[1] the asset id; for man page, type "help"
	char *app = argv[1];
	if (strncmp(app, "help", 4) == 0) {
		printf("usage: fasttrack [1] [2] [3] [4] [5] [6] [[7] [8] [9] [10] [11] [12]..]\n");
		printf("argument [1]: number of assets (int)\n");
		printf("argument [2]: asset start number (int)\n");
		printf("argument [3]: sleep between readings in sec (int)\n");
		printf("argument [4]: the number of reading to take before quitting (int)\n");
		printf("argument [5]: the probability of an asset failure (float 0-1)\n");
		printf("argument [6]: maintenance probability factor (float >0)\n");
		printf("argument [7]: the number of components in the asset (int)\n");
		printf("argument [8]: the type of component (char)\n");
		printf("argument [9]: the normal expected reading of the component (float)\n");
		printf("argument [10]: the expected variance of the component readings (float)\n");
		printf("argument [11]: the number of iterations in fail mode before asset fails (int)\n");
		printf("argument [12]: the target reading for a component in fail mode (float)\n");
		printf("repeat arguments [8], [9], [10], [11] and [12] the number of times specified in argument [6]\n");
		return EXIT_SUCCESS;
	}
	if (argc < 11) {
		printf("error: not enough arguments given\ntype 'fasttrack help' for help\n");
		return EXIT_FAILURE;
	}

	// argv[1] number of assets
	char *zn;
	int n = strtol(argv[1], &zn, 10);
	if (errno != 0 || *zn != '\0' || n > INT_MAX) {
		printf("error: expecting an integer for number argument [1], instead you put %s\ntype 'fasttrack help' for help\n", argv[1]);
		return EXIT_FAILURE;
	}

	// argv[2] asset start number
	char *zsid;
	int sid = strtol(argv[2], &zsid, 10);
	if (errno != 0 || *zsid != '\0' || sid > INT_MAX) {
		printf("error: expecting an integer for id argument [2], instead you put %s\ntype 'fasttrack help' for help\n", argv[2]);
		return EXIT_FAILURE;
	}

	// argv[3] sleep between readings in sec
	char *zsec;
	int sec = strtol(argv[3], &zsec, 10);
	if (errno != 0 || *zsec != '\0' || sec > INT_MAX) {
		printf("error: expecting an integer for sleep argument [3], instead you put %s\ntype 'fasttrack help' for help\n", argv[3]);
		return EXIT_FAILURE;
	}

	// argv[4] the number of reading to take before quitting
	char *ziter;
	int iter = strtol(argv[4], &ziter, 10);
	if (errno != 0 || *ziter != '\0' || iter > INT_MAX ) {
		printf("error: expecting an integer for iterations argument [4], instead you put %s\ntype 'fasttrack help' for help\n", argv[4]);
		return EXIT_FAILURE;
	}

	// argv[5] the probability of a component failure
	char *zprob;
	float prob = strtof(argv[5], &zprob);
	if (errno != 0 || *zprob != '\0' || prob < 0 || prob > 1) {
		printf("error: expecting a float between 0 and 1 for probability argument [5], instead you put %s\ntype 'fasttrack help' for help\n", argv[5]);
		return EXIT_FAILURE;
	}

	// argv[6] the maintenance probability factor
	char *zmaintp;
	float maintp = strtof(argv[6], &zmaintp);
	if (errno != 0 || *zmaintp != '\0') {
		printf("error: expecting a float for maintenance argument [6], instead you put %s\ntype 'fasttrack help' for help\n", argv[6]);
		return EXIT_FAILURE;
	}

	// argv[7] the number of components in the asset
	char *zcomc;
	int comc = strtol(argv[7], &zcomc, 10);
	if (errno != 0 || *zcomc != '\0' || comc > INT_MAX ) {
		printf("error: expecting an integer for components argument [7], instead you put %s\ntype 'fasttrack help' for help\n", argv[7]);
		return EXIT_FAILURE;
	}
	// declare arrays to hold remaining arguments
	int arrargc = 5; //number of repeating arguments
	char comname[comc][14];
	float norm[comc];
	float vari[comc];
	int iter2fail[comc];
	float failtarget[comc];
	if (argc < 8 + arrargc * comc) {
		printf("error: not enough components specified\ntype 'fasttrack help' for help\n");
		return EXIT_FAILURE;
	}

	//parse the remaining arguments
	int i;
	int argi;
	int failure[n][comc];
	float failfactor[n][comc];
	for (i = 0; i < comc; i++) {
		
		// argv[8..] the component type
		argi = 8 + i * arrargc;
		char *com = argv[argi];
		strcpy(comname[i], com);

		// argv[9..] the normal expected reading of the component
		argi = 9 + i * arrargc;
		char *znorm;
		norm[i] = strtof(argv[argi], &znorm);
		if (errno != 0 || *znorm != '\0' || norm[i] > FLT_MAX) {
			printf("error: expecting a float for norm argument [%d], instead you put %s\ntype 'fasttrack help' for help\n", argi, argv[argi]);
			return EXIT_FAILURE;
		}

		// argv[10..] the expected variance of the component readings
		argi = 10 + i * arrargc;
		char *zvari;
		vari[i] = strtof(argv[argi], &zvari);
		if (errno != 0 || *zvari != '\0' || vari[i] > FLT_MAX) {
			printf("error: expecting a float for variance argument [%d], instead you put %s\ntype 'fasttrack help' for help\n", argi, argv[argi]);
			return EXIT_FAILURE;
		}

		// argv[11..] the number of iterations in fail mode before an asset fails
		argi = 11 + i * arrargc;
		char *ziter2fail;
		iter2fail[i] = strtol(argv[argi], &ziter2fail, 10);
		if (errno != 0 || *ziter2fail != '\0' || iter2fail[i] > INT_MAX ) {
			printf("error: expecting an integer for iterations to fail argument [%d], instead you put %s\ntype 'fasttrack help' for help\n", argi, argv[argi]);
			return EXIT_FAILURE;
		}

		// argv[12..] the target reading for a component in fail mode
		argi = 12 + i * arrargc;
		char *zfailtarget;
		failtarget[i] = strtof(argv[argi], &zfailtarget);
		if (errno != 0 || *zfailtarget != '\0' || failtarget[i] > FLT_MAX) {
			printf("error: expecting a float for target argument [%d], instead you put %s\ntype 'fasttrack help' for help\n", argi, argv[argi]);
			return EXIT_FAILURE;
		}

	}
	// if no errors, proceed:
	
	// seed random number generator 
	time_t t;
	unsigned int randval;
	FILE *f;

	f = fopen("/dev/random", "r");
	fread(&randval, sizeof(randval), 1, f);
	fclose(f);

    srand(randval);
	
	// status indicators
	int start = (unsigned)time(NULL); //start time
	int currenttime = 0;
	int status[n]; // asset status
	int failed[n]; // number of failed components in an asset
	int failedcom = 0; // component that failed last
	int failc[n]; // count of iterations in fail mode
	float sig[n][comc]; // sigmoid decay value for a failing component
	float reading[n][comc]; // asset reading
	int needsmaint[n]; // asset in need of maintenance
	int maintc[n]; // count of iterations of needing maintenance
	int maintevt[n]; // signals a maintenance event for the asset
	int maintfac; // random maintenance factor
	
	// loop counters
	int j;
	int k;
	int m;
	for (m = 0; m < n; m++) {
		maintevt[m] = 0;
		maintc[m] = 0;
		failed[m] = 0;
		failc[m] = 0;
		status[m] = 0;
		for (k = 0; k < comc; k++) {
			failfactor[m][k] = 1.0;
			failure[m][k] = 0;
		}
		failfactor[m][rand() % comc] = 2.0;
		// give one component in each asset a higher likelihood of failure
	}	

	for (j = 0; j < iter; j++) {
	// iterate through the number of total iterations
		
		currenttime = (unsigned)time(NULL) - start;

		for (m = 0; m < n; m++) {
		// iterate through each asset

			// print component id, timestamp, and status
			printf("%d|", sid + m);
			printf("%u|", currenttime);
			printf("%d|", status[m]); //print binary status for model building
			
			// asset change status to FAIL if all components in failure mode
			if (failed[m] >= comc) {
				failc[m]++;
				if (failc[m] >= iter2fail[failedcom]) {
					status[m] = 1;
				}
			}

			// asset needs maintenance
			if (failed[m] >= comc - 2) {
				maintc[m]++;
			}

			// loop through each component
			for (k = 0; k < comc; k++) {

				// signal a maintenance event at a random interval
				maintfac = iter2fail[k] + iter2fail[k] * (rand() * (iter2fail[k] / 3.0 / RAND_MAX) + maintp);
				if (failure[m][k] >= 1 && maintc[m] > maintfac) {
					maintevt[m] = 1;
				}
				// possibly put a component in failure mode
				if (failure[m][k] == 0 && prob * failfactor[m][k] > rand() * (1.0 / RAND_MAX)) {
					failure[m][k] = 1;
					failedcom = k;
					failed[m]++;
				}
				// component reporting in failure mode
				if (failure[m][k] >= 1) {
					sig[m][k] = norm[k] + logCurve(failure[m][k] - iter2fail[k] / 2, failtarget[k] - norm[k] + 1, 6.0 / iter2fail[k]);
					reading[m][k] = boxMuller(sig[m][k], vari[k]);
					failure[m][k]++;
				}
				// component reporting in normal mode
				if (failure[m][k] == 0) {
					reading[m][k] = boxMuller(norm[k], vari[k]);
				}
				// print component reading
				printf("%s|%.6f", comname[k], reading[m][k]);
				if (k == comc - 1) {
					//score = 1 / (1 + exp(-(-53.9856 + 1.5331 * reading[0] - 174.0193 * reading[1] - 71.5102 * reading[2])));
					//printf("|%.3f\n", score);
					printf("|%d\n", maintevt[m]);
				} else {
					printf("|");
				}
							
			}
		
			// reset asset after a maintenance event
			if (maintevt[m] == 1) {
				maintevt[m] = 0;
				maintc[m] = 0;
				failed[m] = 0;
				failc[m] = 0;
				status[m] = 0;
				for (k = 0; k < comc; k++) {
					failure[m][k] = 0;
				}
			}
						
		}

		// go to sleep for the specified number of sec
		if (sec > 0) {
			sleep(sec);
		}

	}
	return EXIT_SUCCESS;
}