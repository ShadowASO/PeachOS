#ifndef PATH_H
#define PATH_H

#include <stddef.h>
#include <stdint.h>

/* Ajuste conforme seu kernel */
#ifndef PATH_MAX
//#define PATH_MAX 4096
#define PATH_MAX 108
#endif

#ifndef PATH_MAX_LEVELS
#define PATH_MAX_LEVELS 64
#endif

/* Retorna 1 se o caminho é absoluto, 0 caso contrário */
int path_is_absolute(const char *path);

/* 
 * Normaliza um caminho "path" em relação ao diretório atual "cwd".
 *
 * Exemplos:
 *   cwd = "/home/user", path = "docs/../file.txt"  => "/home/user/file.txt"
 *   cwd = "/home/user", path = "/etc/../var/log"   => "/var/log"
 *   cwd = "/",           path = ".."               => "/"
 *
 * Retorno:
 *   0 em sucesso
 *  -1 se o caminho resultante estourar o buffer out/out_size
 */
int path_resolve(const char *cwd,
                 const char *path,
                 char *out,
                 size_t out_size);

/*
 * Divide um caminho já normalizado (absoluto) em componentes.
 * Ex.: "/usr/local/bin" → components = {"usr","local","bin"}, *count = 3
 *
 * OBS: Essa função modifica o buffer "path" (coloca '\0' onde havia '/').
 *      Então, passe um buffer MUTÁVEL (ex.: uma cópia em heap/stack).
 *
 * Retorno:
 *   0 em sucesso
 *  -1 se exceder PATH_MAX_LEVELS
 */
int path_split_inplace(char *path,
                       char *components[PATH_MAX_LEVELS],
                       int *count);

#endif /* PATH_H */
