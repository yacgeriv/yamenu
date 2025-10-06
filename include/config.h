#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	char background_path[256];
} Settings;

int read_config_file(Settings* user_settings) {
	char config_path[512] = "/.config/yamenu/config.ini";	
	char* home_dir = getenv("HOME");

    char full_config_path[512];
        
    if (snprintf(full_config_path, sizeof(full_config_path), "%s%s", 
                 home_dir, config_path) >= sizeof(full_config_path)) 
    {
        return 0;
    }        

	FILE *config_file = fopen(full_config_path, "r");
	
	if(config_file == NULL)	{
		perror("couldn't read config file\n");
		return 0;	
	}
	
	char line[256];
	while (fgets(line, sizeof(line), config_file) != NULL) {
		char key[128] , value[256];
		sscanf(line,"%127[^=]=%255s", key, value);
		
		if(strcmp(key, "background-img")){
			strcpy(user_settings->background_path, value);
		}
	}
		
	return 1;	
}
