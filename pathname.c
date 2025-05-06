#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/**
 * Busca el inodo correspondiente a una ruta absoluta.
 *
 * @param fs Sistema de archivos Unix
 * @param pathname Ruta absoluta a buscar
 * @return Número de inodo si se encuentra, -1 en caso de error
 */
int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    // Verificamos parámetros
    if (!fs || !pathname) {
        return -1;
    }
    
    // Caso especial: directorio raíz "/"
    if (pathname[0] == '/' && pathname[1] == '\0') {
        return 1;  // El inodo 1 es la raíz en Unix v6
    }
    
    // Verificamos que sea una ruta absoluta
    if (pathname[0] != '/') {
        return -1;
    }
    
    // Comenzamos desde el directorio raíz
    int current_inumber = 1;
    
    // Hacemos una copia de la ruta para poder modificarla con strtok
    char path_copy[strlen(pathname) + 1];
    strcpy(path_copy, pathname);
    
    // Tokenizamos la ruta por '/'
    char *token = strtok(path_copy, "/");
    
    // Si la ruta es solo "/", strtok retornará NULL
    if (token == NULL) {
        return current_inumber;
    }
    
    // Recorremos cada componente de la ruta
    while (token != NULL) {
        // Buscamos este componente en el directorio actual
        struct direntv6 dir_entry;
        // Corregido: directory_findname(fs, nombre, inumber, dirEnt)
        if (directory_findname(fs, token, current_inumber, &dir_entry) != 0) {
            return -1;  // No encontramos el componente
        }
        
        // Actualizamos el inodo actual
        current_inumber = dir_entry.d_inumber;
        
        // Obtenemos el siguiente componente
        token = strtok(NULL, "/");
        
        // Si aún hay más componentes, verificamos que el actual sea un directorio
        if (token != NULL) {
            struct inode in;
            if (inode_iget(fs, current_inumber, &in) != 0) {
                return -1;
            }
            
            // Verificamos que sea un directorio
            if ((in.i_mode & IFMT) != IFDIR) {
                return -1;  // No es un directorio y todavía hay más componentes
            }
        }
    }
    
    return current_inumber;
}
