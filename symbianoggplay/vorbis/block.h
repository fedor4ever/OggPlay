/********************************************************************
 *
 * Vorbis INTERNAL Primitives: 
 *
 * PCM data vector blocking, windowing and dis/reassembly
 *
 ********************************************************************/

void * _vorbis_block_alloc(vorbis_block *vb,long bytes);
void _vorbis_block_ripcord(vorbis_block *vb);