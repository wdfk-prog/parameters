#ifndef _PAR_DEF_CORE_H_
#define _PAR_DEF_CORE_H_

#include <stdint.h>

typedef struct par_cfg_s par_cfg_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  List of device parameters
 *
 * @note Must be started with 0!
 */
#define PAR_ITEM_ENUM(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_) enum_,
enum
{
    #define PAR_ITEM_U8   PAR_ITEM_ENUM
    #define PAR_ITEM_U16  PAR_ITEM_ENUM
    #define PAR_ITEM_U32  PAR_ITEM_ENUM
    #define PAR_ITEM_I8   PAR_ITEM_ENUM
    #define PAR_ITEM_I16  PAR_ITEM_ENUM
    #define PAR_ITEM_I32  PAR_ITEM_ENUM
    #define PAR_ITEM_F32  PAR_ITEM_ENUM
    #include "../../par_table.def"
    #undef PAR_ITEM_U8
    #undef PAR_ITEM_U16
    #undef PAR_ITEM_U32
    #undef PAR_ITEM_I8
    #undef PAR_ITEM_I16
    #undef PAR_ITEM_I32
    #undef PAR_ITEM_F32

    ePAR_NUM_OF
};
#undef PAR_ITEM_ENUM
typedef uint16_t par_num_t;

const par_cfg_t * par_cfg_get_table      (void);
const par_cfg_t * par_cfg_get            (const par_num_t par_num);
uint32_t          par_cfg_get_table_size (void);

#ifdef __cplusplus
}
#endif

#endif /* _PAR_DEF_CORE_H_ */
