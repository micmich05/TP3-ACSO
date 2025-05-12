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
    
    //tamaño del directorio y cuántos bloques ocupa
    int dir_size = inode_getsize(&in);
    int num_blocks = (dir_size + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;
    
    for (int block = 0; block < num_blocks; block++) {
        char buf[DISKIMG_SECTOR_SIZE];
        
        int bytes_read = file_getblock(fs, inumber, block, buf);
        if (bytes_read <= 0) {
            continue;  //error al leer este bloque, intento con el siguiente
        }
        
        //cuántas entradas de directorio hay en este bloque
        int entries_per_block = bytes_read / sizeof(struct direntv6);
        struct direntv6 *entries = (struct direntv6 *)buf;
        
        //cada entrada en este bloque
        for (int i = 0; i < entries_per_block; i++) {
            //la entrada está libre (d_inumber == 0
            if (entries[i].d_inumber == 0) {
                continue;
            }
            
            //comparo el nombre de la entrada con el nombre buscado
            if (strncmp(entries[i].d_name, name, sizeof(entries[i].d_name)) == 0) {
                //coincidencia, copio la entrada
                *dirEnt = entries[i];
                return 0;
            }
        }
    }
    
    //no encuentra la entrada
    return -1;
}
