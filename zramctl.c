#define _GNU_SOURCE

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

void add_device() {
	struct stat zramctl;
	struct stat hotadd;
	const int z = stat("/sys/class/zram-control", &zramctl);
	if (z == -1) {
		fprintf(stderr, "Your system does not support ZRAM!\n");
	} else {
		const int h = stat("/sys/class/zram-control/hot_add", &hotadd);
		if (h == -1) {
			fprintf(stderr, "System error: can't add new device\n");
		} else {
		    FILE *add = fopen("/sys/class/zram-control/hot_add", "r");
		    if (add == NULL) {
		    	fprintf(stderr, "Permission denied!\n");
		    }
		    char *num = NULL;
		    size_t r = 0;
		    getline(&num, &r, add);
		    fclose(add);
		    printf("Adding device zram%s\n", num);
		}
	}
}

void list_devices() {
	struct stat sysblk;
	const int sb = stat("/sys/block", &sysblk);
	if (sb == -1) {
		fprintf(stderr, "Permission denied!\n");
	} else {
		DIR *d = opendir("/sys/block");
		if (d == NULL) {
			fprintf(stderr, "Permission denied!\n");
		} else {
			struct dirent *entry;
			char *siz = malloc(sizeof(char));
			char *cmp = malloc(sizeof(char));
			char *str = NULL;
			size_t ln = 0;
			while ((entry = readdir(d)) != NULL) {
				if (strstr(entry->d_name, "zram") != NULL) {
					printf("%s Size: ", entry->d_name);
					siz = realloc(siz, sizeof(char) * (11 + strlen(entry->d_name) + 9));
					sprintf(siz, "/sys/block/%s/disksize", entry->d_name);
					cmp = realloc(cmp, sizeof(char) * (11 + strlen(entry->d_name) + 15));
					sprintf(cmp, "/sys/block/%s/comp_algorithm", entry->d_name);
					FILE *f = fopen(siz, "r");
					getline(&str, &ln, f);
					fclose(f);
					printf("%s Compression: ", str);
					f = fopen(cmp, "r");
					getline(&str, &ln, f);
					fclose(f);
					printf("%s\n", str);
				}
			}
			free(siz);
			free(cmp);
			closedir(d);
		}
	}
}

void delete_device(char *dev) {
	char *dname = malloc(sizeof(char) * (15 + strlen(dev)));
	sprintf(dname, "/sys/block/zram%s", dev);
	struct stat dv;
	const int v = stat(dname, &dv);
	if (v == -1) {
		fprintf(stderr, "No such device zram%s\n", dev);
	} else {
	    int rm = open("/sys/class/zram-control/hot_remove", O_WRONLY);
	    write(rm, dev, strlen(dev));
	    close(rm);
	}
	free(dname);
}

void setsize(char *num, char *bsize) {
	char *dname = malloc(sizeof(char) * (15 + strlen(num)));
	sprintf(dname, "/sys/block/zram%s", num);
	struct stat dv;
	const int v = stat(dname, &dv);
	if (v == -1) {
		fprintf(stderr, "No such device zram%s\n", num);
	} else {
		char *sf = malloc(sizeof(char) * (strlen(dname) + 9));
		sprintf(sf, "%s/disksize", dname);
		int rm = open(sf, O_WRONLY);
		write(rm, bsize, strlen(bsize));
		close(rm);
		free(sf);
	}
	free(dname);
}

void setcomp(char *num, char *ctype) {
    if (strcmp(ctype, "lz4") == 0 || strcmp(ctype, "lz4hc") == 0 || strcmp(ctype, "deflate") == 0 || strcmp(ctype, "lzo") == 0 || strcmp(ctype, "zstd") == 0) {
	    char *dname = malloc(sizeof(char) * (15 + strlen(num)));
    	sprintf(dname, "/sys/block/zram%s", num);
	    struct stat dv;
	    const int v = stat(dname, &dv);
	    if (v == -1) {
		    fprintf(stderr, "No such device zram%s\n", num);
	    } else {
		    char *cmpfile = malloc(sizeof(char) * (strlen(dname) + 15));
		    sprintf(cmpfile, "%s/comp_algorithm", dname);
		    int rm = open(cmpfile, O_WRONLY);
		    write(rm, ctype, strlen(ctype));
		    close(rm);
		    free(cmpfile);
	    }
	    free(dname);
	} else {
		fprintf(stderr, "Invalid compression type '%s'\n", ctype);
	}
}

void usage() {
	printf("Usage:\n");
	printf(" zramctl add - add device\n");
	printf(" zramctl (num) del - remove device\n");
	printf(" zramctl (num) size (size) - set size of device (in bytes)\n");
	printf(" zramctl (num) compress (comptype) - set compression type (lz4/lz4hc/deflate/lzo/zstd/842)\n");
	printf(" zramctl list - list zram devices\n");
}

int main(int argc, char *argv[]) {
	if (argc < 2 || argc > 4) {
		usage();
	} else if (argc == 2) {
		if (strcmp(argv[1], "add") == 0) {
			add_device();
		} else if (strcmp(argv[1], "list") == 0) {
			list_devices();
		} else {
			usage();
		}
	} else if (argc == 3) {
		if (strcmp(argv[2], "del") == 0) {
			delete_device(argv[1]);
		} else {
			usage();
		}
	} else if (argc == 4) {
		if (strcmp(argv[2], "size") == 0) {
			setsize(argv[1], argv[3]);
		} else if (strcmp(argv[2], "compress") == 0) {
			setcomp(argv[1], argv[3]);
		} else {
			usage();
		}
	}
	return 0;
}
