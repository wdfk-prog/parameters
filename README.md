# **Device Parameters**

The **Device Parameters** module manages all device parameters through a single configuration table, offering a streamlined approach to system configuration and diagnostics. This module often serves as the backbone of an embedded system, controlling the application's behavior and providing insights into device performance, making diagnostics straightforward and efficient.  

For more details about storage into NVM look at the [General Embedded C Library Manual](https://github.com/GeneralEmbeddedCLibraries/documentation/blob/develop/General_Embedded_C_Library_Manual.pdf) document.

## Key Benefits  
- **Centralized Configuration**: Simplifies management by defining all system settings in a single table.
- **Data Integrity**: Integrated Min/Max range checking and custom validation callbacks.
- **Thread Safety**: Optional mutex protection for multi-threaded environments (e.g., RTOS).
- **Persistence**: Transparent NVM (Non-Volatile Memory) integration with CRC and signature verification.
- **Observability**: Built-in support for parameter names, units, and descriptions for easy CLI/GUI integration.
- **Event-Driven**: Chained callback system for notifying other modules of parameter changes.

## Integration with other modules in General Embedded C Libraries ecosystem
When combined with the following modules, the **Device Parameters** module significantly enhances the capabilities of embedded applications:  

1. **[CLI (Command Line Interface)](https://github.com/GeneralEmbeddedCLibraries/cli)**  
   - Enables communication between the embedded application and a PC.  
   - Allows real-time configuration and diagnostics via the CLI interface.  

2. **[NVM (Non-Volatile Memory)](https://github.com/GeneralEmbeddedCLibraries/nvm)**  
   - Ensures that configured settings are stored persistently.  

3. **Device Parameters + CLI + NVM**  
   - Together, these modules allow:  
     - Communication with a PC using CLI.  
     - Configuration and diagnostics of the application via Device Parameters.  
     - Storage of settings to NVM, ensuring data persistence.  

### Development Benefits  
By leveraging this combination of modules, embedded firmware development becomes significantly faster and more efficient, enabling developers to focus on functionality rather than repetitive low-level implementation.  

## **Dependencies**
### **1. Parameter persistance**
In case of using persistant options (*PAR_CFG_NVM_EN = 1*) it is mandatory to use [NVM module](https://github.com/GeneralEmbeddedCLibraries/nvm).

## **Limitations**
 - **Heap Usage:** The module uses malloc during par_init() to allocate RAM space for the parameters based on the configuration table. Ensure your heap is sufficiently sized.
 - **Alignment:** Address offsets are calculated based on 4-byte (32-bit) alignment to satisfy most ARM Cortex-M requirements.
 - **Flat ID Space:** Parameter IDs must be unique across the entire table to ensure NVM consistency.
 - **Execution Time:** If many callbacks are chained to a single parameter, the par_set execution time will increase accordingly.

## **General Embedded C Libraries Ecosystem**
In order to be part of *General Embedded C Libraries Ecosystem* this module must be placed in following path: 
```
root/middleware/parameters/parameters/"module_space"
```

 ## **API**
| API Functions | Description | Prototype |
| --- | ----------- | ----- |
| **par_init** 					| Initialization of parameters module 				| par_status_t par_init(void) |****
| **par_deinit** 				| De-initialization of parameters module 			| par_status_t par_deinit(void) |****
| **par_is_init** 				| Get initialization flag 							| bool par_is_init(void) |

 ## **Setting parameter value API**
| API Functions | Description | Prototype |
| --- | ----------- | ----- |
| **par_set** 					| Set parameter value 								| par_status_t par_set(const par_num_t par_num, const void *p_val) |
| **par_set_by_id** 			| Set parameter value by ID 						| par_status_t par_set_by_id(const uint16_t id, const void * p_val) |
| **par_set_u8** 				| Set u8 parameter value 							| par_status_t par_set_u8(const par_num_t par_num, const uint8_t val) |
| **par_set_i8** 				| Set i8 parameter value 							| par_status_t par_set_i8(const par_num_t par_num, const int8_t val) |
| **par_set_u16** 				| Set u16 parameter value 							| par_status_t par_set_u16(const par_num_t par_num, const uint16_t val) |
| **par_set_i16** 				| Set i16 parameter value 							| par_status_t par_set_i16(const par_num_t par_num, const int16_t val) |
| **par_set_u32** 				| Set u32 parameter value 							| par_status_t par_set_u32(const par_num_t par_num, const uint32_t val) |
| **par_set_i32** 				| Set i32 parameter value 							| par_status_t par_set_i32(const par_num_t par_num, const int32_t val) |
| **par_set_f32** 				| Set f32 parameter value 							| par_status_t par_set_f32(const par_num_t par_num, const float32_t val) |
| **par_set_to_default** 		| Set parameter to default value 					| par_status_t par_set_to_default (const par_num_t par_num) |
| **par_set_all_to_default** 	| Set all parameters to default value 				| par_status_t par_set_all_to_default (void) |
| **PAR_SET** 					| Set generic parameters value 						| #define PAR_SET(par_num, value) |

 ## **Getting parameter value API**
| API Functions | Description | Prototype |
| --- | ----------- | ----- |
| **par_get** 					| Get parameter value 								| par_status_t par_get (const par_num_t par_num, void *const p_val)|
| **par_get_id** 				| Get parameter ID number 							| par_status_t par_get_id(const par_num_t par_num, uint16_t *const p_id) |
| **par_get_u8** 				| Get u8 parameter value  							| uint8_t par_get_u8(const par_num_t par_num) |
| **par_get_i8** 				| Get i8 parameter value  							| uint8_t par_get_i8(const par_num_t par_num) |
| **par_get_u16** 				| Get u16 parameter value  							| uint16_t par_get_u16(const par_num_t par_num) |
| **par_get_i16** 				| Get i16 parameter value  							| uint16_t par_get_i16(const par_num_t par_num) |
| **par_get_u32** 				| Get u32 parameter value  							| uint32_t par_get_u32(const par_num_t par_num) |
| **par_get_i32** 				| Get i32 parameter value  							| uint32_t par_get_i32(const par_num_t par_num) |
| **par_get_f32** 				| Get f32 parameter value  							| float32_t par_get_f32(const par_num_t par_num) |
| **par_get_default** 			| Get default parameter value  						| par_status_t par_get_default(const par_num_t par_num, void * const p_val) |
| **par_is_changed** 			| Is parameter value changed from default			| bool par_is_changed(const par_num_t par_num) |
| **PAR_GET** 					| Get generic parameters value 						| #define PAR_GET(par_num, dest) |

 ## **Parameter configurations API**
| API Functions | Description | Prototype |
| --- | ----------- | ----- |
| **par_get_config** 			| Get parameter configurations 						| const par_cfg_t * par_get_config(const par_num_t par_num) |
| **par_get_name** 				| Get parameter name 								| const char * par_get_name(const par_num_t par_num) |
| **par_get_range** 			| Get parameter range 								| par_range_t par_get_range(const par_num_t par_num) |
| **par_get_unit** 				| Get parameter unit 								| const char * par_get_unit(const par_num_t par_num) |
| **par_get_desc** 				| Get parameter describtion							| const char * par_get_desc(const par_num_t par_num) |
| **par_get_type** 				| Get parameter data type							| par_type_list_t par_get_type(const par_num_t par_num) |
| **par_get_access**			| Get parameter access								| par_access_t par_get_access(const par_num_t par_num) |
| **par_is_persistant**			| Get parameter persistance							| bool par_is_persistant(const par_num_t par_num) |
| **par_get_num_by_id** 		| Get parameter number (enumeration) by its ID 		| par_status_t par_get_num_by_id(const uint16_t id, par_num_t *const p_par_num) |
| **par_get_id_by_num** 		| Get parameter ID by number (enumeration) 			| par_status_t par_get_id_by_num(const par_num_t par_num, uint16_t * const p_id) |

 ## **Parameter NVM storage API**
| API Functions | Description | Prototype |
| --- | ----------- | ----- |
| **par_set_n_save** 	| Set and store parameter to NVM 					| par_status_t par_set_n_save(const par_num_t par_num, const void * p_val) |
| **par_save_all** 		| Store all parameters to NVM 						| par_status_t par_save_all(void) |
| **par_save** 			| Store single parameter 							| par_status_t par_save(const par_num_t par_num) |
| **par_save_by_id** 	| Store single parameter by ID 						| par_status_t par_save_by_id(const uint16_t par_id) |
| **par_save_clean** 	| Re-Write complete NVM memory 						| par_status_t par_save_clean(void) |

 ## **Parameter registrations API**
| API Functions | Description | Prototype |
| --- | ----------- | ----- |
| **par_register_on_change_cb** 		| Register on value change callback			| par_status_t par_register_on_change_cb(const par_on_change_cb_t * const cb) |
| **par_register_validation** 			| Register value validation callback		| par_status_t par_register_validation(const par_validation_t * const validation) |

## Usage

**Put all user code between sections: USER CODE BEGIN & USER CODE END!**

1. Copy template files to root directory of module.
2. List names of all wanted parameters inside **par_cfg.h** file

```C
/**
 * 	List of device parameters
 *
 * @note 	User shall provide parameter name here as it would be using
 * 			later inside code.
 *
 * @note 	User shall change code only inside section of "USER_CODE_BEGIN"
 * 			ans "USER_CODE_END".
 */
typedef enum
{
	// USER CODE START...

	ePAR_TEST_U8 = 0,
	ePAR_TEST_I8,

	ePAR_TEST_U16,
	ePAR_TEST_I16,

	ePAR_TEST_U32,
	ePAR_TEST_I32,

	ePAR_TEST_F32,

	// USER CODE END...

	ePAR_NUM_OF
} par_num_t;
```

3. Change parameter configuration table inside **par_cfg.c** file. It is recommended to use designated initializers.

```C
/**
 *	Parameters definitions
 *
 *	@brief
 *
 *	Each defined parameter has following properties:
 *
 *		i)      Parameter ID:   Unique parameter identification number. ID shall not be duplicated.
 *		ii)     Name:           Parameter name. Max. length of 32 chars.
 *		iii)    Min:            Parameter minimum value. Min value must be less than max value.
 *		iv)     Max:            Parameter maximum value. Max value must be more than min value.
 *		v)      Def:            Parameter default value. Default value must lie between interval: [min, max]
 *		vi)     Unit:           In case parameter shows physical value. Max. length of 32 chars.
 *		vii)    Data type:      Parameter data type. Supported types: uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t and float32_t
 *		viii)   Access:         Access type visible from external device such as PC. Either ReadWrite or ReadOnly.
 *		ix)     Persistence:    Tells if parameter value is being written into NVM.
 *
 *	@note	User shall fill up wanted parameter definitions!
 */
static const par_cfg_t g_par_table[ePAR_NUM_OF] =
{

	// USER CODE BEGIN...

	// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	//                   ID         Name                  Min              Max           Def                 Unit         Data type               PC Access                 Persistent		     Description 
	// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

	[ePAR_TEST_U8] = {	.id = 0,  .name = "Test_u8",  .min.u8 = 0,      .max.u8 = 10,   .def.u8 = 8,       .unit = "n/a", .type = ePAR_TYPE_U8, .access = ePAR_ACCESS_RW,  .persistant = true, .desc = "Test parameter U8" },
	[ePAR_TEST_I8] = {	.id = 1,  .name = "Test_i8",  .min.i8 = -10,    .max.i8 = 100,  .def.i8 = -8,      .unit = "n/a", .type = ePAR_TYPE_I8, .access = ePAR_ACCESS_RW,  .persistant = true, .desc = "Test parameter" },

	[ePAR_TEST_U16] = {	.id = 2,  .name = "Test_u16",  .min.u16 = 0,    .max.u16 = 10,  .def.u16 = 3,      .unit = "n/a", .type = ePAR_TYPE_U16, .access = ePAR_ACCESS_RW, .persistant = true, .desc = "Test parameter U16"},
	[ePAR_TEST_I16] = {	.id = 3,  .name = "Test_i16",  .min.i16 = -10,  .max.i16 = 100, .def.i16 = -5,     .unit = "n/a", .type = ePAR_TYPE_I16, .access = ePAR_ACCESS_RW, .persistant = true, .desc = "Test parameter I16"},

	[ePAR_TEST_U32] = {	.id = 4,  .name = "Test_u32",  .min.u32 = 0,    .max.u32 = 10,  .def.u32 = 10,     .unit = "n/a", .type = ePAR_TYPE_U32, .access = ePAR_ACCESS_RW, .persistant = true, .desc = "Test parameter U32" },
	[ePAR_TEST_I32] = {	.id = 5,  .name = "Test_i32",  .min.i32 = -10,  .max.i32 = 100, .def.i32 = -10,    .unit = "n/a", .type = ePAR_TYPE_I32, .access = ePAR_ACCESS_RW, .persistant = true, .desc = "Test parameter I32" },

	[ePAR_TEST_F32] = {	.id = 6,  .name = "Test_f32",  .min.f32 = -10,  .max.f32 = 100, .def.f32 = -1.123, .unit = "n/a", .type = ePAR_TYPE_F32, .access = ePAR_ACCESS_RW, .persistant = true, .desc = "Test parameter F32" },

	// ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


	// USER CODE END...
};
```

4. Set-up all configurations options inside **par_cfg.h** file (such as using mutex, using NVM, ...)

| Configuration | Description |
| --- | --- |
| **PAR_CFG_MUTEX_EN** 			| Enable/Disable multiple access protection. |
| **PAR_CFG_NVM_EN** 			| Enable/Disable usage of NVM for persistant parameters. |
| **PAR_CFG_NVM_REGION** 		| Select NVM region for Device Parameter storage space. | 
| **PAR_CFG_DEBUG_EN** 			| Enable/Disable debugging mode. | 
| **PAR_CFG_ASSERT_EN** 		| Enable/Disable asserts. Shall be disabled in release build!  | 
| **PAR_DBG_PRINT** 			| Definition of debug print. | 
| **PAR_ASSERT** 				| Definition of assert. | 

5. Call **par_init()** function

```C
// Init parameters
if ( ePAR_OK != par_init())
{
    PROJECT_CONFIG_ASSERT( 0 );
}
```
**NOTICE: NVM module will be initialized as a part of Device Parameters initialization routine in case of usage (*PAR_CFG_NVM_EN = 1*)!**

6. Setting/Getting parameter value

```C
// Set battery voltage & sytem current
// NOTICE: When using "par_set" don't forget to always CAST to appropriate data type!
(void) par_set( ePAR_BAT_VOLTAGE, (float32_t*) &g_pwr_data.bat.voltage_filt );
(void) par_set( ePAR_SYS_CURRENT, (float32_t*) &g_pwr_data.inp.sys_cur );

// Or equivalent to "par_set"
PAR_SET( ePAR_BAT_VOLTAGE, g_pwr_data.bat.voltage_filt );
PAR_SET( ePAR_SYS_CURRENT, g_pwr_data.inp.sys_cur );

// Set and save parameter 
if ( ePAR_OK != par_set_n_save( ePAR_P1_10, (uint32_t) &p1_10_val ))
{
	// Operation error...
	// Further actions here...
}

// Get battery voltage & sytem current
// NOTICE: When using "par_get" don't forget to always CAST to appropriate data type!
(void) par_get( ePAR_BAT_VOLTAGE, (float32_t*) &g_pwr_data.bat.voltage_filt );
(void) par_get( ePAR_SYS_CURRENT, (float32_t*) &g_pwr_data.inp.sys_cur );

// Or equivalent to "par_get"
PAR_GET( ePAR_BAT_VOLTAGE, g_pwr_data.bat.voltage_filt );
PAR_GET( ePAR_SYS_CURRENT, g_pwr_data.inp.sys_cur );
```

Setting direct value to parameter:

```C
par_set( ePAR_BAT_VOLTAGE, (float32_t*) &(float32_t){ 1.1234f} );

// Or equivalent using "PAR_SET"
// NOTE: When putting direct numbers using "PAR_SET" always cast to appropriate data type! 
PAR_SET( ePAR_BAT_VOLTAGE, (float32_t) 1.1234f );	
```

7. Store to NVM

```C
// Store all paramters to NVM
if ( ePAR_OK != par_save_all())
{
	// Storing to NVM error...
	// Further actions here...
}
```

8. On-change callback usage

```C
////////////////////////////////////////////////////////////////////////////////
/**
*        Parameter value change callback
*
* @param[out]   par_num - Parameter number
* @param[out]   new_val - Parameter new value
* @param[out]   new_val - Parameter old value
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
void par_on_change_cb1(const par_num_t par_num, const par_type_t new_val, const par_type_t old_val)
{
    cli_printf("Parameter %d change from %d to %d", par_num, old_val.u8, new_val.u8 );
}

// Define parameter on change callback for "ePAR_CH1_TEST_MODE_EN" parameter
PAR_DEFINE_ON_CHANGE_CB( test_par_cb, ePAR_CH1_TEST_MODE_EN, par_on_change_cb1);

// Register at init phase
@init
{
	if ( ePAR_OK != par_register_on_change_cb( &test_par_cb ))
	{
		// Operation error...
		// Further actions here...
	}
}
```

9. Parameter value validation usage

```C
////////////////////////////////////////////////////////////////////////////////
/**
*        Validate parameter value
*
* @param[in]   	par_num - Parameter number
* @param[in]	val 	- Parameter new value
* @return       true when value valid
*/
////////////////////////////////////////////////////////////////////////////////
static bool validate_par_value(const par_num_t par_num, const par_type_t val)
{
    UNUSED(par_num);

	// Prevent parameter to take exact -10 value
    if ( val.i8 == -10 )   	return false;
    else                    return true;
}

// Define parameter validation for "ePAR_CH1_REF_VAL" parameter
PAR_DEFINE_VALIDATION( par_validate, ePAR_CH1_REF_VAL, validate_par_value);

// Register at init phase
@init
{
	if ( ePAR_OK != par_register_validation( &par_validate ))
	{
		// Operation error...
		// Further actions here...
	}
}
```


