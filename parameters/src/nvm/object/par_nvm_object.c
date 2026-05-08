/**
 * @file par_nvm_object.c
 * @brief Implement NVM support for fixed-capacity object parameters.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-04-28
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include "par_nvm_object.h"

#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED)

#include <stddef.h>
#include <string.h>

#include "par_object.h"
#include "par_nvm_object_store.h"
#include "fnv.h"
#include "par_if.h"

/**
 * @brief Object persistence block signature.
 */
#define PAR_NVM_OBJECT_SIGN (0x504A424FU)
/**
 * @brief Object persistence serialized-layout version.
 */
#define PAR_NVM_OBJECT_VERSION (1U)

/**
 * @brief Serialized object persistence header offsets.
 */
#define PAR_NVM_OBJECT_HEAD_SIGN_OFFSET      (0U)
#define PAR_NVM_OBJECT_HEAD_VERSION_OFFSET   (PAR_NVM_OBJECT_HEAD_SIGN_OFFSET + (uint32_t)sizeof(uint32_t))
#define PAR_NVM_OBJECT_HEAD_OBJ_NB_OFFSET    (PAR_NVM_OBJECT_HEAD_VERSION_OFFSET + (uint32_t)sizeof(uint16_t))
#define PAR_NVM_OBJECT_HEAD_TABLE_ID_OFFSET  (PAR_NVM_OBJECT_HEAD_OBJ_NB_OFFSET + (uint32_t)sizeof(uint16_t))
#define PAR_NVM_OBJECT_HEAD_BODY_SIZE_OFFSET (PAR_NVM_OBJECT_HEAD_TABLE_ID_OFFSET + (uint32_t)sizeof(uint32_t))
#define PAR_NVM_OBJECT_HEAD_CRC_OFFSET       (PAR_NVM_OBJECT_HEAD_BODY_SIZE_OFFSET + (uint32_t)sizeof(uint32_t))
#define PAR_NVM_OBJECT_HEAD_SIZE             (PAR_NVM_OBJECT_HEAD_CRC_OFFSET + (uint32_t)sizeof(uint16_t))

/**
 * @brief Serialized object persistence record offsets.
 */
#define PAR_NVM_OBJECT_RECORD_ID_OFFSET        (0U)
#define PAR_NVM_OBJECT_RECORD_TYPE_OFFSET      (PAR_NVM_OBJECT_RECORD_ID_OFFSET + (uint32_t)sizeof(uint16_t))
#define PAR_NVM_OBJECT_RECORD_FLAGS_OFFSET     (PAR_NVM_OBJECT_RECORD_TYPE_OFFSET + (uint32_t)sizeof(uint8_t))
#define PAR_NVM_OBJECT_RECORD_ELEM_SIZE_OFFSET (PAR_NVM_OBJECT_RECORD_FLAGS_OFFSET + (uint32_t)sizeof(uint8_t))
#define PAR_NVM_OBJECT_RECORD_CAPACITY_OFFSET  (PAR_NVM_OBJECT_RECORD_ELEM_SIZE_OFFSET + (uint32_t)sizeof(uint16_t))
#define PAR_NVM_OBJECT_RECORD_LEN_OFFSET       (PAR_NVM_OBJECT_RECORD_CAPACITY_OFFSET + (uint32_t)sizeof(uint16_t))
#define PAR_NVM_OBJECT_RECORD_CRC_OFFSET       (PAR_NVM_OBJECT_RECORD_LEN_OFFSET + (uint32_t)sizeof(uint16_t))
#define PAR_NVM_OBJECT_RECORD_HEAD_SIZE        (PAR_NVM_OBJECT_RECORD_CRC_OFFSET + (uint32_t)sizeof(uint16_t))

/**
 * @brief Object persistence record flags reserved value.
 */
#define PAR_NVM_OBJECT_RECORD_FLAGS_NONE (0U)
/**
 * @brief Maximum temporary payload bytes used by object write verification.
 */
#define PAR_NVM_OBJECT_VERIFY_CHUNK_SIZE (32U)

/**
 * @brief Object persistence block header stored in platform-native byte order.
 */
typedef struct
{
    uint32_t sign;      /**< Object block signature. */
    uint16_t version;   /**< Object serialized-layout version. */
    uint16_t obj_nb;    /**< Stored persistent object count. */
    uint32_t table_id;  /**< Object persistent-schema digest. */
    uint32_t body_size; /**< Serialized object-record body size in bytes. */
    uint16_t crc;       /**< CRC-16 over version, obj_nb, table_id, and body_size. */
} par_nvm_object_head_t;

/**
 * @brief Object persistence record metadata stored before payload bytes.
 */
typedef struct
{
    uint16_t id;        /**< External parameter ID. */
    uint8_t type;       /**< Serialized par_type_list_t value. */
    uint8_t flags;      /**< Reserved flags. */
    uint16_t elem_size; /**< Object element size in bytes. */
    uint16_t capacity;  /**< Fixed payload capacity in bytes. */
    uint16_t len;       /**< Valid payload length in bytes. */
    uint16_t crc;       /**< CRC-16 over metadata except crc and valid payload bytes. */
} par_nvm_object_record_meta_t;

#if (1 == PAR_CFG_ENABLE_ID)
#define PAR_NVM_OBJECT_CFG_ID_VALUE(par_num_) (par_cfg_get_param_id_const(par_num_))
#else
#define PAR_NVM_OBJECT_CFG_ID_VALUE(par_num_) (0U)
#endif /* (1 == PAR_CFG_ENABLE_ID) */

#define PAR_OBJECT_PERSIST_SLOT_ENTRY_SELECT(enum_, pers_)   PAR_OBJECT_PERSIST_SLOT_ENTRY_SELECT_I(enum_, pers_)
#define PAR_OBJECT_PERSIST_SLOT_ENTRY_SELECT_I(enum_, pers_) PAR_OBJECT_PERSIST_SLOT_ENTRY_SELECT_##pers_(enum_)
#define PAR_OBJECT_PERSIST_SLOT_ENTRY_SELECT_1(enum_)        [PAR_OBJECT_PERSIST_IDX_##enum_] = enum_,
#define PAR_OBJECT_PERSIST_SLOT_ENTRY_SELECT_0(enum_)
#define PAR_ITEM_OBJECT_PERSIST_SLOT(...) PAR_OBJECT_PERSIST_SLOT_ENTRY_SELECT(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_PERS(__VA_ARGS__))
#define PAR_ITEM_NOP(...)
/**
 * @brief Compile-time mapping from persistent object slot to parameter number.
 */
static const par_num_t g_par_object_persist_slot_to_par_num[PAR_OBJECT_PERSIST_SLOT_MAP_CAPACITY] = {
#define PAR_ITEM_U8                      PAR_ITEM_NOP
#define PAR_ITEM_U16                     PAR_ITEM_NOP
#define PAR_ITEM_U32                     PAR_ITEM_NOP
#define PAR_ITEM_I8                      PAR_ITEM_NOP
#define PAR_ITEM_I16                     PAR_ITEM_NOP
#define PAR_ITEM_I32                     PAR_ITEM_NOP
#define PAR_ITEM_F32                     PAR_ITEM_NOP
#define PAR_OBJECT_ITEM_ENABLED_HANDLER  PAR_ITEM_OBJECT_PERSIST_SLOT
#define PAR_OBJECT_ITEM_DISABLED_HANDLER PAR_ITEM_NOP
#include "par_object_item_bind.inc"
#include "par_table.def"
#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32
#include "par_object_item_unbind.inc"
#undef PAR_OBJECT_ITEM_ENABLED_HANDLER
#undef PAR_OBJECT_ITEM_DISABLED_HANDLER
};
#undef PAR_ITEM_NOP
#undef PAR_ITEM_OBJECT_PERSIST_SLOT
#undef PAR_OBJECT_PERSIST_SLOT_ENTRY_SELECT
#undef PAR_OBJECT_PERSIST_SLOT_ENTRY_SELECT_I
#undef PAR_OBJECT_PERSIST_SLOT_ENTRY_SELECT_1
#undef PAR_OBJECT_PERSIST_SLOT_ENTRY_SELECT_0

/**
 * @brief Return the compiled persistent-object count through a runtime-sized value.
 *
 * @details This helper prevents zero-count builds from producing constant-range
 * warnings in generic object-NVM code paths that are still compiled for API
 * completeness.
 *
 * @return Persistent object count.
 */
static uint16_t par_nvm_object_compile_count(void)
{
    return (uint16_t)PAR_OBJECT_PERSISTENT_COMPILE_COUNT;
}

/**
 * @brief Update FNV-1a context with one platform-native scalar image.
 *
 * @param p_hval Pointer to rolling FNV-1a state.
 * @param p_value Pointer to source scalar.
 * @param value_size Scalar width in bytes.
 */
static void par_nvm_object_table_id_hash_update(Fnv32_t * const p_hval,
                                                const void * const p_value,
                                                const uint32_t value_size)
{
    PAR_ASSERT((NULL != p_hval) && (NULL != p_value));
    *p_hval = fnv_32a_buf((void *)p_value, (size_t)value_size, *p_hval);
}


/**
 * @brief Calculate object persistent-schema digest for one stored prefix.
 *
 * @param object_count Persistent object prefix count to hash.
 * @return 32-bit FNV-1a digest.
 */
static uint32_t par_nvm_object_table_id_calc_for_count(const uint16_t object_count)
{
    Fnv32_t hval = FNV1_32A_INIT;
    const uint32_t schema_version = (uint32_t)PAR_CFG_TABLE_ID_SCHEMA_VER;
    const uint32_t object_layout = (uint32_t)PAR_NVM_OBJECT_VERSION;

    PAR_ASSERT(object_count <= (uint16_t)PAR_OBJECT_PERSISTENT_COMPILE_COUNT);

    par_nvm_object_table_id_hash_update(&hval, &schema_version, (uint32_t)sizeof(schema_version));
    par_nvm_object_table_id_hash_update(&hval, &object_layout, (uint32_t)sizeof(object_layout));
    par_nvm_object_table_id_hash_update(&hval, &object_count, (uint32_t)sizeof(object_count));

    for (uint16_t idx = 0U; idx < object_count; idx++)
    {
        const par_num_t par_num = g_par_object_persist_slot_to_par_num[idx];
        const par_cfg_t * const p_cfg = par_get_config(par_num);
        const uint16_t id = (uint16_t)PAR_NVM_OBJECT_CFG_ID_VALUE(par_num);
        const uint8_t type = (uint8_t)p_cfg->type;
        const uint16_t elem_size = p_cfg->value_cfg.object.elem_size;
        const uint16_t min_len = p_cfg->value_cfg.object.range.min_len;
        const uint16_t max_len = p_cfg->value_cfg.object.range.max_len;

        par_nvm_object_table_id_hash_update(&hval, &type, (uint32_t)sizeof(type));
        par_nvm_object_table_id_hash_update(&hval, &id, (uint32_t)sizeof(id));
        par_nvm_object_table_id_hash_update(&hval, &elem_size, (uint32_t)sizeof(elem_size));
        par_nvm_object_table_id_hash_update(&hval, &min_len, (uint32_t)sizeof(min_len));
        par_nvm_object_table_id_hash_update(&hval, &max_len, (uint32_t)sizeof(max_len));
    }

    return (uint32_t)hval;
}

/**
 * @brief Calculate object header CRC.
 *
 * @param p_head Pointer to object header.
 * @return Calculated CRC-16.
 */
static uint16_t par_nvm_object_calc_head_crc(const par_nvm_object_head_t * const p_head)
{
    uint16_t crc = PAR_IF_CRC16_INIT;

    PAR_ASSERT(NULL != p_head);

    crc = par_if_crc16_accumulate(crc, (const uint8_t * const)&p_head->version, (uint32_t)sizeof(p_head->version));
    crc = par_if_crc16_accumulate(crc, (const uint8_t * const)&p_head->obj_nb, (uint32_t)sizeof(p_head->obj_nb));
    crc = par_if_crc16_accumulate(crc, (const uint8_t * const)&p_head->table_id, (uint32_t)sizeof(p_head->table_id));
    crc = par_if_crc16_accumulate(crc, (const uint8_t * const)&p_head->body_size, (uint32_t)sizeof(p_head->body_size));

    return crc;
}

/**
 * @brief Calculate object record CRC.
 *
 * @param p_meta Pointer to record metadata.
 * @param p_payload Pointer to payload bytes, or NULL when len is zero.
 * @return Calculated CRC-16.
 */
static uint16_t par_nvm_object_calc_record_crc(const par_nvm_object_record_meta_t * const p_meta,
                                               const uint8_t * const p_payload)
{
    uint16_t crc = PAR_IF_CRC16_INIT;

    PAR_ASSERT(NULL != p_meta);
    PAR_ASSERT((NULL != p_payload) || (0U == p_meta->len));

    crc = par_if_crc16_accumulate(crc, (const uint8_t * const)&p_meta->id, (uint32_t)sizeof(p_meta->id));
    crc = par_if_crc16_accumulate(crc, (const uint8_t * const)&p_meta->type, (uint32_t)sizeof(p_meta->type));
    crc = par_if_crc16_accumulate(crc, (const uint8_t * const)&p_meta->flags, (uint32_t)sizeof(p_meta->flags));
    crc = par_if_crc16_accumulate(crc, (const uint8_t * const)&p_meta->elem_size, (uint32_t)sizeof(p_meta->elem_size));
    crc = par_if_crc16_accumulate(crc, (const uint8_t * const)&p_meta->capacity, (uint32_t)sizeof(p_meta->capacity));
    crc = par_if_crc16_accumulate(crc, (const uint8_t * const)&p_meta->len, (uint32_t)sizeof(p_meta->len));
    if (p_meta->len > 0U)
    {
        crc = par_if_crc16_accumulate(crc, p_payload, (uint32_t)p_meta->len);
    }

    return crc;
}

/**
 * @brief Calculate object record CRC directly from backend storage.
 *
 * @details This keeps persisted payload bytes out of the live object pool until
 * record integrity has been verified successfully.
 *
 * @param p_store Active storage backend API.
 * @param payload_addr Serialized payload address.
 * @param p_meta Loaded record metadata.
 * @param p_crc Output calculated CRC-16.
 * @return Operation status.
 */
static par_status_t par_nvm_object_calc_record_crc_from_store(const par_store_backend_api_t * const p_store,
                                                              const uint32_t payload_addr,
                                                              const par_nvm_object_record_meta_t * const p_meta,
                                                              uint16_t * const p_crc)
{
    uint8_t payload_buf[PAR_NVM_OBJECT_VERIFY_CHUNK_SIZE] = { 0U };
    uint32_t offset = 0U;
    uint16_t crc = PAR_IF_CRC16_INIT;
    par_status_t status = ePAR_OK;

    PAR_ASSERT((NULL != p_store) && (NULL != p_meta) && (NULL != p_crc));
    if ((NULL == p_store) || (NULL == p_meta) || (NULL == p_crc))
    {
        return ePAR_ERROR_PARAM;
    }

    crc = par_if_crc16_accumulate(crc, (const uint8_t * const)&p_meta->id, (uint32_t)sizeof(p_meta->id));
    crc = par_if_crc16_accumulate(crc, (const uint8_t * const)&p_meta->type, (uint32_t)sizeof(p_meta->type));
    crc = par_if_crc16_accumulate(crc, (const uint8_t * const)&p_meta->flags, (uint32_t)sizeof(p_meta->flags));
    crc = par_if_crc16_accumulate(crc, (const uint8_t * const)&p_meta->elem_size, (uint32_t)sizeof(p_meta->elem_size));
    crc = par_if_crc16_accumulate(crc, (const uint8_t * const)&p_meta->capacity, (uint32_t)sizeof(p_meta->capacity));
    crc = par_if_crc16_accumulate(crc, (const uint8_t * const)&p_meta->len, (uint32_t)sizeof(p_meta->len));

    while (offset < (uint32_t)p_meta->len)
    {
        uint32_t chunk = (uint32_t)p_meta->len - offset;

        if (chunk > (uint32_t)sizeof(payload_buf))
        {
            chunk = (uint32_t)sizeof(payload_buf);
        }

        status = p_store->read(payload_addr + offset, chunk, payload_buf);
        if (ePAR_OK != status)
        {
            PAR_ERR_PRINT("PAR_NVM: object CRC payload read failed, addr=0x%08lX err=%u",
                          (unsigned long)(payload_addr + offset),
                          (unsigned)status);
            return ePAR_ERROR_NVM;
        }

        crc = par_if_crc16_accumulate(crc, payload_buf, chunk);
        offset += chunk;
    }

    *p_crc = crc;
    return ePAR_OK;
}

/**
 * @brief Return serialized record size for one object parameter.
 *
 * @param par_num Parameter number.
 * @return Serialized record size in bytes.
 */
static uint32_t par_nvm_object_record_size_from_par_num(const par_num_t par_num)
{
    const par_cfg_t * const p_cfg = par_get_config(par_num);

    PAR_ASSERT(NULL != p_cfg);
    return (uint32_t)PAR_NVM_OBJECT_RECORD_HEAD_SIZE + (uint32_t)p_cfg->value_cfg.object.range.max_len;
}

/**
 * @brief Return object record address from persistent object index.
 *
 * @param base_addr Object block base address.
 * @param persist_idx Persistent object index.
 * @return Record address.
 */
static uint32_t par_nvm_object_addr_from_persist_idx(const uint32_t base_addr,
                                                     const uint16_t persist_idx)
{
    uint32_t addr = base_addr + (uint32_t)PAR_NVM_OBJECT_HEAD_SIZE;

    for (uint16_t idx = 0U; idx < persist_idx; idx++)
    {
        addr += par_nvm_object_record_size_from_par_num(g_par_object_persist_slot_to_par_num[idx]);
    }

    return addr;
}

/**
 * @brief Calculate serialized object-record body size for a stored prefix.
 *
 * @param object_count Number of persistent object records in the prefix.
 * @return Body size in bytes.
 */
static uint32_t par_nvm_object_body_size_for_count(const uint16_t object_count)
{
    uint32_t size = 0U;

    PAR_ASSERT(object_count <= (uint16_t)PAR_OBJECT_PERSISTENT_COMPILE_COUNT);

    for (uint16_t idx = 0U; idx < object_count; idx++)
    {
        size += par_nvm_object_record_size_from_par_num(g_par_object_persist_slot_to_par_num[idx]);
    }

    return size;
}

/**
 * @brief Calculate serialized body size for the current object schema.
 * @return Body size in bytes.
 */
static uint32_t par_nvm_object_body_size(void)
{
    return par_nvm_object_body_size_for_count((uint16_t)PAR_OBJECT_PERSISTENT_COMPILE_COUNT);
}

/**
 * @brief Resolve one object persistent index by parameter number.
 *
 * @param par_num Parameter number.
 * @param p_persist_idx Output persistent object index.
 * @return Operation status.
 */
static par_status_t par_nvm_object_get_persist_idx(const par_num_t par_num,
                                                   uint16_t * const p_persist_idx)
{
    const uint16_t object_count = par_nvm_object_compile_count();
    const par_cfg_t *p_cfg = NULL;

    if ((NULL == p_persist_idx) || (par_num >= ePAR_NUM_OF))
    {
        return ePAR_ERROR;
    }

    p_cfg = par_get_config(par_num);
    if ((NULL == p_cfg) || (false == p_cfg->persistent) ||
        (false == par_object_type_is_object(p_cfg->type)) ||
        (p_cfg->persist_idx >= object_count))
    {
        return ePAR_ERROR;
    }

    if (g_par_object_persist_slot_to_par_num[p_cfg->persist_idx] != par_num)
    {
        return ePAR_ERROR;
    }

    *p_persist_idx = p_cfg->persist_idx;
    return ePAR_OK;
}

/**
 * @brief Pack object header to a serialized byte buffer.
 *
 * @param p_head Pointer to source header.
 * @param p_buf Output serialized header bytes.
 */
static void par_nvm_object_pack_header(const par_nvm_object_head_t * const p_head,
                                       uint8_t * const p_buf)
{
    PAR_ASSERT((NULL != p_head) && (NULL != p_buf));

    memcpy(&p_buf[PAR_NVM_OBJECT_HEAD_SIGN_OFFSET], &p_head->sign, sizeof(p_head->sign));
    memcpy(&p_buf[PAR_NVM_OBJECT_HEAD_VERSION_OFFSET], &p_head->version, sizeof(p_head->version));
    memcpy(&p_buf[PAR_NVM_OBJECT_HEAD_OBJ_NB_OFFSET], &p_head->obj_nb, sizeof(p_head->obj_nb));
    memcpy(&p_buf[PAR_NVM_OBJECT_HEAD_TABLE_ID_OFFSET], &p_head->table_id, sizeof(p_head->table_id));
    memcpy(&p_buf[PAR_NVM_OBJECT_HEAD_BODY_SIZE_OFFSET], &p_head->body_size, sizeof(p_head->body_size));
    memcpy(&p_buf[PAR_NVM_OBJECT_HEAD_CRC_OFFSET], &p_head->crc, sizeof(p_head->crc));
}

/**
 * @brief Unpack object header from serialized bytes.
 *
 * @param p_buf Input serialized header bytes.
 * @param p_head Output header.
 */
static void par_nvm_object_unpack_header(const uint8_t * const p_buf,
                                         par_nvm_object_head_t * const p_head)
{
    PAR_ASSERT((NULL != p_buf) && (NULL != p_head));

    memcpy(&p_head->sign, &p_buf[PAR_NVM_OBJECT_HEAD_SIGN_OFFSET], sizeof(p_head->sign));
    memcpy(&p_head->version, &p_buf[PAR_NVM_OBJECT_HEAD_VERSION_OFFSET], sizeof(p_head->version));
    memcpy(&p_head->obj_nb, &p_buf[PAR_NVM_OBJECT_HEAD_OBJ_NB_OFFSET], sizeof(p_head->obj_nb));
    memcpy(&p_head->table_id, &p_buf[PAR_NVM_OBJECT_HEAD_TABLE_ID_OFFSET], sizeof(p_head->table_id));
    memcpy(&p_head->body_size, &p_buf[PAR_NVM_OBJECT_HEAD_BODY_SIZE_OFFSET], sizeof(p_head->body_size));
    memcpy(&p_head->crc, &p_buf[PAR_NVM_OBJECT_HEAD_CRC_OFFSET], sizeof(p_head->crc));
}

/**
 * @brief Pack object record metadata to serialized bytes.
 *
 * @param p_meta Pointer to source metadata.
 * @param p_buf Output serialized metadata bytes.
 */
static void par_nvm_object_pack_record_meta(const par_nvm_object_record_meta_t * const p_meta,
                                            uint8_t * const p_buf)
{
    PAR_ASSERT((NULL != p_meta) && (NULL != p_buf));

    memcpy(&p_buf[PAR_NVM_OBJECT_RECORD_ID_OFFSET], &p_meta->id, sizeof(p_meta->id));
    memcpy(&p_buf[PAR_NVM_OBJECT_RECORD_TYPE_OFFSET], &p_meta->type, sizeof(p_meta->type));
    memcpy(&p_buf[PAR_NVM_OBJECT_RECORD_FLAGS_OFFSET], &p_meta->flags, sizeof(p_meta->flags));
    memcpy(&p_buf[PAR_NVM_OBJECT_RECORD_ELEM_SIZE_OFFSET], &p_meta->elem_size, sizeof(p_meta->elem_size));
    memcpy(&p_buf[PAR_NVM_OBJECT_RECORD_CAPACITY_OFFSET], &p_meta->capacity, sizeof(p_meta->capacity));
    memcpy(&p_buf[PAR_NVM_OBJECT_RECORD_LEN_OFFSET], &p_meta->len, sizeof(p_meta->len));
    memcpy(&p_buf[PAR_NVM_OBJECT_RECORD_CRC_OFFSET], &p_meta->crc, sizeof(p_meta->crc));
}

/**
 * @brief Unpack object record metadata from serialized bytes.
 *
 * @param p_buf Input serialized metadata bytes.
 * @param p_meta Output metadata.
 */
static void par_nvm_object_unpack_record_meta(const uint8_t * const p_buf,
                                              par_nvm_object_record_meta_t * const p_meta)
{
    PAR_ASSERT((NULL != p_buf) && (NULL != p_meta));

    memcpy(&p_meta->id, &p_buf[PAR_NVM_OBJECT_RECORD_ID_OFFSET], sizeof(p_meta->id));
    memcpy(&p_meta->type, &p_buf[PAR_NVM_OBJECT_RECORD_TYPE_OFFSET], sizeof(p_meta->type));
    memcpy(&p_meta->flags, &p_buf[PAR_NVM_OBJECT_RECORD_FLAGS_OFFSET], sizeof(p_meta->flags));
    memcpy(&p_meta->elem_size, &p_buf[PAR_NVM_OBJECT_RECORD_ELEM_SIZE_OFFSET], sizeof(p_meta->elem_size));
    memcpy(&p_meta->capacity, &p_buf[PAR_NVM_OBJECT_RECORD_CAPACITY_OFFSET], sizeof(p_meta->capacity));
    memcpy(&p_meta->len, &p_buf[PAR_NVM_OBJECT_RECORD_LEN_OFFSET], sizeof(p_meta->len));
    memcpy(&p_meta->crc, &p_buf[PAR_NVM_OBJECT_RECORD_CRC_OFFSET], sizeof(p_meta->crc));
}

/**
 * @brief Read object block header.
 *
 * @param p_store Active storage backend API.
 * @param base_addr Object block base address.
 * @param p_head Output header.
 * @return Operation status.
 */
static par_status_t par_nvm_object_read_header(const par_store_backend_api_t * const p_store,
                                               const uint32_t base_addr,
                                               par_nvm_object_head_t * const p_head)
{
    uint8_t head_buf[PAR_NVM_OBJECT_HEAD_SIZE] = { 0U };
    par_status_t status = ePAR_OK;

    PAR_ASSERT((NULL != p_store) && (NULL != p_head));

    status = p_store->read(base_addr, (uint32_t)PAR_NVM_OBJECT_HEAD_SIZE, head_buf);
    if (ePAR_OK != status)
    {
        PAR_ERR_PRINT("PAR_NVM: object header read failed, err=%u", (unsigned)status);
        return ePAR_ERROR_NVM;
    }

    par_nvm_object_unpack_header(head_buf, p_head);
    return ePAR_OK;
}

#if (1 == PAR_CFG_NVM_OBJECT_WRITE_VERIFY_EN)
/**
 * @brief Verify one just-written object block header.
 *
 * @param p_store Active storage backend API.
 * @param base_addr Object block base address.
 * @param p_expected_buf Expected serialized header bytes.
 * @return Operation status.
 */
static par_status_t par_nvm_object_verify_header_readback(const par_store_backend_api_t * const p_store,
                                                          const uint32_t base_addr,
                                                          const uint8_t * const p_expected_buf)
{
    uint8_t actual_head_buf[PAR_NVM_OBJECT_HEAD_SIZE] = { 0U };
    par_status_t status = ePAR_OK;

    PAR_ASSERT((NULL != p_store) && (NULL != p_expected_buf));

    status = p_store->read(base_addr, (uint32_t)PAR_NVM_OBJECT_HEAD_SIZE, actual_head_buf);
    if (ePAR_OK != status)
    {
        PAR_ERR_PRINT("PAR_NVM: object header verify read failed, addr=0x%08lX err=%u",
                      (unsigned long)base_addr,
                      (unsigned)status);
        return ePAR_ERROR_NVM;
    }
    if (0 != memcmp(actual_head_buf, p_expected_buf, (size_t)PAR_NVM_OBJECT_HEAD_SIZE))
    {
        PAR_WARN_PRINT("PAR_NVM: object header verify mismatch, addr=0x%08lX",
                       (unsigned long)base_addr);
        return ePAR_ERROR_NVM;
    }

    return ePAR_OK;
}
#endif /* (1 == PAR_CFG_NVM_OBJECT_WRITE_VERIFY_EN) */

/**
 * @brief Write object block header.
 *
 * @param p_store Active storage backend API.
 * @param base_addr Object block base address.
 * @return Operation status.
 */
static par_status_t par_nvm_object_write_header(const par_store_backend_api_t * const p_store,
                                                const uint32_t base_addr)
{
    const uint16_t object_count = par_nvm_object_compile_count();
    par_nvm_object_head_t head = { 0 };
    uint8_t head_buf[PAR_NVM_OBJECT_HEAD_SIZE] = { 0U };
    par_status_t status = ePAR_OK;

    PAR_ASSERT(NULL != p_store);

    head.sign = PAR_NVM_OBJECT_SIGN;
    head.version = PAR_NVM_OBJECT_VERSION;
    head.obj_nb = object_count;
    head.table_id = par_nvm_object_table_id_calc_for_count(object_count);
    head.body_size = par_nvm_object_body_size();
    head.crc = par_nvm_object_calc_head_crc(&head);
    par_nvm_object_pack_header(&head, head_buf);

    status = p_store->write(base_addr, (uint32_t)PAR_NVM_OBJECT_HEAD_SIZE, head_buf);
    if (ePAR_OK != status)
    {
        PAR_ERR_PRINT("PAR_NVM: object header write failed, err=%u", (unsigned)status);
        return ePAR_ERROR_NVM;
    }

    status = p_store->sync();
    if (ePAR_OK != status)
    {
        PAR_ERR_PRINT("PAR_NVM: object header sync failed, err=%u", (unsigned)status);
        return ePAR_ERROR_NVM;
    }

#if (1 == PAR_CFG_NVM_OBJECT_WRITE_VERIFY_EN)
    status = par_nvm_object_verify_header_readback(p_store, base_addr, head_buf);
    if (ePAR_OK != status)
    {
        return status;
    }
#endif /* (1 == PAR_CFG_NVM_OBJECT_WRITE_VERIFY_EN) */

    return ePAR_OK;
}

/**
 * @brief Validate object block header fields.
 *
 * @param p_head Header to validate.
 * @return Operation status.
 */
static par_status_t par_nvm_object_validate_header(const par_nvm_object_head_t * const p_head)
{
    uint16_t crc_calc = 0U;

    PAR_ASSERT(NULL != p_head);
    crc_calc = par_nvm_object_calc_head_crc(p_head);

    if (PAR_NVM_OBJECT_SIGN != p_head->sign)
    {
        PAR_WARN_PRINT("PAR_NVM: object header signature corrupted");
        return ePAR_ERROR;
    }
    if (PAR_NVM_OBJECT_VERSION != p_head->version)
    {
        PAR_WARN_PRINT("PAR_NVM: object header version mismatch, stored=%u live=%u",
                       (unsigned)p_head->version,
                       (unsigned)PAR_NVM_OBJECT_VERSION);
        return ePAR_ERROR_TABLE_ID;
    }
    if (crc_calc != p_head->crc)
    {
        PAR_WARN_PRINT("PAR_NVM: object header CRC corrupted");
        return ePAR_ERROR_CRC;
    }
    if (p_head->obj_nb > (uint16_t)PAR_OBJECT_PERSISTENT_COMPILE_COUNT)
    {
        PAR_WARN_PRINT("PAR_NVM: object count overflow, stored=%u live=%u",
                       (unsigned)p_head->obj_nb,
                       (unsigned)PAR_OBJECT_PERSISTENT_COMPILE_COUNT);
        return ePAR_ERROR_TABLE_ID;
    }
    if (p_head->body_size != par_nvm_object_body_size_for_count(p_head->obj_nb))
    {
        PAR_WARN_PRINT("PAR_NVM: object body-size mismatch, stored=%lu expected=%lu",
                       (unsigned long)p_head->body_size,
                       (unsigned long)par_nvm_object_body_size_for_count(p_head->obj_nb));
        return ePAR_ERROR_TABLE_ID;
    }

    return ePAR_OK;
}

/**
 * @brief Validate one loaded object record metadata against live schema.
 *
 * @param par_num Parameter number.
 * @param p_meta Loaded record metadata.
 * @return Operation status.
 */
static par_status_t par_nvm_object_validate_record_meta(const par_num_t par_num,
                                                        const par_nvm_object_record_meta_t * const p_meta)
{
    const par_cfg_t * const p_cfg = par_get_config(par_num);
    const uint16_t expected_id = (uint16_t)PAR_NVM_OBJECT_CFG_ID_VALUE(par_num);

    PAR_ASSERT((NULL != p_cfg) && (NULL != p_meta));

    if (p_meta->id != expected_id)
    {
        PAR_WARN_PRINT("PAR_NVM: object id mismatch, par_num=%u stored=%u live=%u",
                       (unsigned)par_num,
                       (unsigned)p_meta->id,
                       (unsigned)expected_id);
        return ePAR_ERROR_TABLE_ID;
    }
    if (p_meta->type != (uint8_t)p_cfg->type)
    {
        PAR_WARN_PRINT("PAR_NVM: object type mismatch, par_num=%u stored=%u live=%u",
                       (unsigned)par_num,
                       (unsigned)p_meta->type,
                       (unsigned)p_cfg->type);
        return ePAR_ERROR_TABLE_ID;
    }
    if (PAR_NVM_OBJECT_RECORD_FLAGS_NONE != p_meta->flags)
    {
        PAR_WARN_PRINT("PAR_NVM: object flags unsupported, par_num=%u flags=0x%02X",
                       (unsigned)par_num,
                       (unsigned)p_meta->flags);
        return ePAR_ERROR_TABLE_ID;
    }
    if (p_meta->elem_size != p_cfg->value_cfg.object.elem_size)
    {
        PAR_WARN_PRINT("PAR_NVM: object element size mismatch, par_num=%u stored=%u live=%u",
                       (unsigned)par_num,
                       (unsigned)p_meta->elem_size,
                       (unsigned)p_cfg->value_cfg.object.elem_size);
        return ePAR_ERROR_TABLE_ID;
    }
    if (p_meta->capacity != p_cfg->value_cfg.object.range.max_len)
    {
        PAR_WARN_PRINT("PAR_NVM: object capacity mismatch, par_num=%u stored=%u live=%u",
                       (unsigned)par_num,
                       (unsigned)p_meta->capacity,
                       (unsigned)p_cfg->value_cfg.object.range.max_len);
        return ePAR_ERROR_TABLE_ID;
    }
    if (false == par_object_len_is_valid(&p_cfg->value_cfg.object, p_meta->len))
    {
        PAR_WARN_PRINT("PAR_NVM: object length invalid, par_num=%u len=%u min=%u max=%u elem=%u",
                       (unsigned)par_num,
                       (unsigned)p_meta->len,
                       (unsigned)p_cfg->value_cfg.object.range.min_len,
                       (unsigned)p_cfg->value_cfg.object.range.max_len,
                       (unsigned)p_cfg->value_cfg.object.elem_size);
        return ePAR_ERROR_VALUE;
    }

    return ePAR_OK;
}

/**
 * @brief Read, validate, and restore one persistent object record.
 *
 * @param p_store Active storage backend API.
 * @param addr Record address.
 * @param par_num Parameter number.
 * @return Operation status.
 */
static par_status_t par_nvm_object_read_record(const par_store_backend_api_t * const p_store,
                                               const uint32_t addr,
                                               const par_num_t par_num)
{
    par_nvm_object_record_meta_t meta = { 0 };
    uint8_t meta_buf[PAR_NVM_OBJECT_RECORD_HEAD_SIZE] = { 0U };
    uint8_t *p_payload = NULL;
    uint16_t capacity = 0U;
    uint16_t crc_calc = 0U;
    bool locked = false;
    par_status_t status = ePAR_OK;

    PAR_ASSERT(NULL != p_store);

    status = p_store->read(addr, (uint32_t)PAR_NVM_OBJECT_RECORD_HEAD_SIZE, meta_buf);
    if (ePAR_OK != status)
    {
        PAR_ERR_PRINT("PAR_NVM: object record header read failed, par_num=%u addr=0x%08lX err=%u",
                      (unsigned)par_num,
                      (unsigned long)addr,
                      (unsigned)status);
        return ePAR_ERROR_NVM;
    }

    par_nvm_object_unpack_record_meta(meta_buf, &meta);
    status = par_nvm_object_validate_record_meta(par_num, &meta);
    if (ePAR_OK != status)
    {
        return status;
    }

    status = par_nvm_object_calc_record_crc_from_store(p_store,
                                                       addr + (uint32_t)PAR_NVM_OBJECT_RECORD_HEAD_SIZE,
                                                       &meta,
                                                       &crc_calc);
    if (ePAR_OK != status)
    {
        return status;
    }
    if (crc_calc != meta.crc)
    {
        PAR_WARN_PRINT("PAR_NVM: object record CRC corrupted, par_num=%u stored=0x%04X calc=0x%04X",
                       (unsigned)par_num,
                       (unsigned)meta.crc,
                       (unsigned)crc_calc);
        return ePAR_ERROR_CRC;
    }

    status = par_acquire_mutex(par_num);
    if (ePAR_OK != status)
    {
        return ePAR_ERROR_MUTEX;
    }
    locked = true;

    status = par_obj_nvm_restore_window(par_num, &p_payload, &capacity);
    if (ePAR_OK != status)
    {
        goto out;
    }
    if (meta.len > capacity)
    {
        status = ePAR_ERROR_VALUE;
        goto out;
    }

    if (meta.len > 0U)
    {
        status = p_store->read(addr + (uint32_t)PAR_NVM_OBJECT_RECORD_HEAD_SIZE, (uint32_t)meta.len, p_payload);
        if (ePAR_OK != status)
        {
            PAR_ERR_PRINT("PAR_NVM: object payload read failed, par_num=%u addr=0x%08lX err=%u",
                          (unsigned)par_num,
                          (unsigned long)(addr + (uint32_t)PAR_NVM_OBJECT_RECORD_HEAD_SIZE),
                          (unsigned)status);
            status = ePAR_ERROR_NVM;
            goto out;
        }
    }

    status = par_obj_nvm_commit_restore(par_num, meta.len);

out:
    if (true == locked)
    {
        par_release_mutex(par_num);
    }
    return status;
}

#if (1 == PAR_CFG_NVM_OBJECT_WRITE_VERIFY_EN)
/**
 * @brief Verify one just-written object record without mutating live RAM.
 *
 * @param p_store Active storage backend API.
 * @param addr Record address.
 * @param p_expected_meta Expected serialized record metadata.
 * @param p_expected_payload Expected payload bytes, or NULL when len is zero.
 * @return Operation status.
 */
static par_status_t par_nvm_object_verify_record_readback(const par_store_backend_api_t * const p_store,
                                                          const uint32_t addr,
                                                          const par_nvm_object_record_meta_t * const p_expected_meta,
                                                          const uint8_t * const p_expected_payload)
{
    uint8_t expected_meta_buf[PAR_NVM_OBJECT_RECORD_HEAD_SIZE] = { 0U };
    uint8_t actual_meta_buf[PAR_NVM_OBJECT_RECORD_HEAD_SIZE] = { 0U };
    uint8_t payload_buf[PAR_NVM_OBJECT_VERIFY_CHUNK_SIZE] = { 0U };
    uint32_t offset = 0U;
    par_status_t status = ePAR_OK;

    PAR_ASSERT((NULL != p_store) && (NULL != p_expected_meta));
    PAR_ASSERT((NULL != p_expected_payload) || (0U == p_expected_meta->len));

    par_nvm_object_pack_record_meta(p_expected_meta, expected_meta_buf);

    status = p_store->read(addr, (uint32_t)PAR_NVM_OBJECT_RECORD_HEAD_SIZE, actual_meta_buf);
    if (ePAR_OK != status)
    {
        PAR_ERR_PRINT("PAR_NVM: object verify header read failed, addr=0x%08lX err=%u",
                      (unsigned long)addr,
                      (unsigned)status);
        return ePAR_ERROR_NVM;
    }
    if (0 != memcmp(actual_meta_buf, expected_meta_buf, (size_t)PAR_NVM_OBJECT_RECORD_HEAD_SIZE))
    {
        PAR_WARN_PRINT("PAR_NVM: object verify header mismatch, addr=0x%08lX", (unsigned long)addr);
        return ePAR_ERROR_NVM;
    }

    while (offset < (uint32_t)p_expected_meta->len)
    {
        uint32_t chunk = (uint32_t)p_expected_meta->len - offset;

        if (chunk > (uint32_t)sizeof(payload_buf))
        {
            chunk = (uint32_t)sizeof(payload_buf);
        }

        status = p_store->read(addr + (uint32_t)PAR_NVM_OBJECT_RECORD_HEAD_SIZE + offset, chunk, payload_buf);
        if (ePAR_OK != status)
        {
            PAR_ERR_PRINT("PAR_NVM: object verify payload read failed, addr=0x%08lX err=%u",
                          (unsigned long)(addr + (uint32_t)PAR_NVM_OBJECT_RECORD_HEAD_SIZE + offset),
                          (unsigned)status);
            return ePAR_ERROR_NVM;
        }
        if (0 != memcmp(payload_buf, &p_expected_payload[offset], (size_t)chunk))
        {
            PAR_WARN_PRINT("PAR_NVM: object verify payload mismatch, addr=0x%08lX offset=%lu",
                           (unsigned long)addr,
                           (unsigned long)offset);
            return ePAR_ERROR_NVM;
        }

        offset += chunk;
    }

    return ePAR_OK;
}
#endif /* (1 == PAR_CFG_NVM_OBJECT_WRITE_VERIFY_EN) */

/**
 * @brief Set all persistent object parameters to their defaults.
 * @return Operation status.
 */
static par_status_t par_nvm_object_set_persistent_defaults(void)
{
    const uint16_t object_count = par_nvm_object_compile_count();
    par_status_t status = ePAR_OK;

    for (uint16_t idx = 0U; idx < object_count; idx++)
    {
        status |= par_set_to_default(g_par_object_persist_slot_to_par_num[idx]);
    }

    return status;
}

/**
 * @brief Validate that the active object store can address the object block.
 *
 * @details The object backend abstraction does not expose a reserved-region
 * size. This function uses a one-byte read from the last byte required by the
 * current object block as a non-destructive capacity probe. The active object
 * store may alias the scalar backend or point to a dedicated object backend,
 * depending on the selected object store adapter.
 *
 * @param object_block_addr Object persistence block base address.
 * @return Operation status.
 */
par_status_t par_nvm_object_validate_storage_capacity(const uint32_t object_block_addr)
{
    const par_store_backend_api_t * const p_store = par_nvm_object_store_get_api();
    const uint32_t object_block_size = par_nvm_object_get_block_size();
    uint32_t object_last_offset = 0U;
    uint8_t capacity_probe = 0U;
    uint32_t probe_addr = 0U;
    par_status_t status = ePAR_OK;

    if (0U == object_block_size)
    {
        return ePAR_OK;
    }

    object_last_offset = object_block_size - 1U;

    PAR_ASSERT((NULL != p_store) && (NULL != p_store->read));
    if ((NULL == p_store) || (NULL == p_store->read))
    {
        return ePAR_ERROR_INIT;
    }
    if (((0U == object_block_addr) && (false == par_nvm_object_block_addr_zero_is_valid())) ||
        (object_block_addr > (UINT32_MAX - object_last_offset)))
    {
        PAR_ERR_PRINT("PAR_NVM: object storage capacity check address overflow, base=0x%08lX size=%lu",
                      (unsigned long)object_block_addr,
                      (unsigned long)object_block_size);
        return ePAR_ERROR_NVM;
    }

    probe_addr = object_block_addr + object_block_size - 1U;
    status = p_store->read(probe_addr, 1U, &capacity_probe);
    if (ePAR_OK != status)
    {
        PAR_ERR_PRINT("PAR_NVM: object storage capacity check failed, base=0x%08lX size=%lu last_addr=0x%08lX err=%u",
                      (unsigned long)object_block_addr,
                      (unsigned long)object_block_size,
                      (unsigned long)probe_addr,
                      (unsigned)status);
        return ePAR_ERROR_NVM;
    }

    return ePAR_OK;
}

/**
 * @brief Return the number of compiled persistent object records.
 *
 * @return Persistent object count.
 */
uint16_t par_nvm_object_get_count(void)
{
    return par_nvm_object_compile_count();
}

/**
 * @brief Return serialized object persistence block size.
 *
 * @return Required object block size in bytes, or zero when no object
 * records are configured.
 */
uint32_t par_nvm_object_get_block_size(void)
{
    const uint16_t object_count = par_nvm_object_compile_count();

    if (0U == object_count)
    {
        return 0U;
    }

    return (uint32_t)PAR_NVM_OBJECT_HEAD_SIZE + par_nvm_object_body_size();
}

/**
 * @brief Return one persistent object record address.
 *
 * @param base_addr Object persistence block base address.
 * @param par_num Parameter number.
 * @return Record address, or zero when par_num is not a persistent object.
 */
uint32_t par_nvm_object_get_addr(const uint32_t base_addr, const par_num_t par_num)
{
    uint16_t persist_idx = 0U;

    if (ePAR_OK != par_nvm_object_get_persist_idx(par_num, &persist_idx))
    {
        return 0U;
    }

    return par_nvm_object_addr_from_persist_idx(base_addr, persist_idx);
}

/**
 * @brief Persist one object parameter with optional local mutex acquisition.
 *
 * @details The exported payload pointer references the live object pool.
 * Payload write, metadata write, backend sync, and optional readback
 * verification must run while the parameter mutex is held so the serialized
 * record is a stable snapshot of the current object value.
 *
 * @param p_store Active storage backend API.
 * @param base_addr Object persistence block base address.
 * @param par_num Parameter number.
 * @param nvm_sync Request backend sync after write when true.
 * @param take_mutex Acquire and release the parameter mutex when true.
 * @return Operation status.
 */
static par_status_t par_nvm_object_write_core(const par_store_backend_api_t * const p_store,
                                              const uint32_t base_addr,
                                              const par_num_t par_num,
                                              const bool nvm_sync,
                                              const bool take_mutex)
{
    const par_cfg_t *p_cfg = NULL;
    const uint8_t *p_payload = NULL;
    uint16_t len = 0U;
    uint16_t capacity = 0U;
    uint16_t persist_idx = 0U;
    uint8_t meta_buf[PAR_NVM_OBJECT_RECORD_HEAD_SIZE] = { 0U };
    par_nvm_object_record_meta_t meta = { 0 };
    uint32_t addr = 0U;
    bool locked = false;
    par_status_t status = ePAR_OK;

    if (NULL == p_store)
    {
        return ePAR_ERROR_PARAM;
    }
    if (par_num >= ePAR_NUM_OF)
    {
        return ePAR_ERROR_PAR_NUM;
    }

    p_cfg = par_get_config(par_num);
    if ((NULL == p_cfg) || (false == p_cfg->persistent) ||
        (false == par_object_type_is_object(p_cfg->type)))
    {
        return ePAR_ERROR;
    }

    status = par_nvm_object_get_persist_idx(par_num, &persist_idx);
    if (ePAR_OK != status)
    {
        return status;
    }

    if (true == take_mutex)
    {
        status = par_acquire_mutex(par_num);
        if (ePAR_OK != status)
        {
            return ePAR_ERROR_MUTEX;
        }
        locked = true;
    }

    /* Keep NVM I/O inside the mutex because p_payload aliases the live
     * object pool and readback verification must compare against the same
     * snapshot.
     */
    status = par_obj_nvm_export(par_num, &p_payload, &len, &capacity);
    if (ePAR_OK != status)
    {
        goto out;
    }

    meta.id = (uint16_t)PAR_NVM_OBJECT_CFG_ID_VALUE(par_num);
    meta.type = (uint8_t)p_cfg->type;
    meta.flags = PAR_NVM_OBJECT_RECORD_FLAGS_NONE;
    meta.elem_size = p_cfg->value_cfg.object.elem_size;
    meta.capacity = capacity;
    meta.len = len;
    meta.crc = par_nvm_object_calc_record_crc(&meta, p_payload);
    par_nvm_object_pack_record_meta(&meta, meta_buf);

    addr = par_nvm_object_addr_from_persist_idx(base_addr, persist_idx);
    if (len > 0U)
    {
        status = p_store->write(addr + (uint32_t)PAR_NVM_OBJECT_RECORD_HEAD_SIZE, (uint32_t)len, p_payload);
        if (ePAR_OK != status)
        {
            PAR_ERR_PRINT("PAR_NVM: object payload write failed, par_num=%u addr=0x%08lX err=%u",
                          (unsigned)par_num,
                          (unsigned long)(addr + (uint32_t)PAR_NVM_OBJECT_RECORD_HEAD_SIZE),
                          (unsigned)status);
            status = ePAR_ERROR_NVM;
            goto out;
        }
    }

    status = p_store->write(addr, (uint32_t)PAR_NVM_OBJECT_RECORD_HEAD_SIZE, meta_buf);
    if (ePAR_OK != status)
    {
        PAR_ERR_PRINT("PAR_NVM: object record header write failed, par_num=%u addr=0x%08lX err=%u",
                      (unsigned)par_num,
                      (unsigned long)addr,
                      (unsigned)status);
        status = ePAR_ERROR_NVM;
        goto out;
    }

#if (1 == PAR_CFG_NVM_OBJECT_WRITE_VERIFY_EN)
    (void)nvm_sync;
    status = p_store->sync();
    if (ePAR_OK != status)
    {
        PAR_ERR_PRINT("PAR_NVM: object record sync failed, par_num=%u err=%u", (unsigned)par_num, (unsigned)status);
        status = ePAR_ERROR_NVM;
        goto out;
    }

    status = par_nvm_object_verify_record_readback(p_store, addr, &meta, p_payload);
#else
    if (true == nvm_sync)
    {
        status = p_store->sync();
        if (ePAR_OK != status)
        {
            PAR_ERR_PRINT("PAR_NVM: object record sync failed, par_num=%u err=%u", (unsigned)par_num, (unsigned)status);
            status = ePAR_ERROR_NVM;
            goto out;
        }
    }
#endif /* (1 == PAR_CFG_NVM_OBJECT_WRITE_VERIFY_EN) */

out:
    if (true == locked)
    {
        par_release_mutex(par_num);
    }
    return status;
}

/**
 * @brief Persist one object parameter to the object persistence block.
 *
 * @details Object persistence integrations must budget backend write and verification
 * latency as part of the parameter-set critical path.
 *
 * @param p_store Active storage backend API.
 * @param base_addr Object persistence block base address.
 * @param par_num Parameter number.
 * @param nvm_sync Request backend sync after write when true.
 * @return Operation status.
 */
par_status_t par_nvm_object_write(const par_store_backend_api_t * const p_store,
                                  const uint32_t base_addr,
                                  const par_num_t par_num,
                                  const bool nvm_sync)
{
    return par_nvm_object_write_core(p_store, base_addr, par_num, nvm_sync, true);
}

/**
 * @brief Persist one object parameter while the caller already holds the mutex.
 *
 * @param p_store Active storage backend API.
 * @param base_addr Object persistence block base address.
 * @param par_num Parameter number.
 * @param nvm_sync Request backend sync after write when true.
 * @return Operation status.
 */
par_status_t par_nvm_object_write_locked(const par_store_backend_api_t * const p_store,
                                         const uint32_t base_addr,
                                         const par_num_t par_num,
                                         const bool nvm_sync)
{
    return par_nvm_object_write_core(p_store, base_addr, par_num, nvm_sync, false);
}

/**
 * @brief Store every persistent object parameter to the object persistence block.
 *
 * @details The object block signature is erased first so that an interrupted
 * bulk rewrite cannot be accepted as valid on the next boot. The signature
 * erase is synchronized before records are written so delayed-backend writes
 * cannot leave an old valid header in front of partially rewritten records.
 * Each object record is then written without per-record sync, and the valid
 * object header is committed only after all records have been serialized
 * successfully.
 *
 * @param p_store Active storage backend API.
 * @param base_addr Object persistence block base address.
 * @return Operation status.
 */
par_status_t par_nvm_object_write_all(const par_store_backend_api_t * const p_store,
                                      const uint32_t base_addr)
{
    const uint16_t object_count = par_nvm_object_compile_count();
    par_status_t status = ePAR_OK;

    if (NULL == p_store)
    {
        return ePAR_ERROR_PARAM;
    }

    if (0U == object_count)
    {
        return ePAR_OK;
    }

    status = p_store->erase(base_addr, (uint32_t)sizeof(uint32_t));
    if (ePAR_OK != status)
    {
        PAR_ERR_PRINT("PAR_NVM: object signature erase failed, err=%u", (unsigned)status);
        return ePAR_ERROR_NVM;
    }

    status = p_store->sync();
    if (ePAR_OK != status)
    {
        PAR_ERR_PRINT("PAR_NVM: object signature erase sync failed, err=%u", (unsigned)status);
        return ePAR_ERROR_NVM;
    }

    for (uint16_t idx = 0U; idx < object_count; idx++)
    {
        status = par_nvm_object_write(p_store, base_addr, g_par_object_persist_slot_to_par_num[idx], false);
        if (ePAR_OK != (status & ePAR_STATUS_ERROR_MASK))
        {
            return status;
        }
    }

    status = par_nvm_object_write_header(p_store, base_addr);
    if (ePAR_OK != status)
    {
        return status;
    }

    PAR_INFO_PRINT("PAR_NVM: object store-all finished, count=%u", (unsigned)object_count);
    return ePAR_OK;
}

/**
 * @brief Try to restore object payloads from one object persistence block.
 *
 * @details This helper validates the block header, optional table-ID digest,
 * and every stored record before committing restored payload lengths. It keeps
 * normal startup restore conservative: compatible tail append can request a
 * rewrite, while incompatible or corrupt blocks fall back to defaults.
 *
 * @param p_store Active storage backend API.
 * @param base_addr Object persistence block base address to read from.
 * @param p_need_rewrite Output rewrite request for compatible tail append.
 * @return Restore status for the selected block.
 */
static par_status_t par_nvm_object_try_restore_block(const par_store_backend_api_t * const p_store,
                                                     const uint32_t base_addr,
                                                     bool * const p_need_rewrite)
{
    const uint16_t object_count = par_nvm_object_compile_count();
    par_status_t status = ePAR_OK;
    par_nvm_object_head_t head = { 0 };

    PAR_ASSERT((NULL != p_store) && (NULL != p_need_rewrite));
    if ((NULL == p_store) || (NULL == p_need_rewrite))
    {
        return ePAR_ERROR_PARAM;
    }

    *p_need_rewrite = false;
    status = par_nvm_object_read_header(p_store, base_addr, &head);
    if (ePAR_OK == (status & ePAR_STATUS_ERROR_MASK))
    {
        status |= par_nvm_object_validate_header(&head);
    }

#if (1 == PAR_CFG_TABLE_ID_CHECK_EN)
    if (ePAR_OK == (status & ePAR_STATUS_ERROR_MASK))
    {
        const uint32_t live_table_id = par_nvm_object_table_id_calc_for_count(head.obj_nb);
        if (head.table_id != live_table_id)
        {
            PAR_WARN_PRINT("PAR_NVM: object table-ID mismatch, stored=0x%08lX live=0x%08lX",
                           (unsigned long)head.table_id,
                           (unsigned long)live_table_id);
            status |= ePAR_ERROR_TABLE_ID;
        }
    }
#else
    /* Without table-ID checking, metadata validation is the available schema
     * guard. Allow a shorter stored prefix so tail-appended persistent objects
     * can keep their existing values and initialize only the new records.
     */
#endif /* (1 == PAR_CFG_TABLE_ID_CHECK_EN) */

    if (ePAR_OK == (status & ePAR_STATUS_ERROR_MASK))
    {
        for (uint16_t idx = 0U; idx < head.obj_nb; idx++)
        {
            const par_num_t par_num = g_par_object_persist_slot_to_par_num[idx];

            status |= par_nvm_object_read_record(p_store,
                                                 par_nvm_object_addr_from_persist_idx(base_addr, idx),
                                                 par_num);
            if (ePAR_OK != (status & ePAR_STATUS_ERROR_MASK))
            {
                break;
            }
        }
    }

    if ((ePAR_OK == (status & ePAR_STATUS_ERROR_MASK)) &&
        (head.obj_nb < object_count))
    {
        for (uint16_t idx = head.obj_nb; idx < object_count; idx++)
        {
            status |= par_set_to_default(g_par_object_persist_slot_to_par_num[idx]);
        }
        *p_need_rewrite = true;
    }

    return status;
}


/**
 * @brief Initialize and restore object persistence through the active object store.
 *
 * @param base_addr Object persistence block base address.
 * @return Operation status.
 */
par_status_t par_nvm_object_init_from_active_store(const uint32_t base_addr)
{
    return par_nvm_object_init(par_nvm_object_store_get_api(), base_addr);
}

/**
 * @brief Persist one object parameter through the active object store.
 *
 * @param base_addr Object persistence block base address.
 * @param par_num Parameter number.
 * @param nvm_sync Request backend sync after write when true.
 * @return Operation status.
 */
par_status_t par_nvm_object_write_to_active_store(const uint32_t base_addr,
                                                  const par_num_t par_num,
                                                  const bool nvm_sync)
{
    return par_nvm_object_write(par_nvm_object_store_get_api(), base_addr, par_num, nvm_sync);
}

/**
 * @brief Persist one object parameter while the caller already holds its mutex.
 *
 * @param base_addr Object persistence block base address.
 * @param par_num Parameter number.
 * @param nvm_sync Request backend sync after write when true.
 * @return Operation status.
 */
par_status_t par_nvm_object_write_locked_to_active_store(const uint32_t base_addr,
                                                         const par_num_t par_num,
                                                         const bool nvm_sync)
{
    return par_nvm_object_write_locked(par_nvm_object_store_get_api(), base_addr, par_num, nvm_sync);
}

/**
 * @brief Persist all object parameters through the active object store.
 *
 * @param base_addr Object persistence block base address.
 * @return Operation status.
 */
par_status_t par_nvm_object_write_all_to_active_store(const uint32_t base_addr)
{
    return par_nvm_object_write_all(par_nvm_object_store_get_api(), base_addr);
}

/**
 * @brief Initialize and restore the object persistence block.
 *
 * @details Startup validates the object header, object schema digest and each
 * persistent object record. Recoverable corruption or schema mismatch restores
 * object defaults first and rewrites the object block from current RAM values.
 * The core deliberately does not migrate object blocks between addresses;
 * flash-safe relocation requires product-specific scratch or double-buffer
 * storage, commit state, power-loss recovery, and erase-block planning.
 *
 * @param p_store Active storage backend API.
 * @param base_addr Object persistence block base address.
 * @return Operation status.
 */
par_status_t par_nvm_object_init(const par_store_backend_api_t * const p_store,
                                 const uint32_t base_addr)
{
    const uint16_t object_count = par_nvm_object_compile_count();
    par_status_t status = ePAR_OK;
    par_status_t detect_status = ePAR_OK;
    const par_status_t recoverable_errors = (par_status_t)(ePAR_ERROR | ePAR_ERROR_CRC | ePAR_ERROR_TABLE_ID | ePAR_ERROR_VALUE);
    par_status_t unexpected_errors = ePAR_OK;
    bool need_defaults = false;
    bool need_rewrite = false;

    if (NULL == p_store)
    {
        return ePAR_ERROR_PARAM;
    }

    if (0U == object_count)
    {
        return ePAR_OK;
    }

    detect_status = par_nvm_object_try_restore_block(p_store, base_addr, &need_rewrite);

    unexpected_errors = (par_status_t)(detect_status &
                                       (par_status_t)(ePAR_STATUS_ERROR_MASK & (par_status_t)(~(recoverable_errors | ePAR_ERROR_NVM))));

    if (0U != (detect_status & recoverable_errors))
    {
        need_defaults = true;
        need_rewrite = true;
    }
    if (0U != (detect_status & ePAR_ERROR_NVM))
    {
        need_defaults = true;
    }
    if (0U != unexpected_errors)
    {
        status |= unexpected_errors;
    }

    if (true == need_defaults)
    {
        status |= par_nvm_object_set_persistent_defaults();
        if (ePAR_OK == (status & ePAR_STATUS_ERROR_MASK))
        {
            status |= ePAR_WAR_SET_TO_DEF;
        }
    }

    if ((true == need_rewrite) && (ePAR_OK == (status & ePAR_STATUS_ERROR_MASK)))
    {
        status |= par_nvm_object_write_all(p_store, base_addr);
        if (ePAR_OK == (status & ePAR_STATUS_ERROR_MASK))
        {
            status |= ePAR_WAR_NVM_REWRITTEN;
        }
    }

    if (0U != (detect_status & ePAR_ERROR_NVM))
    {
        status |= ePAR_ERROR_NVM;
    }

    PAR_INFO_PRINT("PAR_NVM: object initialization finished with status=%s", par_get_status_str(status));
    return status;
}
#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */
