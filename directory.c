#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

//int directory_findname(struct unixfilesystem *fs, const char *name, int inumber, struct direntv6 *dirEnt) {
//    //chequeo de parámetros
//    if (!fs || inumber < 1 || !name || !dirEnt) {
//        return -1;
//    }
    
//    //inodo del directorio
//    struct inode in;
//    if (inode_iget(fs, inumber, &in) != 0) {
//        return -1;
//    }
    
    //verifico que sea un directorio
//    if ((in.i_mode & IFMT) != IFDIR) {
//        return -1;  //no es un directorio
//    }
    
    //tamaño del directorio y cuántos bloques ocupa
//    int dir_size = inode_getsize(&in);
//    int num_blocks = (dir_size + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;
    
//    for (int block = 0; block < num_blocks; block++) {
//        char buf[DISKIMG_SECTOR_SIZE];
        
//        int bytes_read = file_getblock(fs, inumber, block, buf);
//        if (bytes_read <= 0) {
//            continue;  //error al leer este bloque, intento con el siguiente
//        }
        
        //cuántas entradas de directorio hay en este bloque
//        int entries_per_block = bytes_read / sizeof(struct direntv6);
//        struct direntv6 *entries = (struct direntv6 *)buf;
        
        //cada entrada en este bloque
//        for (int i = 0; i < entries_per_block; i++) {
            //la entrada está libre (d_inumber == 0
//            if (entries[i].d_inumber == 0) {
//                continue;
//            }
            
            //comparo el nombre de la entrada con el nombre buscado
//            if (strncmp(entries[i].d_name, name, sizeof(entries[i].d_name)) == 0) {
                //coincidencia, copio la entrada
//                *dirEnt = entries[i];
//                return 0;
//            }
//        }
//    }
    
//    //no encuentra la entrada
//    return -1;
//}
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
    
    //tamaño del directorio y total de entradas
    int dir_size = inode_getsize(&in);
    int total_entries = dir_size / sizeof(struct direntv6);
    
    //búsqueda binaria en las entradas del directorio
    int low = 0;
    int high = total_entries - 1;
    
    while (low <= high) {
        int mid = low + (high - low) / 2;
        
        // Calculo posición de la entrada en el archivo
        int block_num = (mid * sizeof(struct direntv6)) / DISKIMG_SECTOR_SIZE;
        int offset_in_block = (mid * sizeof(struct direntv6)) % DISKIMG_SECTOR_SIZE;
        
        // Leo el bloque que contiene la entrada
        char buf[DISKIMG_SECTOR_SIZE];
        if (file_getblock(fs, inumber, block_num, buf) <= 0) {
            return -1;
        }
        
        // Obtengo la entrada específica
        struct direntv6 *mid_entry = (struct direntv6 *)(buf + offset_in_block);
        
        // Si la entrada está libre (d_inumber == 0), necesitamos una búsqueda especial
        if (mid_entry->d_inumber == 0) {
            // Búsqueda secuencial para entradas libres (puede ser menos eficiente)
            return linear_search_in_directory(fs, name, inumber, dirEnt);
        }
        
        // Comparo el nombre de la entrada con el buscado
        int cmp = strncmp(mid_entry->d_name, name, sizeof(mid_entry->d_name));
        
        if (cmp == 0) {
            // Encontramos la entrada
            *dirEnt = *mid_entry;
            return 0;
        } else if (cmp < 0) {
            // El nombre en mid_entry es menor, busco en la mitad superior
            low = mid + 1;
        } else {
            // El nombre en mid_entry es mayor, busco en la mitad inferior
            high = mid - 1;
        }
    }
    
    // No se encontró la entrada
    return -1;
}

// Función auxiliar para búsqueda lineal (necesaria cuando hay entradas libres)
int linear_search_in_directory(struct unixfilesystem *fs, const char *name, int inumber, struct direntv6 *dirEnt) {
    struct inode in;
    if (inode_iget(fs, inumber, &in) != 0) {
        return -1;
    }
    
    int dir_size = inode_getsize(&in);
    int num_blocks = (dir_size + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;
    
    for (int block = 0; block < num_blocks; block++) {
        char buf[DISKIMG_SECTOR_SIZE];
        
        int bytes_read = file_getblock(fs, inumber, block, buf);
        if (bytes_read <= 0) {
            continue;
        }
        
        int entries_per_block = bytes_read / sizeof(struct direntv6);
        struct direntv6 *entries = (struct direntv6 *)buf;
        
        for (int i = 0; i < entries_per_block; i++) {
            if (entries[i].d_inumber == 0) {
                continue;
            }
            
            if (strncmp(entries[i].d_name, name, sizeof(entries[i].d_name)) == 0) {
                *dirEnt = entries[i];
                return 0;
            }
        }
    }
    
    return -1;
}