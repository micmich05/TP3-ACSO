#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "file.h"
#include "inode.h"
#include "diskimg.h"

int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
    // Verificamos parámetros
    if (!fs || blockNum < 0 || !buf) {
        return -1;
    }
    
    // Obtenemos información del inodo
    struct inode in;
    if (inode_iget(fs, inumber, &in) != 0) {
        return -1;
    }
    
    // Calculamos el tamaño total del archivo
    int file_size = inode_getsize(&in);
    
    // Verificamos si el bloque solicitado está fuera del archivo
    int max_block = (file_size + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;
    if (blockNum >= max_block) {
        return -1;  // Bloque fuera del rango del archivo
    }
    
    // Obtenemos el número de bloque físico mediante indexlookup
    int physical_block = inode_indexlookup(fs, &in, blockNum);
    if (physical_block <= 0) {
        return -1;  // Error al obtener el bloque físico
    }
    
    // Leemos el bloque físico
    if (diskimg_readsector(fs->dfd, physical_block, buf) != DISKIMG_SECTOR_SIZE) {
        return -1;  // Error al leer el sector
    }
    
    // Calculamos cuántos bytes válidos hay en este bloque
    // Para bloques intermedios es DISKIMG_SECTOR_SIZE
    // Para el último bloque puede ser menos
    if (blockNum == max_block - 1 && file_size % DISKIMG_SECTOR_SIZE != 0) {
        return file_size % DISKIMG_SECTOR_SIZE;
    } else {
        return DISKIMG_SECTOR_SIZE;
    }
}