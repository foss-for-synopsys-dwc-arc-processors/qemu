#ifndef ARC_DECODER_H
#define ARC_DECODER_H

#if defined(TARGET_ARCV2)
  #include "decoder-v2.h"
#elif defined(TARGET_ARCV3)
  #include "decoder-v3.h"
#else
#error "Should not happen. Something is wrong!"
#endif

#endif
