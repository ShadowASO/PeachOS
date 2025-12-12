#include "path.h"
#include "../klib/string.h"
#include "../klib/memory.h"





/* Retorna 1 se o caminho é absoluto (começa com '/') */
int path_is_absolute(const char *path)
{
    if (!path) return 0;
    return (path[0] == '/') ? 1 : 0;
}

/*
 * Função interna: monta em 'tmp' um caminho "base + path".
 * Se path é absoluto, ignora o base.
 * Se path é relativo, concatena base + "/" + path.
 * Retorna o tamanho em bytes (sem contar o '\0') ou -1 em erro de tamanho.
 */
static int path_build_full(const char *cwd,
                           const char *path,
                           char *tmp,
                           size_t tmp_size)
{
    size_t len = 0;

    if (!path || !tmp || tmp_size == 0) {
        return -1;
    }

    if (path_is_absolute(path) || !cwd || cwd[0] == '\0') {
        /* Basta copiar path */
        size_t plen = kstrlen(path);
        if (plen + 1 > tmp_size) {
            return -1;
        }
        kmemcpy(tmp, path, plen + 1); /* copia também o '\0' */
        return (int)plen;
    }

    /* Caminho relativo: começamos do cwd */
    size_t clen = kstrlen(cwd);
    size_t plen = kstrlen(path);

    /* Precisaremos de: cwd + "/" + path + '\0' (no pior caso) */
    if (clen + 1 + plen + 1 > tmp_size) {
        return -1;
    }

    /* Copia cwd */
    kmemcpy(tmp, cwd, clen);
    len = clen;

    /* Se cwd não termina com '/', acrescenta */
    if (len == 0 || tmp[len - 1] != '/') {
        tmp[len++] = '/';
    }

    /* Copia path */
    kmemcpy(tmp + len, path, plen);
    len += plen;
    tmp[len] = '\0';

    return (int)len;
}

/*
 * Função interna: normaliza um caminho em-place, tratando ".", ".." e "//".
 * Recebe um caminho ABSOLUTO em tmp.
 * Exemplo de entrada: "/home//user/./docs/../file.txt"
 * Exemplo de saída:   "/home/user/file.txt"
 *
 * Retorna:
 *   novo comprimento (sem '\0') ou -1 em erro.
 */
static int path_normalize_inplace(char *tmp)
{
    const char *p;
    const char *end;
    const char *segment_start;
    char *out = tmp;
    char *stack[PATH_MAX_LEVELS];    /* ponteiros para início dos segmentos */
    int  stack_top = 0;

    if (!tmp) return -1;

    size_t len = kstrlen(tmp);
    if (len == 0) {
        /* Faz de conta que é "/" */
        tmp[0] = '/';
        tmp[1] = '\0';
        return 1;
    }

    /* Garante que começa com '/' (para essa função interna) */
    if (tmp[0] != '/') {
        return -1;
    }

    p   = tmp;
    end = tmp + len;

    /* Pulamos o primeiro '/', raiz */
    p++;

    while (p <= end) {
        /* Encontrar fim do segmento (ou fim da string) */
        segment_start = p;
        while (p < end && *p != '/') {
            p++;
        }

        size_t seg_len = (size_t)(p - segment_start);

        if (seg_len == 0) {
            /* segmento vazio (barras repetidas) -> ignora */
        } else if (seg_len == 1 && segment_start[0] == '.') {
            /* "." -> ignora */
        } else if (seg_len == 2 &&
                   segment_start[0] == '.' &&
                   segment_start[1] == '.') {
            /* ".." -> remover um nível da pilha, se tiver */
            if (stack_top > 0) {
                stack_top--;
            }
        } else {
            /* segmento normal -> empilha */
            if (stack_top >= PATH_MAX_LEVELS) {
                return -1;
            }
            stack[stack_top++] = (char *)segment_start;
        }

        /* Se encontramos '/', pula ele */
        if (p < end && *p == '/') {
            p++;
        } else {
            /* p == end */
            break;
        }
    }

    /* Agora reconstrói em 'out' a partir da pilha */
    char *dst = out;
    *dst++ = '/';

    if (stack_top == 0) {
        /* raiz apenas */
        *dst = '\0';
        return 1;
    }

    for (int i = 0; i < stack_top; i++) {
        char *seg = stack[i];

        /* medir até próxima '/' ou '\0' */
        char *q = seg;
        while (*q != '/' && *q != '\0') {
            q++;
        }
        size_t seg_len = (size_t)(q - seg);

        /* copia '/' se não for o primeiro (já pus um antes) */
        if (i > 0) {
            *dst++ = '/';
        }

        /* copia segmento */
        for (size_t j = 0; j < seg_len; j++) {
            *dst++ = seg[j];
        }
    }

    *dst = '\0';
    return (int)(dst - out);
}

int path_resolve(const char *cwd,
                 const char *path,
                 char *out,
                 size_t out_size)
{
    char tmp[PATH_MAX];
    int len;

    if (!path || !out || out_size == 0) {
        return -1;
    }

    /* 1) Monta caminho completo base + path em tmp */
    len = path_build_full(cwd, path, tmp, sizeof(tmp));
    if (len < 0) {
        return -1;
    }

    /* 2) Normaliza "." e ".." em-place */
    len = path_normalize_inplace(tmp);
    if (len < 0) {
        return -1;
    }

    /* 3) Copia para o buffer de saída */
    if ((size_t)len + 1 > out_size) {
        return -1;
    }

    kmemcpy(out, tmp, (size_t)len + 1);
    return 0;
}

/*
 * Divide um caminho ABSOLUTO já normalizado em componentes.
 * Modifica o buffer (coloca '\0'), e preenche components[] com
 * ponteiros para cada componente.
 */
int path_split_inplace(char *path,
                       char *components[PATH_MAX_LEVELS],
                       int *count)
{
    if (!path || !components || !count) {
        return -1;
    }

    int   n = 0;
    char *p = path;

    /* Se começa com '/', pula */
    if (*p == '/') p++;

    while (*p != '\0') {
        if (n >= PATH_MAX_LEVELS) {
            return -1;
        }

        components[n++] = p;

        /* anda até o fim do componente */
        while (*p != '\0' && *p != '/') {
            p++;
        }

        if (*p == '/') {
            *p = '\0';  /* termina o componente */
            p++;
        }
    }

    *count = n;
    return 0;
}
