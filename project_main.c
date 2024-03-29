#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include "cJSON.c"


static cJSON *json;

static char *buffer;


cJSON *find(char const *path){
	//taking the corresponding node of the path
    char *str;
    str = (char*) malloc(sizeof(char) * strlen(path));								
    memcpy(str, path, strlen(path));
    
    const char s[2] = "/";
	char *token;
	token = strtok(str, s);
	cJSON *x = json->child;
	while( token != NULL ) { 						
		
		while(x->next != NULL && strcmp(x->string, token)){
			x = x->next;
		}
		
		token = strtok(NULL, s);
		if(token != NULL){      
			x = x->child;
		}
	
	}
    
    return x;
}


cJSON *findParent(char const *path){
	char *str;
    str = (char*) malloc(sizeof(char) * strlen(path));								
    memcpy(str, path, strlen(path));
	
	const char s[2] = "/";
	char *token;
	char *tokens[10];
	int i = 0;
	token = strtok(str, s);
	tokens[i] = token;
	
	while( token != NULL ) { 						
		token = strtok(NULL, s);
		
		i++;
		tokens[i] = token;
    }
	
	cJSON *y = json;  						
	for(int j = 0; j<i-1; j++){
		y = y->child;
		while(y->next != NULL && strcmp(y->string, tokens[j])){
			y = y->next;
		}
		
	}
	
	return y;
}


static int json_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;
    
    json = cJSON_ParseWithOpts(buffer, 0, 1);
    cJSON *x = find(path);
    
    
    memset(stbuf, 0, sizeof(struct stat));
    
    //if valuestring == NULL -> directory
    //if valuestring != NULL -> file
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        stbuf->st_size = 4096;
    } else if (x->valuestring == NULL) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        stbuf->st_size = 4096;
    } else if (x->valuestring != NULL) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(x->valuestring);
    }
    else
        res = -ENOENT;

    return res;
}

static int json_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;
    
    json = cJSON_ParseWithOpts(buffer, 0, 1); 
    cJSON *x = find(path);
	
	//if valuestring == NULL -> directory
    //if valuestring != NULL -> file
    if (x->valuestring != NULL)
        return -ENOENT;
        
	//filler(buf, x->string, NULL, 0);
    if(strcmp(path, "/") != 0){
		x = x->child;
	}
	
	
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
		
    filler(buf, x->string, NULL, 0);
    while(x->next != NULL){
		x = x->next;
		filler(buf, x->string, NULL, 0);
	}
	
    return 0;
}

static int json_open(const char *path, struct fuse_file_info *fi)
{
	json = cJSON_ParseWithOpts(buffer, 0, 1); 
    cJSON *x = find(path);
    
    if (x->valuestring == NULL)
        return -ENOENT;

    if ((fi->flags & 3) != O_RDONLY)
        return -EACCES;

    return 0;
}


static int json_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    size_t len;
    (void) fi;
    
        
    json = cJSON_ParseWithOpts(buffer, 0, 1); 
    cJSON *x = find(path);
    
    
    if (x->valuestring == NULL)
        return -ENOENT;
    
    len = strlen(x->valuestring);
    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, x->valuestring + offset, size);
    } else
        size = 0;

    return size;
}


static int json_unlink(const char *path){  				//class/operatingsystem
	json = cJSON_ParseWithOpts(buffer, 0, 1);
    cJSON *x = find(path);
    cJSON *y = findParent(path);
    
    
    if(x == NULL || y->valuestring != NULL){
		return -ENOENT;
	}
	
    
    if(y->child == x){   					//deleting first child
		y->child = y->child->next;
	} else {
		x->prev->next = x->next;
	}
	
	
	buffer = cJSON_Print(json);
	
	///////////////////////////////		writing to the json file after deleting file (only works in debug mode)
	/*
	FILE *fp;
	fp = fopen("example.json", "w");
	fputs(buffer, fp);
	fclose(fp);
	*/
	///////////////////////////////
	
    return 0;
}


static struct fuse_operations json_oper = {
    .getattr	= json_getattr,
    .readdir	= json_readdir,
    .open		= json_open,
    .read		= json_read,
    .unlink		= json_unlink,
};

int main(int argc, char *argv[])
{
	FILE *fp;
	free(buffer);
	buffer = (char*) malloc(sizeof(char) * 10000);
	fp = fopen("example.json","r+");
	fread(buffer, 10000, 1, fp);
	fclose(fp);
	
    return fuse_main(argc, argv, &json_oper, NULL);;
}
