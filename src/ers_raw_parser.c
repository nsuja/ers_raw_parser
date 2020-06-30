#include "ers_raw_parser.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

#define LEADER_FILE_DESCRIPTOR_RECORD_SIZE (720)
#define LEADER_FILE_DATASET_MISSION_ID_POS (LEADER_FILE_DESCRIPTOR_RECORD_SIZE+396)
#define LEADER_FILE_DATASET_MISSION_ID_SIZE (16)
#define LEADER_FILE_DATASET_RADAR_WAVELENGTH_POS (LEADER_FILE_DESCRIPTOR_RECORD_SIZE+500)
#define LEADER_FILE_DATASET_RADAR_WAVELENGTH_SIZE (16)
#define LEADER_FILE_DATASET_RANGE_CHIRP_SLOPE_POS (LEADER_FILE_DESCRIPTOR_RECORD_SIZE+550)
#define LEADER_FILE_DATASET_RANGE_CHIRP_SLOPE_SIZE (16)
#define LEADER_FILE_DATASET_RANGE_CHIRP_PHASE_OFFSET_POS (LEADER_FILE_DESCRIPTOR_RECORD_SIZE+615)
#define LEADER_FILE_DATASET_RANGE_CHIRP_PHASE_OFFSET_SIZE (16)
#define LEADER_FILE_DATASET_RANGE_SAMPLING_FREC_POS (LEADER_FILE_DESCRIPTOR_RECORD_SIZE+710)
#define LEADER_FILE_DATASET_RANGE_SAMPLING_FREC_SIZE (16)
#define LEADER_FILE_DATASET_RANGE_PULSE_LENGTH_POS (LEADER_FILE_DESCRIPTOR_RECORD_SIZE+742)
#define LEADER_FILE_DATASET_RANGE_PULSE_LENGTH_SIZE (16)
#define LEADER_FILE_DATASET_PRF_POS (LEADER_FILE_DESCRIPTOR_RECORD_SIZE+934)
#define LEADER_FILE_DATASET_PRF_SIZE (16)
#define LEADER_FILE_DATASET_AZIMUTH_BEAM_WIDTH_POS (LEADER_FILE_DESCRIPTOR_RECORD_SIZE+966)
#define LEADER_FILE_DATASET_AZIMUTH_BEAM_WIDTH_SIZE (16)
#define LEADER_FILE_DATASET_RANGE_BANDWIDTH_POS (LEADER_FILE_DESCRIPTOR_RECORD_SIZE+1254)
#define LEADER_FILE_DATASET_RANGE_BANDWIDTH_SIZE (16)
#define LEADER_FILE_DATASET_RGD_POS (LEADER_FILE_DESCRIPTOR_RECORD_SIZE+1766)
#define LEADER_FILE_DATASET_RGD_SIZE (16)
#define LEADER_FILE_DATA_PROCESSOR_SPECIFIC_LOCAL_SEGMENT_POS (LEADER_FILE_DESCRIPTOR_RECORD_SIZE+1886)
#define LEADER_FILE_DATASET_VELOCITY_X_POS (LEADER_FILE_DATA_PROCESSOR_SPECIFIC_LOCAL_SEGMENT_POS+452)
#define LEADER_FILE_DATASET_VELOCITY_X_SIZE (22)
#define LEADER_FILE_DATASET_VELOCITY_Y_POS (LEADER_FILE_DATA_PROCESSOR_SPECIFIC_LOCAL_SEGMENT_POS+474)
#define LEADER_FILE_DATASET_VELOCITY_Y_SIZE (22)
#define LEADER_FILE_DATASET_VELOCITY_Z_POS (LEADER_FILE_DATA_PROCESSOR_SPECIFIC_LOCAL_SEGMENT_POS+496)
#define LEADER_FILE_DATASET_VELOCITY_Z_SIZE (22)

#define DATA_FILE_RAW_SIGNAL_HEADER_SIZE (412)
#define DATA_FILE_RAW_SIGNAL_RECORD_SIZE (11644)

#define LIGHTSPEED (2.99792458E8) //m/s
#define EARTH_RADIUS (6378144.0) //m
#define ERS_SLOPE (4.19E11) //Hz/s
#define ERS_ANTENNA_LENGTH (10.0) //m
#define ERS_PLATFORM_ALTITUDE (790000.0) //m
//#define ERS_AZ_BEAM_WIDTH (1.3) //degrees

#define ERS_PATH_SIZE (2000)

struct ERS_Raw_Parser_Ctx {
	char path_ldr[ERS_PATH_SIZE];
	int fd_ldr;
	char path_raw[ERS_PATH_SIZE];
	int fd_raw;
	int fd_raw_pos;
	int az_pos;

	int initialized;
	int has_out_params;
	int n_range;
	int n_azimuth;
	ERS_Raw_Parser_Params out_par;
};

ERS_Raw_Parser_Ctx * ers_raw_parser_alloc(char *path_ldr, char *path_raw)
{
	int fd_ldr = -1, fd_raw = -1;
	ERS_Raw_Parser_Ctx *ctx;

	if(!path_ldr || !path_raw) {
		fprintf(stderr, "%s:: Invalid arguments (%p, %p)", __func__, path_ldr, path_raw);
		return NULL;
	}

	fd_ldr = open(path_ldr, O_RDONLY);
	if(fd_ldr < 0) {
		fprintf(stderr, "%s:: Error opening file: %s errno(%d):%s\n", __func__, path_ldr, errno, strerror(errno));
		goto ers_raw_parser_alloc_error;
	}

	fd_raw = open(path_raw, O_RDONLY);
	if(fd_raw < 0) {
		fprintf(stderr, "%s:: Error opening file: %s errno(%d):%s\n", __func__, path_raw, errno, strerror(errno));
		goto ers_raw_parser_alloc_error;
	}

	ctx = (ERS_Raw_Parser_Ctx *)calloc(1, sizeof(ERS_Raw_Parser_Ctx));
	if(!ctx) {
		fprintf(stderr, "%s:: Error calloc(): errno(%d):%s\n", __func__, errno, strerror(errno));
		goto ers_raw_parser_alloc_error;
	}

	ctx->fd_ldr = fd_ldr;
	snprintf(ctx->path_ldr, ERS_PATH_SIZE, "%s", path_ldr);
	ctx->fd_raw = fd_raw;
	snprintf(ctx->path_raw, ERS_PATH_SIZE, "%s", path_raw);

	ctx->initialized = 1;

	return ctx;

ers_raw_parser_alloc_error:
	if(fd_ldr > -1)
		close(fd_ldr);
	if(fd_raw > -1)
		close(fd_raw);

	return NULL;
}

void ers_raw_parser_free(ERS_Raw_Parser_Ctx *ctx)
{
	if(!ctx)
		return;

	if(ctx->fd_ldr) {
		close(ctx->fd_ldr);
	}

	if(ctx->fd_raw) {
		close(ctx->fd_raw);
	}
	free(ctx);

	return;
}

int log_ers_params(ERS_Raw_Parser_Params *par)
{
	if(!par) {
		return -1;
	}

	fprintf(stdout, "Mission id: %s\n", par->missionid);
	fprintf(stdout, "Wavelength: %f[m]\n", par->lambda);
	fprintf(stdout, "Range chirp slope: %f [Hz/s]\n", par->kr);
	fprintf(stdout, "Range pulse phase offset: %f [rad]\n", par->ra_ph_off);
	fprintf(stdout, "Sampling frequency: %f[Hz]\n", par->fs);
	fprintf(stdout, "Range pulse width: %f[s]\n", par->tau);
	fprintf(stdout, "Pulse repetition frequency: %f[Hz]\n", par->prf);
	fprintf(stdout, "Range gate delay: %f[s]\n", par->rgd);

	fprintf(stdout, "Velocity: %f[m/s]\n", par->velocity);
	fprintf(stdout, "n_samples: %d\n", par->n_valid_samples);
	fprintf(stdout, "FFT length: %d\n", par->ra_fft_len);
	fprintf(stdout, "FFT lines: %d\n", par->fft_lines);

	return 0;
}

int ers_raw_parser_get_params_from_file(ERS_Raw_Parser_Ctx *ctx, ERS_Raw_Parser_Params *out)
{
	int ret;
	ERS_Raw_Parser_Params params;

	char missionid[LEADER_FILE_DATASET_MISSION_ID_SIZE+1] = {0};
	char lambda_str[LEADER_FILE_DATASET_RADAR_WAVELENGTH_SIZE+1] = {0};
	double lambda;
	char fs_str[LEADER_FILE_DATASET_RANGE_SAMPLING_FREC_SIZE+1] = {0};
	double fs;
	char tau_str[LEADER_FILE_DATASET_RANGE_PULSE_LENGTH_SIZE+1] = {0};
	double tau;
	char ra_ph_off_str[LEADER_FILE_DATASET_RANGE_CHIRP_PHASE_OFFSET_SIZE+1] = {0};
	double ra_ph_off;
	char prf_str[LEADER_FILE_DATASET_PRF_SIZE+1] = {0};
	double prf;
	char ra_w_str[LEADER_FILE_DATASET_RANGE_BANDWIDTH_SIZE+1] = {0};
	double ra_w;
	char rgd_str[LEADER_FILE_DATASET_RGD_SIZE+1] = {0};
	double rgd;
	char v_x_str[LEADER_FILE_DATASET_VELOCITY_X_SIZE+1] = {0};
	double v_x;
	char v_y_str[LEADER_FILE_DATASET_VELOCITY_Y_SIZE+1] = {0};
	double v_y;
	char v_z_str[LEADER_FILE_DATASET_VELOCITY_Z_SIZE+1] = {0};
	double v_z;

	double v;
	double effective_v;
	int n_valid_samples;
	int ra_fft_len;

	if(!ctx) {
		fprintf(stderr, "%s::Invalid arguments ctx %p\n", __func__, ctx);
		return -1;
	}
	if(!ctx->initialized) {
		fprintf(stderr, "%s::Invalid context! ctx %p\n", __func__, ctx);
		return -1;
	}

#define GET_VAL(X, Y, outname, PRINT) \
	ret = lseek(ctx->fd_ldr, X, SEEK_SET); \
	if(ret != X) { \
		fprintf(stderr, "seek() ret %d fd %d errno(%d):%s\n", ret, ctx->fd_ldr, errno, strerror(errno)); \
		return -1; \
	} \
	ret = read(ctx->fd_ldr, outname, Y); \
	if(ret != Y) { \
		fprintf(stderr, "read() ret %d fd %d errno(%d):%s\n", ret, ctx->fd_ldr, errno, strerror(errno)); \
		return -1; \
	} \
	if(PRINT) \
		fprintf(stdout, "%s: %s\n", #outname, outname);

	GET_VAL(LEADER_FILE_DATASET_MISSION_ID_POS, LEADER_FILE_DATASET_MISSION_ID_SIZE, missionid, 0);
	GET_VAL(LEADER_FILE_DATASET_RADAR_WAVELENGTH_POS, LEADER_FILE_DATASET_RADAR_WAVELENGTH_SIZE, lambda_str, 0);
	GET_VAL(LEADER_FILE_DATASET_RANGE_CHIRP_PHASE_OFFSET_POS, LEADER_FILE_DATASET_RANGE_CHIRP_PHASE_OFFSET_SIZE, ra_ph_off_str, 0);
	GET_VAL(LEADER_FILE_DATASET_RANGE_SAMPLING_FREC_POS, LEADER_FILE_DATASET_RANGE_SAMPLING_FREC_SIZE, fs_str, 0);
	GET_VAL(LEADER_FILE_DATASET_RANGE_PULSE_LENGTH_POS, LEADER_FILE_DATASET_RANGE_PULSE_LENGTH_SIZE, tau_str, 0);
	GET_VAL(LEADER_FILE_DATASET_PRF_POS, LEADER_FILE_DATASET_PRF_SIZE, prf_str, 0);
	GET_VAL(LEADER_FILE_DATASET_RANGE_BANDWIDTH_POS, LEADER_FILE_DATASET_RANGE_BANDWIDTH_SIZE, ra_w_str, 0);
	GET_VAL(LEADER_FILE_DATASET_RGD_POS, LEADER_FILE_DATASET_RGD_SIZE, rgd_str, 0);
	GET_VAL(LEADER_FILE_DATASET_VELOCITY_X_POS, LEADER_FILE_DATASET_VELOCITY_X_SIZE, v_x_str, 1);
	GET_VAL(LEADER_FILE_DATASET_VELOCITY_Y_POS, LEADER_FILE_DATASET_VELOCITY_Y_SIZE, v_y_str, 1);
	GET_VAL(LEADER_FILE_DATASET_VELOCITY_Z_POS, LEADER_FILE_DATASET_VELOCITY_Z_SIZE, v_z_str, 1);

	lambda = strtod(lambda_str, NULL);
	ra_ph_off = strtod(ra_ph_off_str, NULL);
	fs = strtod(fs_str, NULL) *1E6;
	tau = strtod(tau_str, NULL) * 1E-6;
	prf = strtod(prf_str, NULL);
	ra_w = strtod(ra_w_str, NULL) * 1E6;
	rgd = strtod(rgd_str, NULL) * 1E-3;
	v_x = strtod(v_x_str, NULL);
	v_y = strtod(v_y_str, NULL);
	v_z = strtod(v_z_str, NULL);
#undef GET_VAL

	v = sqrt(v_x*v_x + v_y*v_y + v_z*v_z);
	effective_v = v*sqrt(EARTH_RADIUS/(EARTH_RADIUS+ERS_PLATFORM_ALTITUDE));

	//https://igppweb.ucsd.edu/~fialko/insar/03_ortiz_zebker.pdf
	double az_pulse_spacing = effective_v/prf; //az_tau
	double az_beam_width = lambda/ERS_ANTENNA_LENGTH; //[rad]
	double az_beam_width_gnd = ERS_PLATFORM_ALTITUDE*az_beam_width; //[m] @ground
	int pulses_for_full_aperture = ceil(az_beam_width_gnd/az_pulse_spacing); //N
	int fft_lines = pow(2, ceil(log((double)pulses_for_full_aperture)/log(2.0))); //next power of 2 for fft

	double kr = ra_w/tau; //ra_w[MHz], tau[us]

	n_valid_samples = (DATA_FILE_RAW_SIGNAL_RECORD_SIZE - DATA_FILE_RAW_SIGNAL_HEADER_SIZE)/2 - round(tau*fs); //minus unambiguos range
	ra_fft_len = pow(2, ceil(log((double)n_valid_samples)/log(2.0))); //next power of 2 for fft

	snprintf(params.missionid, LEADER_FILE_DATASET_MISSION_ID_SIZE, "%s", missionid);
	params.lambda = lambda;
	params.ra_ph_off = ra_ph_off;
	params.kr = kr;
	params.fs = fs;
	params.tau = tau;
	params.prf = prf;
	params.rgd = rgd;
	params.velocity = effective_v;
	params.n_valid_samples = n_valid_samples;
	params.ra_fft_len = ra_fft_len;
	params.fft_lines = fft_lines;
	params.C = LIGHTSPEED;
	params.r0 = LIGHTSPEED * params.rgd/2;
	params.az_beam_width = az_beam_width;

	//To test
	//params.lambda = 0.0566;
	//params.ra_ph_off = 0;
	//params.kr = 4.19E11;
	//params.fs = 18962468;
	//params.tau = 37.1E-6;
	//params.prf = 1679.9;
	//params.rgd = 0.0055;
	//params.velocity = 7.1277E3;

	if(out) {
		memcpy(out, &params, sizeof(ERS_Raw_Parser_Params));
	}
	memcpy(&ctx->out_par, &params, sizeof(ERS_Raw_Parser_Params));
	ctx->has_out_params = 1;
	ctx->n_range = (DATA_FILE_RAW_SIGNAL_RECORD_SIZE - DATA_FILE_RAW_SIGNAL_HEADER_SIZE)/2; //minus header, two values (16 bit)

	return 0;
}

int ers_raw_parser_get_raw_data_from_file(ERS_Raw_Parser_Ctx *ctx, ERS_Raw_Parser_Data_Patch **data)
{
	int ret, i, j;
	int need_bytes;
	uint8_t *buff;
	double complex *complex_buff;
	ERS_Raw_Parser_Data_Patch *out;

	if(!ctx->has_out_params) {
		fprintf(stderr, "%s:: Load ldr file first!", __func__);
		return -1;
	}

	if(!ctx->fd_raw_pos) {
		ret = lseek(ctx->fd_raw, DATA_FILE_RAW_SIGNAL_RECORD_SIZE, SEEK_SET);
		if(ret != DATA_FILE_RAW_SIGNAL_RECORD_SIZE) {
			fprintf(stderr, "%s:: seek() ret %d fd %d errno(%d):%s\n", __func__, ret, ctx->fd_raw, errno, strerror(errno));
			return -1;
		}
		ctx->fd_raw_pos = ret;
	}

	need_bytes = DATA_FILE_RAW_SIGNAL_RECORD_SIZE * ctx->out_par.fft_lines;
	buff = (uint8_t*) calloc(1, need_bytes);
	if(!buff) {
		fprintf(stderr, "%s:: calloc(%d) errno(%d):%s\n", __func__, need_bytes, errno, strerror(errno));
		return -1;
	}

	ret = read(ctx->fd_raw, buff, need_bytes);
	if(ret != need_bytes) { //FIXME No handle for incomplete read, everything is an error
		fprintf(stderr, "%s:: read(%d, %d) ret %d errno(%d):%s\n", __func__, ctx->fd_raw, need_bytes, ret, errno, strerror(errno));
		free(buff);
		return -1;
	}
	ctx->fd_raw_pos += ret;
	ctx->az_pos += ctx->out_par.fft_lines;

	complex_buff = (complex double *) calloc(1, ctx->n_range * ctx->out_par.fft_lines * sizeof(complex double));
	if(!complex_buff) {
		fprintf(stderr, "%s:: calloc(%d) errno(%d):%s\n", __func__, ctx->n_range * ctx->out_par.fft_lines, errno, strerror(errno));
		free(buff);
		return -1;
	}

	for(j = 0; j < ctx->out_par.fft_lines; j++) {
		for(i = DATA_FILE_RAW_SIGNAL_HEADER_SIZE; i < DATA_FILE_RAW_SIGNAL_RECORD_SIZE; i = i+2) {
			complex_buff[(i-DATA_FILE_RAW_SIGNAL_HEADER_SIZE)/2+j*ctx->n_range] = buff[i+j*DATA_FILE_RAW_SIGNAL_RECORD_SIZE] + I*buff[i+j*DATA_FILE_RAW_SIGNAL_RECORD_SIZE+1];

			//printf("%g+j%g ", creal(complex_buff[(i-DATA_FILE_RAW_SIGNAL_HEADER_SIZE)/2+j*ctx->n_range]),cimag(complex_buff[(i-DATA_FILE_RAW_SIGNAL_HEADER_SIZE)/2+j*ctx->n_range]));
		}
		//printf("\n", creal(complex_buff[(i-DATA_FILE_RAW_SIGNAL_HEADER_SIZE)/2+j*ctx->n_range]),cimag(complex_buff[(i-DATA_FILE_RAW_SIGNAL_HEADER_SIZE)/2+j*ctx->n_range]));
	}
	free(buff);

	out = calloc(1, sizeof(ERS_Raw_Parser_Data_Patch));
	if(!out) {
		fprintf(stderr, "%s:: calloc(%ld) errno(%d):%s\n", __func__, sizeof(ERS_Raw_Parser_Data_Patch), errno, strerror(errno));
		free(buff);
		free(complex_buff);
		return -1;
	}
	out->n_ra = ctx->n_range;
	out->az_pos = ctx->az_pos -= ctx->out_par.fft_lines;
	out->n_az = ctx->out_par.fft_lines;
	out->data = complex_buff;

	if(data)
		*data = out;

	return 0;
}

void ers_raw_parser_data_patch_free(ERS_Raw_Parser_Data_Patch *data)
{
	if(!data)
		return;
	if(data->data) {
		free(data->data);
		data->data = NULL;
	}
	free(data);
}
