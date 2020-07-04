#pragma once

#include <complex.h>

//http://www.ga.gov.au/__data/assets/pdf_file/0019/11719/GA10287.pdf

typedef struct ERS_Raw_Parser_Ctx ERS_Raw_Parser_Ctx;

typedef struct ERS_Raw_Parser_Data_Patch {
	int n_ra;
	int n_az;
	int az_pos;
	double complex *data;
} ERS_Raw_Parser_Data_Patch;

typedef struct {
	char missionid[17];
	double lambda;
	double ra_ph_off; //Range pulse phase offset [rad]
	double kr; //Range chirp slope [Hz/s]
	double ka; //Azimuth chirp slope [Hz/s]
	double fs;
	double tau;
	double prf;
	double rgd;
	double velocity;
	double range_pixel_spacing;
	double C; //lightspeed
	double r0;
	double az_beam_width; //lambda/L [rad]

	int n_valid_samples;
	int ra_fft_len;
	int fft_lines;
} ERS_Raw_Parser_Params;

/**
 * Allocates context and open files to read
 */
ERS_Raw_Parser_Ctx * ers_raw_parser_alloc(char *path_ldr, char *path_raw);

/**
 * Frees context and closes opened files
 */
void ers_raw_parser_free(ERS_Raw_Parser_Ctx *ctx);

int ers_raw_parser_get_params_from_file(ERS_Raw_Parser_Ctx *ctx, ERS_Raw_Parser_Params *out);

/**
 * Get raw data patch
 * @param[in] start_line: Line from where to read, not counting first DATA_FILE_RAW_SIGNAL_RECORD_SIZE bytes
 * @param[out] data: Output data patch, must be freed with ers_raw_parser_data_patch_free()
 * @return 0 if OK, < 0 on error, 1 Incomplete read (EOF)
 */
int ers_raw_parser_get_raw_data_from_file(ERS_Raw_Parser_Ctx *ctx, ERS_Raw_Parser_Data_Patch **data, int start_line);

void ers_raw_parser_data_patch_free(ERS_Raw_Parser_Data_Patch *data);

int log_ers_params(ERS_Raw_Parser_Params *par);
