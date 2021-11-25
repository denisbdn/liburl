
/**
 * Small example for liburl for backlinks
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "uri.h"

int main(int argc, char **argv)
{
	int ch;
	const char *base = NULL;
	const char *relt = NULL;

    struct uri url_uri;
    ZERROSET(url_uri);
	const char *u = "http://en.news-4-u.ru/inopressa-the-song-1944-stated-ukraine-at-eurovision-2016-the-heavy-stone-in-the-garden-russia-2.html";
    if (parse_uri(u, &url_uri) != OK) {
        return 1;
    }
    struct holder url_holder = encode(&url_uri);
    if (is_empty_holder(&url_holder)) {
        return 1;
    }
	
	while ((ch = getopt(argc, argv, "b:r:")) != -1) {
		switch (ch) {
			case 'b':
				base = optarg;
				break;
			case 'r':
				relt = optarg;
				break;

			case '?':
			default:
				fprintf(stderr, "Usage: %s -b base_url -r relative_url\n", argv[0]);
				return EXIT_FAILURE;
		}
	}

	struct uri base_uri, relt_uri;
	ZERROSET(base_uri);
	ZERROSET(relt_uri);
	if (parse_uri(base, &base_uri) == OK && parse_uri(relt, &relt_uri) == OK) {
		struct holder h = relative_uri(&base_uri, &relt_uri);
		if (!is_empty_holder(&h)) {
			printf("%s\n", to_string(&h));
			free_holder(&h);
		}
	}

	return EXIT_SUCCESS;
}
