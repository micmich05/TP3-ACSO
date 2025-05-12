#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "inode.h"
#include "diskimg.h"

int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
    //caso borde: inumber < 1 y parametros nulos
    if (!fs || !inp || inumber < 1) {
        return -1;
    }
    
    //caso borde: inumber > max_inodes (superblock.s_isize * INODES_PER_BLOCK), fuera de rango
    int max_inodes = fs->superblock.s_isize * INODES_PER_BLOCK;
    if (inumber > max_inodes) {
        return -1;  
    }

    //sector donde está el inodo
    int inodes_per_sector = DISKIMG_SECTOR_SIZE / sizeof(struct inode);
    int sector = INODE_START_SECTOR + (inumber - 1) / inodes_per_sector;
    
    //offset dentro del sector
    int offset = (inumber - 1) % inodes_per_sector;
    
    //buffer para leer el sector completo
    struct inode sector_buffer[inodes_per_sector];
    
    //chequeo de lectura del sector
    if (diskimg_readsector(fs->dfd, sector, sector_buffer) != DISKIMG_SECTOR_SIZE) {
        return -1;
    }
    
    //copio el inodo específico al buffer proporcionado
    *inp = sector_buffer[offset];
    
    return 0;
}

/**
 * Busca el número de bloque físico correspondiente al índice lógico en un inodo.
 */

int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
    //caso borde: parámetros nulos o blockNum < 0
    if (!fs || !inp || blockNum < 0) {
        return -1;
    }
    
    //verifico si el inodo es grande o chico
    int is_large = (inp->i_mode & ILARG) != 0;
    
    //casos según el tipo de archivo (grande o chico)
    if (!is_large) {
        //bloques directos
        if (blockNum >= 8) {
            return -1;  //fuera de rango
        }
        return inp->i_addr[blockNum];
    } else {
        //bloques indirectos
        unsigned short indirect_block[256];  // 512 bytes / 2 bytes por entrada = 256 entradas
        
        //primeros 7 bloques indirectos simples (cada uno con 256 bloques)
        if (blockNum < 7 * 256) {
            int indirect_index = blockNum / 256;
            int offset = blockNum % 256;
            
            //lectura del bloque indirecto
            if (diskimg_readsector(fs->dfd, inp->i_addr[indirect_index], indirect_block) != DISKIMG_SECTOR_SIZE) {
                return -1;
            }
            
            return indirect_block[offset];
        }
        //bloque doblemente indirecto (el octavo)
        else if (blockNum < 7 * 256 + 256 * 256) {
            int offset_from_double_indirect = blockNum - (7 * 256);
            int indirect_index = offset_from_double_indirect / 256;
            int offset = offset_from_double_indirect % 256;

            //lectura del bloque doblemente indirecto
            unsigned short double_indirect_block[256];
            if (diskimg_readsector(fs->dfd, inp->i_addr[7], double_indirect_block) != DISKIMG_SECTOR_SIZE) {
                return -1;
            }

            //lectura del bloque indirecto correspondiente
            if (diskimg_readsector(fs->dfd, double_indirect_block[indirect_index], indirect_block) != DISKIMG_SECTOR_SIZE) {
                return -1;
            }
            
            return indirect_block[offset];
        } else {
            return -1;  //fuera de rango
        }
    }
}

int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
