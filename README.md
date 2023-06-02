# liburl
Fast c URL parser


Compile on linux

cd project_path

cmake .

make


For parse URL use function from "uri_parse.h"

	enum uri_res parse_uri(const char *uri, struct uri *build);

It is parse input string "uri", use finite state machine, and init structure "build".


For normalize URL use function from "encode.h"

	struct holder encode(const struct uri *source);
  
It is allocate memory and store to it normalize URL (encode, punicode, etc.)

For free memory use

	struct holder *free_holder(struct holder *h);


You can build relative URL

	struct holder relative_uri(const struct uri *base, const struct uri *relative);

You can write own logic for build relative URL.


Good luck with projects.
