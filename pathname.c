#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    //verifico parametros no nulos
    if (!fs || !pathname) {
        return -1;
    }
    
    //caso especial: directorio raíz "/"
    if (pathname[0] == '/' && pathname[1] == '\0') {
        return 1;  //el inodo 1 es el root
    }
    
    //ruta absoluta
    if (pathname[0] != '/') {
        return -1;
    }
    
    //arranco en el root
    int current_inumber = 1;
    
    //copia de la ruta para poder modificarla con strtok
    char path_copy[strlen(pathname) + 1];
    strcpy(path_copy, pathname);
    
    //chequeo que la copia no haya fallado
    if (!path_copy) {
        return -1;
    }

    char *token = strtok(path_copy, "/");
    
    //si la ruta es solo "/", strtok devuelve NULL
    if (!token) {
        return current_inumber;
    }
    
    //cada componente de la ruta
    while (token) {
        //busco este componente en el directorio actual
        struct direntv6 dir_entry;
        if (directory_findname(fs, token, current_inumber, &dir_entry) != 0) {
            return -1;  //no lo encuentra 
        }
        
        //actualizo el inodo actual
        current_inumber = dir_entry.d_inumber;
        
        //siguiente componente
        token = strtok(NULL, "/");
        
        //si hay más componentes, verifico que el actual sea un directorio
        if (token) {
            struct inode in;
            if (inode_iget(fs, current_inumber, &in) != 0) {
                return -1;
            }
            
            //chequeo que sea un directorio
            if ((in.i_mode & IFMT) != IFDIR) {
                return -1;  //no es un directorio y todavía hay más componentes
            }
        }
    }
    
    return current_inumber;
}
