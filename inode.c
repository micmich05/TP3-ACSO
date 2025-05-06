#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "inode.h"
#include "diskimg.h"

int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
    // Verificamos que el número de inodo sea válido
    if (!fs || !inp || inumber < 1) {
        return -1;
    }

    // Calculamos el sector donde está el inodo
    // Cada sector tiene 512 bytes (DISKIMG_SECTOR_SIZE) y cada inodo ocupa 32 bytes (sizeof(struct inode))
    int inodes_per_sector = DISKIMG_SECTOR_SIZE / sizeof(struct inode);
    int sector = INODE_START_SECTOR + (inumber - 1) / inodes_per_sector;
    
    // Calculamos el offset dentro del sector
    int offset = (inumber - 1) % inodes_per_sector;
    
    // Buffer para leer el sector completo
    struct inode sector_buffer[inodes_per_sector];
    
    // Leemos el sector
    if (diskimg_readsector(fs->dfd, sector, sector_buffer) != DISKIMG_SECTOR_SIZE) {
        return -1;
    }
    
    // Copiamos el inodo específico al buffer proporcionado
    *inp = sector_buffer[offset];
    
    return 0;
}

/**
 * Busca el número de bloque físico correspondiente al índice lógico en un inodo.
 */

int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
    // Verificamos parámetros
    if (!fs || !inp || blockNum < 0) {
        return -1;
    }
    
    // Verificamos si es un archivo grande (ILARG bit establecido)
    int is_large = (inp->i_mode & ILARG) != 0;
    
    // Casos según el tipo de archivo (grande o pequeño)
    if (!is_large) {
        // Archivo pequeño: bloques directos
        if (blockNum >= 8) {
            return -1;  // Índice fuera de rango
        }
        return inp->i_addr[blockNum];
    } else {
        // Archivo grande: bloques indirectos
        unsigned short indirect_block[256];  // 512 bytes / 2 bytes por entrada = 256 entradas
        
        // Primeros 7 bloques indirectos simples (cada uno con 256 bloques)
        if (blockNum < 7 * 256) {
            int indirect_index = blockNum / 256;
            int offset = blockNum % 256;
            
            // Leemos el bloque indirecto
            if (diskimg_readsector(fs->dfd, inp->i_addr[indirect_index], indirect_block) != DISKIMG_SECTOR_SIZE) {
                return -1;
            }
            
            return indirect_block[offset];
        }
        // Bloque doblemente indirecto (el octavo)
        else if (blockNum < 7 * 256 + 256 * 256) {
            int offset_from_double_indirect = blockNum - (7 * 256);
            int indirect_index = offset_from_double_indirect / 256;
            int offset = offset_from_double_indirect % 256;
            
            // Leemos el bloque doblemente indirecto
            unsigned short double_indirect_block[256];
            if (diskimg_readsector(fs->dfd, inp->i_addr[7], double_indirect_block) != DISKIMG_SECTOR_SIZE) {
                return -1;
            }
            
            // Leemos el bloque indirecto correspondiente
            if (diskimg_readsector(fs->dfd, double_indirect_block[indirect_index], indirect_block) != DISKIMG_SECTOR_SIZE) {
                return -1;
            }
            
            return indirect_block[offset];
        } else {
            return -1;  // Índice fuera de rango
        }
    }
}

int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
