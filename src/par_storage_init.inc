/*
 * Private include fragment. Included only by par.c.
 */

#define PAR_STORAGE_INIT_NOP(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)
#define PAR_STORAGE_U8_FROM_U8(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)   (par_atomic_u8_t)((uint8_t)(def_)),
#define PAR_STORAGE_U8_FROM_I8(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)   (par_atomic_u8_t)((uint8_t)(int8_t)(def_)),
#define PAR_STORAGE_U16_FROM_U16(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_) (par_atomic_u16_t)((uint16_t)(def_)),
#define PAR_STORAGE_U16_FROM_I16(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_) (par_atomic_u16_t)((uint16_t)(int16_t)(def_)),
#define PAR_STORAGE_U32_FROM_U32(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_) (par_atomic_u32_t)((uint32_t)(def_)),
#define PAR_STORAGE_U32_FROM_I32(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_) (par_atomic_u32_t)((uint32_t)(int32_t)(def_)),
#if ( 1 == PAR_CFG_ENABLE_TYPE_F32 )
#define PAR_STORAGE_U32_FROM_F32(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_) (par_atomic_u32_t)(0u),
#endif

#define PAR_ITEM_U8   PAR_STORAGE_U8_FROM_U8
#define PAR_ITEM_U16  PAR_STORAGE_INIT_NOP
#define PAR_ITEM_U32  PAR_STORAGE_INIT_NOP
#define PAR_ITEM_I8   PAR_STORAGE_U8_FROM_I8
#define PAR_ITEM_I16  PAR_STORAGE_INIT_NOP
#define PAR_ITEM_I32  PAR_STORAGE_INIT_NOP
#define PAR_ITEM_F32  PAR_STORAGE_INIT_NOP
static par_atomic_u8_t gs_par_u8_storage[PAR_STORAGE_NONZERO(PAR_STORAGE_COUNT8)] =
{
    #include "../../par_table.def"
};
#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32

#define PAR_ITEM_U8   PAR_STORAGE_INIT_NOP
#define PAR_ITEM_U16  PAR_STORAGE_U16_FROM_U16
#define PAR_ITEM_U32  PAR_STORAGE_INIT_NOP
#define PAR_ITEM_I8   PAR_STORAGE_INIT_NOP
#define PAR_ITEM_I16  PAR_STORAGE_U16_FROM_I16
#define PAR_ITEM_I32  PAR_STORAGE_INIT_NOP
#define PAR_ITEM_F32  PAR_STORAGE_INIT_NOP
static par_atomic_u16_t gs_par_u16_storage[PAR_STORAGE_NONZERO(PAR_STORAGE_COUNT16)] =
{
    #include "../../par_table.def"
};
#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32

#define PAR_ITEM_U8   PAR_STORAGE_INIT_NOP
#define PAR_ITEM_U16  PAR_STORAGE_INIT_NOP
#define PAR_ITEM_U32  PAR_STORAGE_U32_FROM_U32
#define PAR_ITEM_I8   PAR_STORAGE_INIT_NOP
#define PAR_ITEM_I16  PAR_STORAGE_INIT_NOP
#define PAR_ITEM_I32  PAR_STORAGE_U32_FROM_I32
#if ( 1 == PAR_CFG_ENABLE_TYPE_F32 )
    #define PAR_ITEM_F32  PAR_STORAGE_U32_FROM_F32
#else
    #define PAR_ITEM_F32  PAR_STORAGE_INIT_NOP
#endif
static par_atomic_u32_t gs_par_u32_storage[PAR_STORAGE_NONZERO(PAR_STORAGE_COUNT32)] =
{
    #include "../../par_table.def"
};
#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32

#undef PAR_STORAGE_INIT_NOP
#undef PAR_STORAGE_U8_FROM_U8
#undef PAR_STORAGE_U8_FROM_I8
#undef PAR_STORAGE_U16_FROM_U16
#undef PAR_STORAGE_U16_FROM_I16
#undef PAR_STORAGE_U32_FROM_U32
#undef PAR_STORAGE_U32_FROM_I32
#if ( 1 == PAR_CFG_ENABLE_TYPE_F32 )
#undef PAR_STORAGE_U32_FROM_F32
#endif
