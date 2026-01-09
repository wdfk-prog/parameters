// Copyright (c) 2026 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      par_nvm.c
*@brief     Parameter storage to non-volatile memory
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@date      06.12.2024
*@version   V2.2.0
*/
////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup PAR_NVM
* @{ <!-- BEGIN GROUP -->
*
*     Parameter storage to non-volatile memory handling.
*
*
* @pre        NVM module shall have memory region called "Parameters".
*
*             NVM module must be initialized before calling any of following
*             functions.
*
* @brief    This module is responsible for parameter NVM object creation and
*             storage manipulation. NVM parameter object consist of it's value
*             and a CRC value for validation purposes.
*
*             Parameter storage is reserved in "Parameters" region of NVM. Look
*             at the nvm_cfg.h/c module for NVM region descriptions.
*
*            Parameters stored into NVM in little endianness format.
*
*            For details how parameters are handled in NVM go look at the
*            documentation.
*
* @note        RULES OF "PAR_CFG_TABLE_ID_CHECK_EN" SETTINGS:
*
*             It is normal that parameter table will change during development
*             and therefore code will detect table change between "RAM" and "NVM"
*             (detection of par table change enable/disable with
*             "PAR_CFG_TABLE_ID_CHECK_EN" setting).
*
*             But as SW is released and potential at customer, developer must not
*             change the pre-existing parameter table as it will loose all values
*             stored inside NVM. Therefore it is recommended to disable table ID
*             detection after first release of SW. Adding new parameters to pre-existing
*             table has no harm at all nor does it have any side effects.
*
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include "par_nvm.h"
#include "../../par_cfg.h"
#include "../../par_if.h"

#if ( 1 == PAR_CFG_NVM_EN )

    #include <assert.h>
    #include <string.h>

    #include "middleware/nvm/nvm/src/nvm.h"

    /**
     *     Check NVM module compatibility
     */
    _Static_assert( 2 == NVM_VER_MAJOR );
    _Static_assert( 1 <= NVM_VER_MINOR );

    ////////////////////////////////////////////////////////////////////////////////
    // Definitions
    ////////////////////////////////////////////////////////////////////////////////


    /**
     *     Parameter signature and size in bytes
     */
    #define PAR_NVM_SIGN                            ( 0xFF00AA55 )
    #define PAR_NVM_SIGN_SIZE                       ( 4U )

    /**
     *     Parameter header number of object size
     *
     *     Unit: byte
     */
    #define PAR_NVM_NB_OF_OBJ_SIZE                  ( 2U )

    /**
     *     Parameter CRC size
     *
     *     Unit: byte
     */
    #define PAR_NVM_CRC_SIZE                        ( 2U )

    /**
     *     Parameter configuration hash size
     *
     *     Unit: byte
     */
    #define PAR_NVM_HASH_SIZE                       ( 32U )

    /**
     *     Parameter NVM header content address start
     *
     *     @note     This is offset to reserved NVM region. For absolute address
     *             add that value to NVM start region.
     */
    #define PAR_NVM_HEAD_ADDR                       ( 0x00 )
    #define PAR_NVM_HEAD_SIGN_ADDR                  ( PAR_NVM_HEAD_ADDR )
    #define PAR_NVM_HEAD_NB_OF_OBJ_ADDR             ( PAR_NVM_HEAD_SIGN_ADDR        + PAR_NVM_SIGN_SIZE         )
    #define PAR_NVM_HEAD_CRC_ADDR                   ( PAR_NVM_HEAD_NB_OF_OBJ_ADDR   + PAR_NVM_NB_OF_OBJ_SIZE    )
    #define PAR_NVM_HEAD_HASH_ADDR                  ( PAR_NVM_HEAD_CRC_ADDR         + PAR_NVM_CRC_SIZE          )

    /**
     *     Parameters first data object start address
     *
     *     Unit: byte
     */
    #define PAR_NVM_FIRST_DATA_OBJ_ADDR             ( PAR_NVM_HEAD_HASH_ADDR + PAR_NVM_HASH_SIZE )

    /**
     *     Parameter NVM header object
     */
    typedef struct
    {
        uint32_t sign;      /**<Signature */
        uint16_t obj_nb;    /**<Stored data object number */
        uint16_t crc;       /**<Header CRC */
    } par_nvm_head_obj_t;

    /**
     *     Parameter NVM data object
     */
    typedef struct
    {
        uint16_t    id;     /**<Parameter ID */
        uint8_t     size;   /**<Size of parameter data block */
        uint8_t     crc;    /**<CRC of parameter value */
        par_type_t  data;   /**<4-byte storage for parameter value */
    } par_nvm_data_obj_t;

    /**
     *     Parameter NVM LUT talbe
     */
    typedef struct
    {
        uint32_t    addr;   /**<Start address of parameter */
        uint16_t    id;     /**<ID of stored parameter */
        bool        valid;  /**<Valid entry */
    } par_nvm_lut_t;

    ////////////////////////////////////////////////////////////////////////////////
    // Variables
    ////////////////////////////////////////////////////////////////////////////////

    /**
     *      Initialization guard
     */
     static bool gb_is_init = false;

    /**
     *     Parameter NVM lut
     */
    static par_nvm_lut_t g_par_nvm_data_obj_addr[ePAR_NUM_OF] = {0};

    ////////////////////////////////////////////////////////////////////////////////
    // Function Prototypes
    ////////////////////////////////////////////////////////////////////////////////
    static par_status_t par_nvm_load_all(const uint16_t num_of_par);

    static par_status_t par_nvm_corrupt_signature(void);
    static par_status_t par_nvm_read_header      (par_nvm_head_obj_t * const p_head_obj);
    static par_status_t par_nvm_write_header     (const uint16_t num_of_par);
    static par_status_t par_nvm_validate_header  (uint16_t * const p_num_of_par);

    static uint16_t par_nvm_calc_crc    (const uint8_t * const p_data, const uint8_t size);
    static uint8_t  par_nvm_calc_obj_crc(const par_nvm_data_obj_t * const p_obj);
    static uint16_t par_nvm_get_per_par (void);

    static void     par_nvm_build_new_nvm_lut(void);
    static uint32_t par_nvm_get_nvm_lut_addr (const uint16_t id);
    static bool     par_nvm_is_in_nvm_lut    (const uint16_t id );

    #if ( 1 == PAR_CFG_TABLE_ID_CHECK_EN )
        static par_status_t    par_nvm_erase_signature    (void);
        static par_status_t par_nvm_check_table_id    (const uint8_t * const p_table_id);
        static par_status_t par_nvm_write_table_id    (const uint8_t * const p_table_id);
    #endif

    static par_status_t par_nvm_init_nvm(void);
    static par_status_t par_nvm_sync    (void);

    ////////////////////////////////////////////////////////////////////////////////
    // Functions
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Corrupt parameter signature to NVM
    *
    * @brief    Return ePAR_OK if signature corrupted OK. In case of NVM error it returns
    *             ePAR_ERROR_NVM.
    *
    * @return        status - Status of operation
    */
    ////////////////////////////////////////////////////////////////////////////////
    static par_status_t par_nvm_corrupt_signature(void)
    {
        par_status_t status = ePAR_OK;

        if ( eNVM_OK != nvm_erase( PAR_CFG_NVM_REGION, PAR_NVM_HEAD_SIGN_ADDR, PAR_NVM_SIGN_SIZE ))
        {
            status = ePAR_ERROR_NVM;
            PAR_DBG_PRINT( "PAR_NVM: NVM error during signature corruption!" );
        }

        return status;
    }

    #if ( 1 == PAR_CFG_TABLE_ID_CHECK_EN )

        ////////////////////////////////////////////////////////////////////////////////
        /**
        *        Erase parameter signature from NVM
        *
        * @return        status     - Status of operation
        */
        ////////////////////////////////////////////////////////////////////////////////
        static par_status_t par_nvm_erase_signature(void)
        {
            par_status_t status = ePAR_OK;

            status = nvm_erase( PAR_CFG_NVM_REGION, PAR_NVM_SIGNATURE_ADDR_OFFSET, 4U );

            return status;
        }

        ////////////////////////////////////////////////////////////////////////////////
        /**
        *        Check unique parameter table ID
        *
        * @brief    This function check for parameter configuration table change while
        *             some parameters are already stored in NVM. First it read stored
        *             table ID from NVM and then compare it with current table ID (live
        *             from RAM). In case of mismatched it return error.
        *
        *             Table ID is being calculated based on hash algorithm (SHA-256).
        *
        * @param[in]    p_table_id - Pointer to reference table ID (in "RAM")
        * @return       status     - Status of operation
        */
        ////////////////////////////////////////////////////////////////////////////////
        static par_status_t par_nvm_check_table_id(const uint8_t * const p_table_id)
        {
            par_status_t status             = ePAR_OK;
            uint8_t      nvm_table_id[32]   = { 0 };

            if ( eNVM_OK != nvm_read( eNVM_REGION_EEPROM_RUN_PAR, PAR_NVM_TABLE_ID_ADDR_OFFSET, 32U, (uint8_t*) &nvm_table_id ))
            {
                status = ePAR_ERROR_NVM;
            }
            else
            {
                // Table ID is the same in "RAM" and in NVM
                if ( 0 == memcmp( &nvm_table_id, p_table_id, 32U ))
                {
                    status = ePAR_OK;
                }

                // Different table ID found
                else
                {
                    status = ePAR_ERROR;
                }
            }

            return status;
        }

        ////////////////////////////////////////////////////////////////////////////////
        /**
        *        Write unique parameter table ID to NVM
        *
        * @param[in]    p_table_id    - Pointer to reference table ID (in "RAM")
        * @return        status         - Status of operation
        */
        ////////////////////////////////////////////////////////////////////////////////
        static par_status_t par_nvm_write_table_id(const uint8_t * const p_table_id)
        {
            par_status_t status = ePAR_OK;

            if ( eNVM_OK != nvm_write( eNVM_REGION_EEPROM_RUN_PAR, PAR_NVM_TABLE_ID_ADDR_OFFSET, 32U, p_table_id ))
            {
                status = ePAR_ERROR_NVM;
            }

            return status;
        }

    #endif // 1 == PAR_CFG_TABLE_ID_CHECK_EN

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Read parameter NVM header
    *
    * @param[in]    p_head_obj    - Pointer to parameter NVM header
    * @return        status         - Status of operation
    */
    ////////////////////////////////////////////////////////////////////////////////
    static par_status_t par_nvm_read_header(par_nvm_head_obj_t * const p_head_obj)
    {
        par_status_t status = ePAR_OK;

        PAR_ASSERT( NULL != p_head_obj );

        if ( eNVM_OK != nvm_read( PAR_CFG_NVM_REGION, PAR_NVM_HEAD_ADDR, sizeof( par_nvm_head_obj_t ), (uint8_t*) p_head_obj ))
        {
            status = ePAR_ERROR_NVM;
            PAR_DBG_PRINT( "PAR_NVM: NVM error during header read!" );
        }

        return status;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Write parameter NVM header
    *
    * @param[in]    num_of_par - Number of persistent parameters that are stored in NVM
    * @return       status     - Status of operation
    */
    ////////////////////////////////////////////////////////////////////////////////
    static par_status_t par_nvm_write_header(const uint16_t num_of_par)
    {
        par_status_t       status   = ePAR_OK;
        par_nvm_head_obj_t head_obj = {0};

        // Add number of objects
        head_obj.obj_nb = num_of_par;

        // Calculate CRC
        head_obj.crc = par_nvm_calc_crc((uint8_t*) &head_obj.obj_nb, PAR_NVM_NB_OF_OBJ_SIZE );

        // Set signature
        head_obj.sign = PAR_NVM_SIGN;

        // Write num of object and CRC
        if ( eNVM_OK != nvm_write( PAR_CFG_NVM_REGION, PAR_NVM_HEAD_ADDR, sizeof( par_nvm_head_obj_t ), (const uint8_t*) &head_obj ))
        {
            status = ePAR_ERROR_NVM;
            PAR_DBG_PRINT( "PAR_NVM: NVM error during header write!" );
        }

        PAR_DBG_PRINT( "PAR_NVM: Write NVM header with %d nb. of object", num_of_par );

        return status;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Validate parameter NVM header
    *
    * @param[out]    p_num_of_par - Pointer to number of persistent parameters that are stored in NVM
    * @return        status       - Status of operation
    */
    ////////////////////////////////////////////////////////////////////////////////
    static par_status_t par_nvm_validate_header(uint16_t * const p_num_of_par)
    {
        par_status_t       status   = ePAR_OK;
        par_nvm_head_obj_t obj_head = { 0 };
        uint16_t           crc_calc = 0;

        // Read header
        status = par_nvm_read_header( &obj_head );

        // NVM error
        if ( ePAR_ERROR_NVM == status )
        {
            // No actions...
        }
        else
        {
            // Check for signature
            if ( PAR_NVM_SIGN == obj_head.sign )
            {
                // Calculate CRC
                crc_calc = par_nvm_calc_crc((uint8_t*) &obj_head.obj_nb, PAR_NVM_NB_OF_OBJ_SIZE );

                // Validate CRC
                if ( crc_calc == obj_head.crc )
                {
                    *p_num_of_par = obj_head.obj_nb;
                    PAR_DBG_PRINT( "PAR_NVM: HVM header OK! Nb. of stored obj: %d", obj_head.obj_nb );
                }

                // CRC corrupt
                else
                {
                    status = ePAR_ERROR_CRC;
                    PAR_DBG_PRINT( "PAR_NVM: Header CRC corrupted!" );
                }
            }
            else
            {
                status = ePAR_ERROR;
                PAR_DBG_PRINT( "PAR_NVM: Signature corrupted!" );
            }
        }

        return status;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Calculate CRC-16
    *
    * @param[in]    p_data - Pointer to data
    * @param[in]    size   - Size of data to calc crc
    * @return       crc16  - Calculated CRC
    */
    ////////////////////////////////////////////////////////////////////////////////
    static uint16_t par_nvm_calc_crc(const uint8_t * const p_data, const uint8_t size)
    {
        const     uint16_t poly     = 0x1021U;    // CRC-16-CCITT
        const     uint16_t seed     = 0x1234U;    // Custom seed
                uint16_t crc16     = seed;

        // Check input
        PAR_ASSERT( NULL != p_data );
        PAR_ASSERT( size > 0 );

        for (uint8_t i = 0; i < size; i++)
        {
            crc16 = ( crc16 ^ ( p_data[i] << 8U ));

            for (uint8_t j = 0U; j < 8U; j++)
            {
                if (crc16 & 0x8000)
                {
                    crc16 = (( crc16 << 1U ) ^ poly );
                }
                else
                {
                    crc16 = ( crc16 << 1U );
                }
            }
        }

        return crc16;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Calculate parameter data object CRC
    *
    * @param[in]    p_obj - Pointer to data object
    * @return       crc16 - Calculated CRC
    */
    ////////////////////////////////////////////////////////////////////////////////
    static uint8_t par_nvm_calc_obj_crc(const par_nvm_data_obj_t * const p_obj)
    {
        uint16_t crc     = 0;
        uint8_t  rtn_crc = 0;

        crc = par_nvm_calc_crc((const uint8_t*) &p_obj->id, 2U );
        crc ^= par_nvm_calc_crc((const uint8_t*) &p_obj->size, 1U );
        crc ^= par_nvm_calc_crc((const uint8_t*) &p_obj->data.u8, 4U );
        rtn_crc = ( crc & 0xFFU );

        return rtn_crc;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Load all parameters value from NVM
    *
    * @param[in]    num_of_par - Number of stored parameters inside NVM
    * @return       status     - Status of operation
    */
    ////////////////////////////////////////////////////////////////////////////////
    static par_status_t par_nvm_load_all(const uint16_t num_of_par)
    {
        par_status_t       status      = ePAR_OK;
        par_num_t          par_num     = 0;
        uint16_t           i           = 0;
        uint32_t           obj_addr    = 0;
        par_nvm_data_obj_t obj_data    = {0};
        nvm_status_t       nvm_status  = eNVM_OK;
        uint8_t            crc_calc    = 0;
        uint16_t           per_par_nb  = 0;
        par_cfg_t          par_cfg     = {0};
        uint16_t           new_par_cnt = 0;

        // Loop thru stored NVM object
        for ( i = 0; i < num_of_par; i++ )
        {
            // Calculate address
            // NOTE: For know fixed 8 bytes!
            obj_addr = (( 8 * i ) + PAR_NVM_FIRST_DATA_OBJ_ADDR );

            // Load parameter NVM object
            nvm_status = nvm_read( PAR_CFG_NVM_REGION, obj_addr, sizeof( par_nvm_data_obj_t ), (uint8_t*) &obj_data );

            // NVM read OK
            if ( eNVM_OK == nvm_status )
            {
                // Calculate CRC
                crc_calc = par_nvm_calc_obj_crc( &obj_data );

                // CRC OK
                if ( crc_calc == obj_data.crc )
                {
                    // Is that parameter in current table
                    if ( ePAR_OK == par_get_num_by_id( obj_data.id, &par_num ))
                    {
                        par_get_config( par_num, &par_cfg );

                        /**
                         *     Parameter found in device and stored in NVM
                         *
                         *     Check if that parameter is still persistent!
                         */
                        if ( true == par_cfg.persistant )
                        {
                            // Check if already in LUT
                            if ( false == par_nvm_is_in_nvm_lut( obj_data.id ))
                            {
                                // Add to NVM lut
                                g_par_nvm_data_obj_addr[per_par_nb].id    = obj_data.id;
                                g_par_nvm_data_obj_addr[per_par_nb].addr  = obj_addr;
                                g_par_nvm_data_obj_addr[per_par_nb].valid = true;

                                // Set parameter
                                par_set( par_num, &obj_data.data );

                                // Increment current persistent parameter counter
                                per_par_nb++;
                            }
                        }
                    }

                    // Parameter not in current table
                    else
                    {
                        // No action...
                    }
                }

                // CRC corrupted
                else
                {
                    status = ePAR_ERROR_CRC;
                    break;
                }
            }
            else
            {
                status = ePAR_ERROR_NVM;
                break;
            }
        }

        PAR_DBG_PRINT( "PAR_NVM: Loading all persistent parameters with status: %s", par_get_status_str(status));
        PAR_DBG_PRINT( "PAR_NVM: Nb. of stored pars in NVM: %d", num_of_par );
        PAR_DBG_PRINT( "PAR_NVM: Nb. of live persistent: \t%d", par_nvm_get_per_par());

        // Find new persistent parameter
        if ( ePAR_OK == status )
        {
            for ( i = 0; i < ePAR_NUM_OF; i++ )
            {
                par_get_config( i, &par_cfg );

                if ( true == par_cfg.persistant )
                {
                    if ( false == par_nvm_is_in_nvm_lut( par_cfg.id ))
                    {
                        // Is persistant and not jet in NVM lut -> Add to LUT
                        g_par_nvm_data_obj_addr[per_par_nb].id    = par_cfg.id;
                        g_par_nvm_data_obj_addr[per_par_nb].addr  = obj_addr + ( 8U * ( new_par_cnt + 1U ));
                        g_par_nvm_data_obj_addr[per_par_nb].valid = true;

                        // Write new par to NVM
                        par_save( i );

                        per_par_nb++;
                        new_par_cnt++;
                    }
                }
            }

            // If there is a new persistent parameter change HVM header
            if ( new_par_cnt > 0 )
            {
                // Add additional new persistent parameters number to existing one!
                // NOTE: In general obj number will only rise!
                status |= par_nvm_write_header( num_of_par + new_par_cnt );

                // Sync NVM
                status |= par_nvm_sync();

                #if ( PAR_CFG_DEBUG_EN )
                    PAR_DBG_PRINT( "PAR_NVM: Added %d new parameters to NVM LUT table!", new_par_cnt );
                #endif
            }
        }

        return status;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Get total number of persistent parameters
    *
    * @return    num_of_per_par - Number of persistent parameters
    */
    ////////////////////////////////////////////////////////////////////////////////
    static uint16_t par_nvm_get_per_par(void)
    {
        uint16_t  num_of_per_par = 0U;
        uint16_t  par_num        = 0U;
        par_cfg_t par_cfg        = {0};

        for ( par_num = 0; par_num < ePAR_NUM_OF; par_num++ )
        {
            par_get_config( par_num, &par_cfg );

            if ( true == par_cfg.persistant )
            {
                num_of_per_par++;
            }
        }

        return num_of_per_par;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Build new parameter NVM LUT table
    *
    * @return    void
    */
    ////////////////////////////////////////////////////////////////////////////////
    static void par_nvm_build_new_nvm_lut(void)
    {
        uint16_t  per_par_nb = 0U;
        par_num_t par_num    = 0U;
        par_cfg_t par_cfg    = {0};

        // Loop thru all parameters
        for ( par_num = 0; par_num < ePAR_NUM_OF; par_num++ )
        {
            par_get_config( par_num, &par_cfg );

            if ( true == par_cfg.persistant )
            {
                // First parameter
                if ( 0 == per_par_nb )
                {
                    g_par_nvm_data_obj_addr[per_par_nb].addr = PAR_NVM_FIRST_DATA_OBJ_ADDR;
                }

                // Build consecutive address space
                // NOTE: For know each NVM data object is fixed in size (8 bytes)
                else
                {
                    // Calculate and add address of next parameter
                    g_par_nvm_data_obj_addr[per_par_nb].addr = ( g_par_nvm_data_obj_addr[per_par_nb-1].addr + 8 );
                }

                // Store parameter ID
                g_par_nvm_data_obj_addr[per_par_nb].id = par_cfg.id;

                // Next persistent parameter
                per_par_nb++;
            }
        }
        
        // Show NVM LUT table
        par_nvm_print_nvm_lut();
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Get parameter NVM object start address based on its ID
    *
    * @note     In case ID is not found there is a problem with building the
    *           NVM lut!!!
    *
    * @param[in]    id       - Parameter ID
    * @return       obj_addr - NVM address of object with ID
    */
    ////////////////////////////////////////////////////////////////////////////////
    static uint32_t par_nvm_get_nvm_lut_addr(const uint16_t id)
    {
        uint32_t    obj_addr    = 0;
        par_num_t    par_num        = 0;

        for ( par_num = 0; par_num < ePAR_NUM_OF; par_num++ )
        {
            if ( id == g_par_nvm_data_obj_addr[par_num].id )
            {
                obj_addr = g_par_nvm_data_obj_addr[par_num].addr;
                break;
            }
        }

        // ZIGA: TODO: Validate if address is found correctly!!!

        return obj_addr;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Check if parameter is in NVM LUT
    *
    * @param[in]    id        - Parameter ID
    * @return       is_in_lut - Flag that indicated if object is in NVM lut
    */
    ////////////////////////////////////////////////////////////////////////////////
    static bool par_nvm_is_in_nvm_lut(const uint16_t id )
    {
        bool      is_in_lut = false;
        par_num_t par_num   = 0;

        for ( par_num = 0; par_num < ePAR_NUM_OF; par_num++ )
        {
            if  (   ( id == g_par_nvm_data_obj_addr[par_num].id )
                &&  ( true == g_par_nvm_data_obj_addr[par_num].valid ))
            {
                is_in_lut = true;
                break;
            }
        }

        return is_in_lut;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Initialize NVM module
    *
    * @return        status - Status of operation
    */
    ////////////////////////////////////////////////////////////////////////////////
    static par_status_t par_nvm_init_nvm(void)
    {
        par_status_t status      = ePAR_OK;
        bool         is_nvm_init = false;

        // First check if NVM is already init
        (void) nvm_is_init( &is_nvm_init );
        
        // NVM is not jet init
        if ( false == is_nvm_init )
        {
            // Init NVM
            if ( eNVM_OK != nvm_init())
            {
                status = ePAR_ERROR_INIT;
                PAR_DBG_PRINT( "PAR_NVM: NVM module init error!" );  
            }
        }

        return status;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Sync NVM module
    *
    * @return        status - Status of operation
    */
    ////////////////////////////////////////////////////////////////////////////////
    static par_status_t par_nvm_sync(void)
    {
        par_status_t status = ePAR_OK;

        if ( eNVM_OK != nvm_sync( PAR_CFG_NVM_REGION ))
        {
            status = ePAR_ERROR_NVM;
        }

        return status;       
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    * @} <!-- END GROUP -->
    */
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *@addtogroup API_PAR_NVM_FUNCTIONS
    * @{ <!-- BEGIN GROUP -->
    *
    *     Following functions are part of Device Parameter NVM manipulation API.
    */
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Initialize parameter NVM handling
    *
    * @brief    Based on settings in "par_cfg.h" initialisation phase is done.
    *           Settings such as "PAR_CFG_TABLE_ID_CHECK_EN" will affect checking
    *           for table ID.
    *
    * @return    status - Status of initialisation
    */
    ////////////////////////////////////////////////////////////////////////////////
    par_status_t par_nvm_init(void)
    {
        par_status_t status     = ePAR_OK;
        uint16_t     obj_nb     = 0U;
        uint16_t     per_par_nb = 0U;

        // Init NVM module
        status = par_nvm_init_nvm();

        // NVM driver init OK
        if ( ePAR_OK == status )
        {
            // Par NVM module init
            gb_is_init = true;

            // Get number of persistent parameters
            per_par_nb = par_nvm_get_per_par();

            // At least one persistent parameter
            if ( per_par_nb > 0 )
            {
                // Validate header
                status = par_nvm_validate_header( &obj_nb );

                // NVM header OK
                if ( ePAR_OK == status )
                {
                    // Check hash
                    #if ( PAR_CFG_TABLE_ID_CHECK_EN )
                        // TODO: ...
                    #endif

                    // Load all parameters
                    status = par_nvm_load_all( obj_nb );

                    // Load CRC error
                    if ( ePAR_ERROR_CRC == status )
                    {
                        status = par_nvm_reset_all();

                        status |= ePAR_WARN_SET_TO_DEF;
                        status |= ePAR_WARN_NVM_REWRITTEN;
                    }

                    // NVM error
                    else if ( ePAR_ERROR_NVM == status )
                    {
                        /**
                         * @note    Set all parameters to default as it might happend
                         *          that some of the parameters will be loaded from
                         *          NVM and some will have default values.
                         *
                         *          System might behave unexpectedly if having some
                         *          default and some modified parameter values!
                         */
                        par_set_all_to_default();

                        status |= ePAR_WARN_SET_TO_DEF;
                    }
                    else
                    {
                        // No actions...
                    }
                }

                //      Signature NOT OK
                // OR   Header CRC corrupted
                else if (   ( ePAR_ERROR == status )
                        ||  ( ePAR_ERROR_CRC == status ))
                {
                    status = par_nvm_reset_all();

                    status |= ePAR_WARN_SET_TO_DEF;
                    status |= ePAR_WARN_NVM_REWRITTEN;
                }

                // NVM Error
                else
                {
                    // No actions...
                }
            }

            // No persistent parameters
            else
            {
                status |= ePAR_WARN_NO_PERSISTANT;
                PAR_DBG_PRINT( "PAR_NVM: No persistent parameters... Nothing to do..." );
            }
        }

        return status;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        De-Initialize parameter NVM handling
    *
    * @return    status - Status of de-init
    */
    ////////////////////////////////////////////////////////////////////////////////
    par_status_t par_nvm_deinit(void)
    {
        par_status_t status = ePAR_OK;

        if ( true == gb_is_init )
        {
            if ( eNVM_OK != nvm_deinit())
            {
                status = ePAR_ERROR;
            }
        }
        else
        {
            status = ePAR_ERROR;
        }

        return status;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Store parameter value to NVM
    *
    * @note     Sync has only effect when using EEPROM emulated NVM feature! When
    *           using Flash end memory device.
    *
    * @note     In case of using Flash end memory for storing parameters take special
    *           care when enabling sync (nvm_sync=true). At each sync data from RAM
    *           is copied to FLASH.
    *
    * @param[in]    par_num  - Parameter enumeration number
    * @param[in]    nvm_sync - Perform NVM sync after parameter write
    * @return       status   - Status of operation
    */
    ////////////////////////////////////////////////////////////////////////////////
    par_status_t par_nvm_write(const par_num_t par_num, const bool nvm_sync)
    {
        par_status_t       status   = ePAR_OK;
        par_nvm_data_obj_t obj_data = { 0 };
        uint32_t           par_addr = 0UL;
        par_cfg_t          par_cfg  = {0};

        PAR_ASSERT( true == gb_is_init );
        PAR_ASSERT( par_num < ePAR_NUM_OF );

        if ( true == gb_is_init )
        {
            if ( par_num < ePAR_NUM_OF )
            {
                // Get configuration
                par_get_config( par_num, &par_cfg );

                // Is that parameter persistent
                if ( true == par_cfg.persistant )
                {
                    // Get current par value
                    par_get( par_num, (uint32_t*) &obj_data.data );

                    // Get parameter ID
                    obj_data.id = par_cfg.id;

                    // Get parameter type size
                    // NOTE: For know fixed!
                    //par_get_type_size( par_cfg.type, &obj_data.size );
                    obj_data.size = 4;

                    // Calculate CRC
                    obj_data.crc = par_nvm_calc_obj_crc( &obj_data );

                    // Get address from NVM lut
                    //par_addr = g_par_nvm_data_obj_addr[par_num].addr;
                    par_addr = par_nvm_get_nvm_lut_addr( obj_data.id );

                    // Write to NVM
                    if ( eNVM_OK != nvm_write( PAR_CFG_NVM_REGION, par_addr, sizeof( par_nvm_data_obj_t ), (const uint8_t*) &obj_data ))
                    {
                        status |= ePAR_ERROR_NVM;
                    }

                    // Sync NVM
                    if ( true == nvm_sync )
                    {
                        status |= par_nvm_sync();
                    }
                }
                else
                {
                    status = ePAR_ERROR;
                }
            }
            else
            {
                status = ePAR_ERROR;
            }
        }
        else
        {
            status = ePAR_ERROR_INIT;
        }

        return status;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Store all parameter value to NVM
    *
    * @return        status     - Status of operation
    */
    ////////////////////////////////////////////////////////////////////////////////
    par_status_t par_nvm_write_all(void)
    {
        par_status_t status  = ePAR_OK;
        uint16_t     par_num = 0;
        par_cfg_t    par_cfg = { 0 };

        PAR_ASSERT( true == gb_is_init );

        if ( true == gb_is_init )
        {
            // Corrupt header (enter critical)
            status |= par_nvm_corrupt_signature();

            // Got thru all parameters
            for ( par_num = 0UL; par_num < ePAR_NUM_OF; par_num++ )
            {
                // Get parameter configuration
                par_get_config( par_num, &par_cfg );

                // Store only persistant one
                if ( true == par_cfg.persistant )
                {
                    // Sync will be done later
                    status |= par_nvm_write( par_num, false );
                }
            }

            // Re-write header (exit critical)
            status |= par_nvm_write_header( par_nvm_get_per_par());

            // Sync NVM
            status |= par_nvm_sync();

            PAR_DBG_PRINT( "PAR_NVM: Storing all to NVM status: %s", par_get_status_str(status));
        }
        else
        {
            status = ePAR_ERROR_INIT;
        }

        return status;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Reset total section of parameters NVM
    *
    * @brief    This function completely re-write whole NVM section.
    *
    *             It first corrupt signature in order to raise "working in progress"
    *             flag.
    *
    * @return    num_of_per_par - Number of persistent parameters
    */
    ////////////////////////////////////////////////////////////////////////////////
    par_status_t par_nvm_reset_all(void)
    {
        par_status_t status = ePAR_OK;

        PAR_ASSERT( true == gb_is_init );

        if ( true == gb_is_init )
        {
            // Build new NVM lut
            par_nvm_build_new_nvm_lut();

            // Write all data object
            status |= par_nvm_write_all();

            // Re-write header as reseting whole NVM parameter memory

            // ZIGA: Redundant TODO:
            //status |= par_nvm_write_header( par_nvm_get_per_par() );

            // Sync NVM

            // ZIGA: Redundant TODO:
            //status |= par_nvm_sync();
        }
        else
        {
            status = ePAR_ERROR_INIT;
        }

        return status;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Print parameter NVM table
    *
    * @note        Only for debugging purposes
    *
    * @return    void
    */
    ////////////////////////////////////////////////////////////////////////////////
    par_status_t par_nvm_print_nvm_lut(void)
    {
        par_status_t status = ePAR_OK;

        #if ( PAR_CFG_DEBUG_EN )
            uint16_t par_num = 0;

            PAR_DBG_PRINT( "PAR_NVM: Parameter NVM look-up table:" );
            PAR_DBG_PRINT( " %s\t%s\t%s\t\t%s", "#", "ID", "Addr", "Valid" );
            PAR_DBG_PRINT( "===============================" );

            for ( par_num = 0; par_num < ePAR_NUM_OF; par_num++ )
            {
                PAR_DBG_PRINT( " %d\t%d\t0x%04X\t%d", par_num,     g_par_nvm_data_obj_addr[par_num].id,
                                                                g_par_nvm_data_obj_addr[par_num].addr,
                                                                g_par_nvm_data_obj_addr[par_num].valid );
                PAR_DBG_PRINT( "-----------------------------" );
            }
        #endif

        return status;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    * @} <!-- END GROUP -->
    */
    ////////////////////////////////////////////////////////////////////////////////

#endif // 1 == PAR_CFG_NVM_EN
