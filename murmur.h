/******************************************************************************
 * Filename: murmur.h
 *
 * Created: 2016/05/04 20:38
 * Author: Ladislav Láska
 * e-mail: laska@kam.mff.cuni.cz
 *
 ******************************************************************************/

#ifndef __MURMUR_H__
#define __MURMUR_H__

#include <cstdint>

uint32_t MurmurHash1 ( const void * key, int len, uint32_t seed );
uint32_t MurmurHash1 ( uint64_t key, uint32_t seed );


#endif
