#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int directory_findname(struct unixfilesystem *fs, const char *name, int inumber, struct direntv6 *dirEnt) {
    //chequeo de parámetros
    if (!fs || inumber < 1 || !name || !dirEnt) {
        return -1;
    }
    
    //inodo del directorio
    struct inode in;
    if (inode_iget(fs, inumber, &in) != 0) {
        return -1;
    }
    
    //verifico que sea un directorio
    if ((in.i_mode & IFMT) != IFDIR) {
        return -1;  //no es un directorio
    }

    size_t name_len = strlen(name);
    if (name_len > 14) {
        return -1;  //max size del nombre
    }
    
    //tamaño del directorio y cuántos bloques ocupa
    int dir_size = inode_getsize(&in);
    int num_blocks = (dir_size + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;
    
    char *buf = malloc(DISKIMG_SECTOR_SIZE);
    if (!buf) {
        return -1;  //error de asignación de memoria
    }
    
    for (int block = 0; block < num_blocks; block++) {
        int bytes_read = file_getblock(fs, inumber, block, buf);
        if (bytes_read <= 0) {
           continue;  //intento con el siguiente
        }
        
        //cuántas entradas de directorio hay en este bloque
        int entries_per_block = bytes_read / sizeof(struct direntv6);
        struct direntv6 *entries = (struct direntv6 *)buf;
        
        //cada entrada en este bloque
        for (int i = 0; i < entries_per_block; i++) {
            //la entrada está libre (d_inumber == 0)
            if (entries[i].d_inumber == 0) {
                continue;
            }
            
            if (strncmp(entries[i].d_name, name, name_len) == 0 && 
                (name_len == 14 || entries[i].d_name[name_len] == '\0')) {
                //coincidencia, copio la entrada
                *dirEnt = entries[i];
                free(buf);  
                return 0;   
            }
        }
    }
    
    //no se encontró nada
    free(buf);
    return -1;
}