#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct compression_lut{
	unsigned int subscript;
	unsigned int number;
};

#define COMPRESSION_THRESHOLD sizeof(struct compression_lut)

int compression(unsigned char *source_data, unsigned int data_length,
		unsigned char *data, struct compression_lut *lut)
{
	unsigned int source_data_subscript = 0;
	unsigned int data_subscript = 0;
	unsigned int lut_subscript = 3;

	while(source_data_subscript < data_length) {
		data[data_subscript] = source_data[source_data_subscript];
		while(source_data[source_data_subscript] == source_data[source_data_subscript+1]) {
			lut[lut_subscript].subscript = data_subscript;
			lut[lut_subscript].number++;
			if( source_data_subscript == (data_length - 1)) {
				lut[lut_subscript].number++;
				break;
			}
			source_data_subscript++;
		}

		if(lut[lut_subscript].number != 0) {
			if(lut[lut_subscript].number < COMPRESSION_THRESHOLD) {
				while(lut[lut_subscript].number) {
					data[++data_subscript] =
						source_data[source_data_subscript - lut[lut_subscript].number];
					lut[lut_subscript].number--;
				}
				lut[lut_subscript].subscript = 0;
				lut[lut_subscript].number    = 0;
			} else {
				lut_subscript++;
			}
		}

		if( source_data_subscript == (data_length - 1)) {
			data_subscript++;
			source_data_subscript++;
			data[data_subscript] = source_data[source_data_subscript];
			break;
		}

		source_data_subscript++;
		data_subscript++;
	}

	lut[0].number    = data_length;
	lut[1].number    = data_subscript;
	lut[2].number    = lut_subscript;

	printf("Compression: %ld%%\n",
			(data_subscript + lut_subscript *
			 sizeof(struct compression_lut))*100/data_length);
	return 0;
}

int decompression(unsigned char *source_data,
		unsigned char *data, struct compression_lut *lut)
{
	unsigned int data_length = lut[0].number;
	unsigned int source_data_subscript = 0;
	unsigned int data_subscript = 0;
	unsigned int lut_subscript = 3;
	unsigned int temp_i = 0;

	while ( data_subscript < data_length) {
		if (source_data_subscript == lut[lut_subscript].subscript) {
			for (temp_i = 0; temp_i < lut[lut_subscript].number + 1; temp_i++) {
				data[data_subscript] = source_data[source_data_subscript];
				data_subscript++;
			}
			lut_subscript++;
		} else {
			data[data_subscript] = source_data[source_data_subscript];
			data_subscript++;
		}
		source_data_subscript++;
	}
	return 0;
}

int write_data_file(FILE *file, unsigned char *data, unsigned int data_length)
{
	unsigned int data_subscript = 0;

	while (data_subscript < data_length) {
		char write_buffer[100] = "";
		sprintf(write_buffer, "    0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x,\n",
				data[data_subscript+0], data[data_subscript+1],
				data[data_subscript+2], data[data_subscript+3],
				data[data_subscript+4],	data[data_subscript+5],
				data[data_subscript+6], data[data_subscript+7],
				data[data_subscript+8], data[data_subscript+9]);

		if(data_length - data_subscript < 10) {
			fwrite(write_buffer, 1,
					(data_length - data_subscript) * strlen("0x00, ") + 4 - 2, file);
		} else {
			fwrite(write_buffer, 1, strlen(write_buffer), file);
		}

		data_subscript += 10;
		printf("\rwrite file data %d%%", (data_subscript*100/data_length));
		fflush(stdout);
	}
	printf("\rwrite file data 100%%\n");
	return 0;
}

int write_lut_file(FILE *file, struct compression_lut *lut, unsigned int lut_length)
{
	unsigned int lut_subscript = 0;

	while (lut_subscript < lut_length) {
		char write_buffer[50] = "";
		sprintf(write_buffer, "    { %d, %d},\n",
				lut[lut_subscript].subscript, lut[lut_subscript].number);

		fwrite(write_buffer, 1, strlen(write_buffer), file);
		lut_subscript++;

		printf("\rwrite file lut %d%%", (lut_subscript*100/lut_length));
		fflush(stdout);
	}
	printf("\rwrite file lut 100%%\n");
	return 0;
}

int main(int argc, char **argv)
{
	FILE *fb_bin = NULL;
	FILE *fb_c = NULL;

	char *include_head = "#include \"../jz4775_android_epd.h\"\n\n";
	char *head_c = "unsigned char WAVEFORM_LUT[] = {\n";
	char *end_c = "\n};\n";

	char *lut_head_c = "struct compression_lut COMPRESSION_LUT[] = {\n";
	char *lut_end_c = "\n};\n";

	unsigned int file_length = 0;

	unsigned char *source_data;
	unsigned char *data;
	struct compression_lut *lut;

	if(argc != 3){
		printf("ERROR:Input parameter error\n");
		printf("./binary_array_tools source_file object_file\n");
		return -1;
	}
	printf("source file:%s\nobject_file:%s\n", argv[1], argv[2]);

	fb_bin = fopen(argv[1], "rb");
	if(fb_bin == NULL) {
		printf("ERROR:Open %s failed\n", argv[1]);
		return -1;
	}

	if(fseek(fb_bin, 0, SEEK_END)) {
		printf("ERROR:Calculated length of the %s failed\n", argv[1]);
		fclose(fb_bin);
		return -1;
	}
	file_length = ftell(fb_bin);
	printf("%s:file length = %d\n", argv[1], file_length);

	if(fseek(fb_bin, 0, SEEK_SET)) {
		printf("ERROR:Calculated length of the %s failed\n", argv[1]);
		fclose(fb_bin);
		return -1;
	}

	fb_c = fopen(argv[2], "w");
	if(fb_c == NULL) {
		printf("ERROR:Open %s failed\n", argv[2]);
		fclose(fb_bin);
		return -1;
	}

	source_data = (unsigned char *)malloc(file_length);
	data        = (unsigned char *)malloc(file_length);
	lut = (struct compression_lut *)malloc(
			sizeof(struct compression_lut) * file_length);
	if(source_data == NULL || data == NULL || lut == NULL) {
		printf("ERROR: buffer malloc failed\n");
		fclose(fb_bin);
		fclose(fb_c);
		return -1;
	}
	memset(source_data, 0x0, file_length);
	memset(data, 0x0, file_length);
	memset(lut, 0x0, sizeof(struct compression_lut) * file_length);

	fread(source_data, 1, file_length, fb_bin);
	compression(source_data, file_length, data, lut);

	fwrite(include_head, strlen(include_head), 1, fb_c);
	fwrite(head_c, strlen(head_c), 1, fb_c);
	write_data_file(fb_c, data, lut[1].number);
	fwrite(end_c, strlen(end_c), 1, fb_c);

	fwrite(lut_head_c, strlen(lut_head_c), 1, fb_c);
	write_lut_file(fb_c, lut, lut[2].number);
	fwrite(lut_end_c, strlen(lut_end_c), 1, fb_c);

	fclose(fb_bin);
	fclose(fb_c);
	return 0;
}
