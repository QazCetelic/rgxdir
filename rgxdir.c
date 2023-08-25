#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <regex.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// This program groups files in a subdirectories based on a regex pattern.
// 
// Imagine a directory with the following files: 
// 2020-01-01.log
// 2020-01-05.log 
// 2020-02-01.log
//
// Using "rgxdir '^([0-9]{4})-([0-9]{2})-([0-9]{2}).*$' /path/to/dir" will result in the following directory structure.
// 2020/
//   01/
//     01/
//       2020-01-01.log
//     05/
//       2020-01-05.log
//   02/
//     01/
//       2020-02-01.log

DIR* touch_dir(char* path);
int process_dir(DIR *dir, unsigned int dir_path_length, char *dir_path, regex_t regex, unsigned long rgx_groups);
void move_to_subfolder(char *dir_path, char *sub_folder_path, char *file_name);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: rgxdir <pattern> <directory>\n");
        return 0;
    }

    char* rgx_pattern = argv[1];
    unsigned int dir_path_length = strlen(argv[2]);
    if (argv[2][dir_path_length - 1] == '/') {
        dir_path_length--;
    }
    char dir_path[dir_path_length];
    memcpy(dir_path, argv[2], dir_path_length);

    DIR* dir = opendir(dir_path);
    if (ENOENT == errno) {
        fprintf(stderr, "Error: Directory does not exist!\n");
        return 1;
    }
    else if (!dir) {
        fprintf(stderr, "Error: Failed to open directory!\n");
        return 1;
    }

    regex_t regex;
    int rgx_comp_ret = regcomp(&regex, rgx_pattern, REG_EXTENDED);
    // The entire input is also a group, re_nsub does not include that.
    unsigned long rgx_groups = regex.re_nsub + 1;
    if (rgx_comp_ret) {
        fprintf(stderr, "Error %d: Could not compile regex\n", rgx_comp_ret);
        return 1;
    }

    process_dir(dir, dir_path_length, dir_path, regex, rgx_groups);

    regfree(&regex);
    closedir(dir);

    return 0;
}

int process_dir(DIR *dir, unsigned int dir_path_length, char *dir_path, regex_t regex, unsigned long rgx_groups) {
    struct dirent* f;
    while ((f = readdir(dir)) != NULL) {
        // Check if entry is a file and not a directory or symlink
        if (f->d_type == DT_REG) {
            regmatch_t capture_groups[rgx_groups];
            bool rgx_match = !regexec(&regex, f->d_name, rgx_groups, capture_groups, 0);

            if (rgx_match) {
                unsigned int chars = dir_path_length;
                unsigned int group_index; // Skips 0th group containing entire string
                char* path = malloc(chars * sizeof(char));
                strcpy(path, dir_path);
                for (group_index = 1; group_index < rgx_groups; group_index++) {
                    unsigned int group_length = capture_groups[group_index].rm_eo - capture_groups[group_index].rm_so;
                    char group[group_length];
                    memcpy(group, &f->d_name[capture_groups[group_index].rm_so], group_length);
                    group[group_length] = '\0';
                    chars += group_length + strlen("/");
                    char match_path[chars];
                    strcpy(match_path, path);
                    strcat(match_path, "/");
                    strcat(match_path, group);
                    // Realloc with size including appended group
                    free(path);
                    path = malloc(chars * sizeof(char));
                    strcpy(path, match_path);

                    DIR* group_dir = touch_dir(path);
                    if (group_dir == NULL) {
                        fprintf(stderr, "Error: Failed to interact with directory %s\n", path);
                        return 1;
                    }
                    closedir(group_dir);

                    if (group_index == rgx_groups - 1) {
                        move_to_subfolder(dir_path, path, f->d_name);
                    }
                }
                free(path);
            }
        }
    }

    return 0;
}

void move_to_subfolder(char *dir_path, char *sub_folder_path, char *file_name) {
    unsigned int old_file_path_length = strlen(dir_path) + strlen("/") + strlen(file_name) + 1;
    char old_file_path[old_file_path_length];
    strcpy(old_file_path, dir_path);
    strcat(old_file_path, "/");
    strcat(old_file_path, file_name);

    unsigned int new_file_path_length = strlen(sub_folder_path) + strlen("/") + strlen(file_name) + 1;
    char new_file_path[new_file_path_length];
    strcpy(new_file_path, sub_folder_path);
    strcat(new_file_path, "/");
    strcat(new_file_path, file_name);

    // Rename is fine, because the subfolder is within the same filesystem.
    rename(old_file_path, new_file_path);
}

/**
 * Returns an existing directory or creates a new one.
 * @param path The path to the directory
 * @return The directory or NULL if it could not be created
 */
DIR* touch_dir(char* path) {
    DIR* dir = opendir(path);
    if (dir) {
        return dir;
    }
    else if (ENOENT == errno) {
        int mkdir_ret = mkdir(path, 0777);
        if (mkdir_ret) {
            fprintf(stderr, "Error %d: Could not create directory %s\n", mkdir_ret, path);
            return NULL;
        }
        else {
            return opendir(path);
        }
    }
    else {
        fprintf(stderr, "Error: Failed to open directory %s\n", path);
        return NULL;
    }
}