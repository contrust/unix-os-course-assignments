#include <stdio.h>
#include <stddef.h>
int main(){
	FILE * output;
	char zero_buffer[1024] = {0};
	char one_buffer[] = {1};
	output = fopen("fileA", "w");
	if (output == NULL)
		return 1;
	fwrite(one_buffer, 1, 1, output);
	for (int i = 0; i < 4 * 1024; ++i){
		fwrite(zero_buffer, 1, 1024, output);
	}
	fseek(output, 4000, SEEK_SET);
	fwrite(one_buffer, 1, 1, output);
	fseek(output, 10000, SEEK_SET);
	fwrite(one_buffer, 1, 1, output);
	fclose(output);
	return 0;
}
