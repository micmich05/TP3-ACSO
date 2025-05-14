#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "inode.h"
#include "diskimg.h"

//onstantes para inode_iget 
#define INODES_PER_BLOCK (DISKIMG_SECTOR_SIZE / sizeof(struct inode))
#define INODES_PER_SECTOR INODES_PER_BLOCK

//constantes para inode_indexlookup
#define DIRECT_BLOCKS_COUNT 8
#define ADDRESSES_PER_BLOCK 256
#define SINGLE_INDIRECT_BLOCKS 7
#define DOUBLE_INDIRECT_BLOCK_INDEX 7
#define MAX_SINGLE_INDIRECT_BLOCKS (SINGLE_INDIRECT_BLOCKS * ADDRESSES_PER_BLOCK)
#define MAX_DOUBLE_INDIRECT_BLOCKS (ADDRESSES_PER_BLOCK * ADDRESSES_PER_BLOCK)
#define MAX_FILE_BLOCKS (MAX_SINGLE_INDIRECT_BLOCKS + MAX_DOUBLE_INDIRECT_BLOCKS)

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
    int sector = INODE_START_SECTOR + (inumber - 1) / INODES_PER_SECTOR;
    
    //offset dentro del sector
    int offset = (inumber - 1) % INODES_PER_SECTOR;
    
    //buffer para leer el sector completo
    struct inode sector_buffer[INODES_PER_SECTOR];
    
    //chequeo de lectura del sector
    if (diskimg_readsector(fs->dfd, sector, sector_buffer) != DISKIMG_SECTOR_SIZE) {
        return -1;
    }
    
    //verificar que el inodo esté asignado
    if (!(sector_buffer[offset].i_mode & IALLOC)) {
        return -1;
    }
    
    //copio el inodo específico al buffer proporcionado
    *inp = sector_buffer[offset];
    
    return 0;
}

/**
 * Busca el número de bloque físico correspondiente al índice lógico en un inodo.
 */

// int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
//     //caso borde: parámetros nulos o blockNum < 0
//     if (!fs || !inp || blockNum < 0) {
//         return -1;
//     }
    
//     //verifico si el inodo es grande o chico
//     int is_large = (inp->i_mode & ILARG) != 0;
    
//     //casos según el tipo de archivo (grande o chico)
//     if (!is_large) {
//         //bloques directos
//         if (blockNum >= DIRECT_BLOCKS_COUNT) {
//             return -1;  //fuera de rango
//         }
//         return inp->i_addr[blockNum];
//     } else {
//         //bloques indirectos
//         unsigned short buffer[ADDRESSES_PER_BLOCK];  
        
//         //primeros 7 bloques indirectos simples (cada uno con 256 bloques)
//         if (blockNum < MAX_SINGLE_INDIRECT_BLOCKS) {
//             int indirect_index = blockNum / ADDRESSES_PER_BLOCK;
//             int offset = blockNum % ADDRESSES_PER_BLOCK;
            
//             //lectura del bloque indirecto
//             if (diskimg_readsector(fs->dfd, inp->i_addr[indirect_index], buffer) != DISKIMG_SECTOR_SIZE) {
//                 return -1;
//             }
            
//             return buffer[offset];
//         }
//         //bloque doblemente indirecto (el octavo)
//         else if (blockNum < MAX_SINGLE_INDIRECT_BLOCKS + MAX_DOUBLE_INDIRECT_BLOCKS) {
//             int offset_from_double_indirect = blockNum - MAX_SINGLE_INDIRECT_BLOCKS;
//             int indirect_index = offset_from_double_indirect / ADDRESSES_PER_BLOCK;
//             int offset = offset_from_double_indirect % ADDRESSES_PER_BLOCK;

//             //bloque doblemente indirecto
//             if (diskimg_readsector(fs->dfd, inp->i_addr[DOUBLE_INDIRECT_BLOCK_INDEX], buffer) != DISKIMG_SECTOR_SIZE) {
//                 return -1;
//             }

//             //dirección del bloque indirecto
//             unsigned short indirect_block_addr = buffer[indirect_index];
            
//             if (diskimg_readsector(fs->dfd, indirect_block_addr, buffer) != DISKIMG_SECTOR_SIZE) {
//                 return -1;
//             }
            
//             return buffer[offset];
//         } else {
//             return -1;  //fuera de rango
//         }
//     }
// }

int inode_indexlookup_malloc(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
    // caso borde: parámetros nulos o blockNum < 0
    if (!fs || !inp || blockNum < 0) {
        return -1;
    }
    
    // verifico si el inodo es grande o chico
    int is_large = (inp->i_mode & ILARG) != 0;
    
    if (!is_large) {
        // Bloques directos
        if (blockNum >= DIRECT_BLOCKS_COUNT) {
            return -1;  // fuera de rango
        }
        return inp->i_addr[blockNum];
    } else {
        // Asigno dinámicamente el buffer para los bloques indirectos
        unsigned short *buffer = malloc(ADDRESSES_PER_BLOCK * sizeof(unsigned short));
        if (!buffer) {
            return -1;
        }
        
        // Primeros 7 bloques indirectos simples (cada uno con 256 bloques)
        if (blockNum < MAX_SINGLE_INDIRECT_BLOCKS) {
            int indirect_index = blockNum / ADDRESSES_PER_BLOCK;
            int offset = blockNum % ADDRESSES_PER_BLOCK;
            
            if (diskimg_readsector(fs->dfd, inp->i_addr[indirect_index], buffer) != DISKIMG_SECTOR_SIZE) {
                free(buffer);
                return -1;
            }
            
            int ret = buffer[offset];
            free(buffer);
            return ret;
        }
        // Bloque doblemente indirecto (el octavo)
        else if (blockNum < MAX_SINGLE_INDIRECT_BLOCKS + MAX_DOUBLE_INDIRECT_BLOCKS) {
            int offset_from_double_indirect = blockNum - MAX_SINGLE_INDIRECT_BLOCKS;
            int indirect_index = offset_from_double_indirect / ADDRESSES_PER_BLOCK;
            int offset = offset_from_double_indirect % ADDRESSES_PER_BLOCK;
            
            // Lectura del bloque doblemente indirecto
            if (diskimg_readsector(fs->dfd, inp->i_addr[DOUBLE_INDIRECT_BLOCK_INDEX], buffer) != DISKIMG_SECTOR_SIZE) {
                free(buffer);
                return -1;
            }
            
            unsigned short indirect_block_addr = buffer[indirect_index];
            
            if (diskimg_readsector(fs->dfd, indirect_block_addr, buffer) != DISKIMG_SECTOR_SIZE) {
                free(buffer);
                return -1;
            }
            
            int ret = buffer[offset];
            free(buffer);
            return ret;
        } else {
            free(buffer);
            return -1;  // fuera de rango
        }
    }
}

int inode_getsize(struct inode *inp) {
    if (!inp) {
        return -1;  //puntero nulo
    }
    return ((inp->i_size0 << 16) | inp->i_size1); //combino los dos campos de tamaño con un desplazamiento y una operación OR
}