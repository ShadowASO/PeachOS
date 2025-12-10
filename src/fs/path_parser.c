#include "path_parser.h"


struct path_root * path_parser(const char * path, const char * current_dir_path);
void path_free(struct path_root * root);