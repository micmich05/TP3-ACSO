#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "file.h"
#include "inode.h"
#include "diskimg.h"

int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
    //caso borde: parámetros nulos o blockNum < 0
    if (!fs || blockNum < 0 || !buf) {
        return -1;
    }
    
    struct inode in;
    if (inode_iget(fs, inumber, &in) != 0) {
        return -1;
    }
    
    int file_size = inode_getsize(&in);
    
    //chequeo si el bloque solicitado está fuera del archivo
    int max_block = (file_size + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;
    if (blockNum >= max_block) {
        return -1;  //fuera del rango del archivo
    }
    
    //número de bloque físico con indexlookup
    int physical_block = inode_indexlookup(fs, &in, blockNum);
    if (physical_block <= 0) {
        return -1;  //error al obtener el bloque físico
    }
    
    if (diskimg_readsector(fs->dfd, physical_block, buf) != DISKIMG_SECTOR_SIZE) {
        return -1;  //error al leer el sector
    }

    //calculo cuántos bytes válidos hay en este bloque
    //para bloques intermedios es DISKIMG_SECTOR_SIZE
    //para el último bloque puede ser menos
    if (blockNum == max_block - 1 && file_size % DISKIMG_SECTOR_SIZE != 0) {
        return file_size % DISKIMG_SECTOR_SIZE;
    } else {
        return DISKIMG_SECTOR_SIZE;
    }
}