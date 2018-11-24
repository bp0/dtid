/*
 *    dtid - Device Tree identifier utility
 *    Copyright (C) 2018 Burt P. <pburt0@gmail.com>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

static char *scan_dtids_file(const char *dtids_file, const char *compat_string, const char *class) {
#define DTID_BUFF_SIZE 128
    char buff[DTID_BUFF_SIZE];
    FILE *fd;
    char *cls, *cstr, *name;
    char *p, *b;
    int l;

    fd = fopen(dtids_file, "r");
    if (!fd) return NULL;

    while (fgets(buff, DTID_BUFF_SIZE, fd)) {
        /* line ends at comment */
        p = strchr(buff, '#');
        if (p) *p = 0;

        /* trim trailing white space */
        p = buff + strlen(buff) - 1;
        while(p > buff && isspace((unsigned char)*p)) p--;
        *(p+1) = 0;

        /* scan for fields */
#define DTID_FFWD() while(isspace((unsigned char)*p)) p++;
        p = buff; DTID_FFWD();
        cls = p; if (!*cls) continue;
        b = strpbrk(p, " \t");
        if (!b) continue;
        *b = 0; p = b + 1; DTID_FFWD();
        cstr = p; if (!*cstr) continue;
        b = strpbrk(p, " \t");
        if (!b) continue;
        *b = 0; p = b + 1; DTID_FFWD();
        name = p; /* whatever is left is name */

        if (class) {
            /* class filter */
            if (strcmp(class, cls) != 0) continue;

            /* wildcard, allowed only when looking
             * for a specific kind of thing */
            b = strchr(cstr, '*');
            if (b) {
                *b = 0;
                l = strlen(cstr);
                if (strncmp(compat_string, cstr, l) == 0) {
                    fclose(fd);
                    return strdup(name);
                } else
                    continue;
            }
        }

        /* exact match */
        if (strcmp(compat_string, cstr) == 0) {
            fclose(fd);
            return strdup(name);
        }

    }
    fclose(fd);
    return NULL;
}

static int dtid(const char *dtids_file, const char *compat_string, const char *class, char **name, char **vendor) {

    if (!compat_string) return 0;

    if (name) {
        *name = scan_dtids_file(dtids_file, compat_string, class);
    }
    if (vendor) {
        char *c, *p;
        c = strdup(compat_string);
        p = strchr(c, ',');
        if (p) {
            *p = 0;
            *vendor = scan_dtids_file(dtids_file, c, "vendor");
        }
        free(c);
    }

    return (*name || *vendor) ? 1 : 0;
}

static void usage(const char* name) {
    fprintf(stderr, "Usage:\n"
        "%s [options] [compat_string]\n", name);
    fprintf(stderr, "Options:\n"
        "    -h\t\t display usage information\n"
        "    -d <file>\t location of dt.ids file\n"
        );
}

static const char dtids_file_loc[] = "../dt.ids";

int main(int argc, char *argv[]) {
    char *dn = NULL, *dv = NULL;
    int found = 0;

    const char *dtids_file = dtids_file_loc;
    const char *sclass = NULL;
    char *compat_string = NULL;
    int c;

    int opt_help = 0;

    while ((c = getopt(argc, argv, "c:d:h")) != -1) {
        switch (c) {
            case 'c':
                sclass = optarg;
                break;
            case 'd':
                dtids_file = optarg;
                break;
            case 'h':
                opt_help = 1;
                break;
            default:
                usage(argv[0]);
                return 1;
        }
    }

    if (argc - optind >= 1)
        compat_string = argv[optind];
    else {
        usage(argv[0]);
        return 2;
    }

    if (opt_help) {
        usage(argv[0]);
        return 0;
    }

    if (access(dtids_file, R_OK)) {
        fprintf(stderr, "error: file unreadable: %s\n", dtids_file);
        return 2;
    }

    found = dtid(dtids_file, compat_string, sclass, &dn, &dv);
    if (found)
        printf("found: n='%s' v='%s'\n", dn, dv);
    else
        printf("not found\n");

    free(dn);
    free(dv);

    return (found) ? 0 : 1;
}
