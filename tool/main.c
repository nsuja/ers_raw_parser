#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <ers_raw_parser.h>

#define INPUT_PATH_SIZE (2000)

static int g_has_input_ldr = 0;
static char g_input_ldr[INPUT_PATH_SIZE];
static int g_has_input_raw = 0;
static char g_input_raw[INPUT_PATH_SIZE];

static const char *short_opts= "l:r:";
static struct option long_opts[] = {
	{"ldr", required_argument, NULL,'l'},
	{"raw", required_argument, NULL,'r'},
	{"help", no_argument, NULL,'h'},
	{NULL,0,NULL,0}
};

int parse_opts(int argc, char **argv)
{
	int c;

	while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch (c){
			case 'l':
				snprintf(g_input_ldr, INPUT_PATH_SIZE, "%s", optarg);
				g_has_input_ldr = 1;
				break;
			case 'r':
				snprintf(g_input_raw, INPUT_PATH_SIZE, "%s", optarg);
				g_has_input_raw = 1;
				break;
			case 'h':
				//print_help(argc, argv);
				return 1;
			default:
				fprintf(stderr, "Bad arguments: Try `%s --help' for further information.\n", argv[0]);
				return -1;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	int ret;
	ERS_Raw_Parser_Ctx *ctx;
	ERS_Raw_Parser_Params params;
	ERS_Raw_Parser_Data_Patch *data;

	if(parse_opts(argc, argv)) {
		//print_help(argv[0]);
		printf("Bad arguments\n");
		return EXIT_FAILURE;
	}

	if(!g_has_input_ldr || !g_has_input_raw) {
		//print_help(argv[0]);
		printf("No input\n");
		return EXIT_FAILURE;
	}

	ctx = ers_raw_parser_alloc(g_input_ldr, g_input_raw);

	if(ers_raw_parser_get_params_from_file(ctx, &params)) {
		fprintf(stderr, "%s:: ers_raw_parser_get_params_from_file(%s)", __func__, g_input_ldr);
		return EXIT_FAILURE;
	}

	log_ers_params(&params);

	if(ers_raw_parser_get_raw_data_from_file(ctx, &data)) {
		fprintf(stderr, "%s:: ers_raw_parser_get_raw_data_from_file(%s)", __func__, g_input_raw);
		return EXIT_FAILURE;
	}

	ers_raw_parser_data_patch_free(data);

	ers_raw_parser_free(ctx);

	return 0;
}

