#include <stdio.h>

#include "../parser.h"

#define TEST_FAILED	0
#define TEST_OK		!TEST_FAILED


char *quality_vector[] = {

		/* SD TV */
		"Test.Show.S01E02.PDTV.XViD-GROUP",		(char*) QUALITY_SDTV,
		"Test.Show.S01E02.PDTV.x264-GROUP",		(char*) QUALITY_SDTV,
		"Test.Show.S01E02.HDTV.XViD-GROUP",		(char*) QUALITY_SDTV,
		"Test.Show.S01E02.HDTV.x264-GROUP",		(char*) QUALITY_SDTV,
		"Test.Show.S01E02.DSR.XViD-GROUP",		(char*) QUALITY_SDTV,
		"Test.Show.S01E02.DSR.x264-GROUP",		(char*) QUALITY_SDTV,
		"Test.Show.S01E02.TVRip.XViD-GROUP",	(char*) QUALITY_SDTV,
		"Test.Show.S01E02.TVRip.x264-GROUP",	(char*) QUALITY_SDTV,
		"Test.Show.S01E02.WEBRip.XViD-GROUP",	(char*) QUALITY_SDTV,
		"Test.Show.S01E02.WEBRip.x264-GROUP",	(char*) QUALITY_SDTV,

		/* SD DVD */

		"The.Walking.Dead.S01E01.Gute.alte.Zeit.UNCUT.BDRip.AC3.DL.German.XviD-ERC", (char*) QUALITY_SDDVD,


		/* UNKNOWN */
		"Test.Show.S01E02-MINIDRAGONFLY", (char *) QUALITY_UNKNOWN,

		NULL
};



int
test_quality_regex () {
	int i;

	quality_e quality;
	for ( i = 0; quality_vector[i] != NULL; i+=2) {
		quality = strqual( quality_vector[i] );

		if ( quality == quality_vector[i+1] ) {
			printf("OK\n");
		} else {
			printf("FAILED\n");
		}
	}

	return TEST_OK;
}




int main() {

	printf("Starting test suite\n");

	printf("Quality regex\n");
	test_quality_regex();

	return 0;
}

