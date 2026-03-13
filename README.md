[TOC]

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

### **1. NVM Module**
In case of using NVM module *PAR_CFG_NVM_EN = 1*, then [NVM module](https://github.com/GeneralEmbeddedCLibraries/nvm) must pe part of project. 
NVM module must take following path:`"root/middleware/nvm/nvm/src/nvm.h"`

### **2. Platform adaptation layer**

This package separates the core parameter logic from platform-specific integration.

Platform-dependent configuration and hooks are provided through the `port/` layer:

* `port/par_cfg_port.h` – platform configuration bridge
* `port/par_if_port.c` – platform interface backend
* `port/par_atomic_port.h` – platform atomic backend

This keeps the core module portable while allowing integration with RTOS, mutex, logging, assertion, and atomic services provided by the target platform.

> Note: `parameters/src/par_cfg.h` includes `par_cfg_port.h` unconditionally.
> You must provide this header in your project include path.
> If no platform override is required, provide an empty stub `par_cfg_port.h` with include guard.

### **3. Atomic backend configuration**

The module requires an atomic backend for parameter value access.

By default, it uses the C11 atomic backend. If your compiler does not provide usable C11 atomics, or if your platform already provides its own atomic API, you can switch the module to a port-specific backend through `par_atomic_port.h`.

#### When to use `par_atomic_port.h`

Use `par_atomic_port.h` when:

- the compiler does not fully support `<stdatomic.h>`
- the target platform already provides atomic primitives
- you want the parameter module to use the RTOS or platform-native atomic implementation

#### How to enable `par_atomic_port.h`

Atomic backend selection is controlled by `PAR_ATOMIC_BACKEND` in `parameters/src/par_atomic.h`.

Available options:

```c
#define PAR_ATOMIC_BACKEND_C11   1
#define PAR_ATOMIC_BACKEND_PORT  2
```

To use the port backend, define:

```c
#define PAR_ATOMIC_BACKEND PAR_ATOMIC_BACKEND_PORT
```

After that, `par_atomic.h` will include `par_atomic_port.h` and use the port-provided atomic types and helpers.

#### Notes

* `par_atomic_port.h` must provide all atomic types and operations required by `par_atomic.h`
* `float32_t` should be stored and loaded by preserving its raw bit representation, not by numeric cast
* make sure the underlying atomic storage type matches the size of `float`
* keep all platform-specific atomic adaptation inside `par_atomic_port.h` so the core parameter code does not need to change

## **Limitations**
 - **Heap Usage:** The module uses malloc during par_init() to allocate RAM space for the parameters based on the configuration table. Ensure your heap is sufficiently sized.
 - **Alignment:** Address offsets are calculated based on 4-byte (32-bit) alignment to satisfy most ARM Cortex-M requirements.
 - **Flat ID Space:** Parameter IDs must be unique across the entire table to ensure NVM consistency.
 - **Execution Time:** If many callbacks are chained to a single parameter, the par_set execution time will increase accordingly.
 - **Hash-Based ID Lookup:** ID-based lookup is optimized for runtime speed and rejects hash collisions during initialization. See the ID lookup section below.

## **General Embedded C Libraries Ecosystem**
In order to be part of *General Embedded C Libraries Ecosystem* this module must be placed in following path: `root/middleware/parameters/parameters/"module_space"`

## **Package structure**

The package is split into three layers.

### Core layer

Portable parameter logic is implemented under: `parameters/src/`

This layer contains the core implementation of:

* parameter storage and typed access
* configuration lookup
* validation and callbacks
* ID lookup
* NVM integration
* atomic abstraction
* configuration abstraction

### Port layer

Platform-specific integration is implemented under:`port/`

This layer contains:

* `par_cfg_port.h` – platform configuration bridge
* `par_if_port.c` – platform-specific low-level interface
* `par_atomic_port.h` – platform-specific atomic backend

This separation keeps the core module portable while isolating platform and RTOS dependencies.

### Template layer

Reference templates are provided under: `parameters/template/`

This layer currently contains:

* `par_cfg_port.htmp` – port bridge header template

Template files are used as:

* baseline references for keeping generated or manually maintained files aligned with upstream style
* starting points when creating new `par_def.h` / `par_def.c` / `par_table.def` / `par_cfg_port.h` in other projects

Template files are not compiled directly by the runtime library.

## `par_table.def` and X-Macro workflow

The parameter table can be maintained through an **X-Macro single-source definition file**: `par_table.def`.

This file acts as the **single source of truth** for all parameter definitions.

Typical workflow:

* define all parameters once in `par_table.def`
* automatically expand:

  * `par_num_t` enumeration
  * compile-time validation checks
  * `g_par_table[]` configuration table

This avoids duplicating parameter metadata across multiple files and prevents inconsistencies between enumeration definitions and parameter table entries.

---

### Why `par_table.def` is used

Without a single-source definition file, the same parameter information would normally be repeated in multiple places:

* enum definition
* parameter table definition
* optional validation or helper structures

This approach is error-prone because parameters can easily be added in one location but forgotten in another.

Using `par_table.def` solves this by keeping **all parameter metadata in one place** and reusing it through macro expansion.

---

### X-Macro expansion model

Each entry in `par_table.def` is written as a macro invocation describing one parameter.

Example:

```c
PAR_ITEM_U8( PAR_CH1_CTRL, 0, "Ch1 Control", 0U, 2U, 2U, NULL, ePAR_ACCESS_RW, false, "Channel 1 control")
```

The same file can then be included multiple times with different macro definitions.

Conceptually:

* one expansion generates the enum
* one expansion generates compile-time validation checks
* one expansion generates `g_par_table[]`

---

### Example expansions

#### Enum expansion

`par_table.def` can be expanded into:

```c
typedef enum
{
    ePAR_CH1_CTRL = 0,
    ...
    ePAR_NUM_OF
} par_num_t;
```

#### Compile-time validation expansion

The same item can generate static assertions such as:

```sh
min <= max
def >= min
def <= max
```

#### Table initialization expansion

The same item can also generate the designated initializer inside the configuration table:

```c
static const par_cfg_t g_par_table[ePAR_NUM_OF] =
{
    ...
};
```

This X-Macro pattern allows:

* **single parameter definition**
* **multiple generated outputs**
* **no duplicated metadata**

---

### Why this improves robustness

The X-Macro + `par_table.def` approach improves the package in several ways:

* **consistency** – enum, validation, and table initialization are generated from one source
* **maintainability** – parameters are added or modified in only one place
* **early error detection** – configuration errors can be caught during compilation

For embedded systems with large parameter sets, this significantly reduces configuration drift and integration errors.

---

## **Validation model**

The parameter module performs validation at two stages:

1. **Compile-time validation**
2. **Runtime validation**

This layered approach catches most configuration errors during compilation while still allowing dynamic checks where required.

---

### Compile-time checks

For **integer parameter types**

```sh
U8, I8, U16, I16, U32, I32
```

the module performs compile-time validation using static assertions generated from `par_table.def`.

Typical checks include:

* `min <= max`
* `def >= min`
* `def <= max`

These checks are expanded through the X-Macro validation layer located in: `par_table_xmacro_check.h`

Example generated assertion:

```c
PAR_STATIC_ASSERT(ePAR_CH1_CTRL_def_le_max, ((def_) <= (max_)));
```

If the condition evaluates to false, compilation fails immediately.

This guarantees that invalid parameter configurations cannot pass the build stage.

---

### Example compile-time error

If the default value exceeds the allowed range:

```c
PAR_ITEM_U8( PAR_CH1_CTRL, 0, "Ch1 Control", 0U, 2U, 2U, NULL, ePAR_ACCESS_RW, false, "Channel 1 control")
```

Compilation will fail with an error similar to:

```sh
error: size of array '_static_assert_ePAR_CH1_CTRL_def_le_max' is negative
```

This indicates that the condition `def <= max` failed.

---

### Runtime checks

Some validation must be performed at runtime.

Runtime validation includes:

* floating-point (`F32`) range validation
* parameter `name` must not be `NULL`
* parameter `desc` must not be `NULL`
* `desc` must not contain `,`

These checks are executed during module initialization in: `par_check_table_validity()` located in `par.c`.

If a runtime validation fails, module initialization will fail and an error will be reported.

---

### Why `F32` validation is runtime-only

Floating-point values are not ideal for strict compile-time validation in embedded toolchains because:

* floating-point constant expressions are handled inconsistently across compilers
* static assertions involving floating-point comparisons are less portable
* rounding behaviour can vary between toolchains

To maintain portability and predictable builds, `F32` validation is performed **at runtime instead of compile time**.

---

### Validation summary

| Validation                  | Stage        | Applies to     |
| --------------------------- | ------------ | -------------- |
| `min <= max`                | compile-time | integer types  |
| `def >= min`                | compile-time | integer types  |
| `def <= max`                | compile-time | integer types  |
| float range check           | runtime      | `F32`          |
| `name != NULL`              | runtime      | all parameters |
| `desc != NULL`              | runtime      | all parameters |
| `desc` must not contain `,` | runtime      | all parameters |

## **Configuration model**

The package uses a two-level configuration model.

### Core configuration

Core configuration defaults are defined in: `parameters/src/par_cfg.h`

This file provides default values for options such as:

* `PAR_CFG_NVM_EN`
* `PAR_CFG_NVM_REGION`
* `PAR_CFG_TABLE_ID_CHECK_EN`
* `PAR_CFG_DEBUG_EN`
* `PAR_CFG_ASSERT_EN`
* `PAR_CFG_MUTEX_EN`
* `PAR_CFG_MUTEX_TIMEOUT_MS`
* `PAR_CFG_IF_PORT_EN`
* `PAR_CFG_PORT_HOOK_EN`

### Platform bridge

Platform-specific overrides are provided in: `port/par_cfg_port.h`

This file maps platform or build-system configuration symbols to `PAR_CFG_*` options.

`par_cfg_port.h` is mandatory for build because it is directly included by `parameters/src/par_cfg.h`.
If you do not need overrides, keep a minimal empty file:

```c
#ifndef _PAR_CFG_PORT_H_
#define _PAR_CFG_PORT_H_
/* Optional platform overrides */
#endif
```

### Why this split exists

This design keeps the core implementation independent of any single build system or RTOS, while still allowing package-level integration through a dedicated platform bridge.

---

## **Port hooks**

When `PAR_CFG_PORT_HOOK_EN = 1`, the module uses platform-provided hooks for:

* logging
* assertions
* compile-time assertions

The following hooks may be provided by the port layer:

* `PAR_PORT_LOG(...)`
* `PAR_PORT_ASSERT(x)`
* `PAR_PORT_STATIC_ASSERT(name, expn)`

This allows the module to integrate with the native debug and assert infrastructure of the target platform.

---

## **Interface backend**

The low-level interface layer is implemented in `parameters/src/par_if.c`.

When `PAR_CFG_IF_PORT_EN = 1`, the module uses the platform-specific backend provided by: `port/par_if_port.c`

This backend is responsible for platform-dependent services such as:

* initialization
* mutex handling
* optional table hash calculation

This keeps the public parameter logic independent from the RTOS or platform implementation.


## **Parameter identification**

Each parameter defined in the configuration table contains two identifiers:

- **par_num** – internal parameter index (enumeration)
- **id** – external parameter identifier

These identifiers serve different purposes and are intentionally separated.

### Internal identifier (`par_num`)

`par_num` is the enumeration defined in `par_cfg.h` and represents the **internal index of a parameter**.

It is primarily used inside firmware code and by the parameter module APIs.

Example:

```c
par_set(ePAR_TEST_U8, &value);
```

Characteristics:

* used as an **index into the parameter configuration table**
* provides **fast and type-safe access** inside firmware
* may change if parameters are reordered or new parameters are added

Because of this, `par_num` should generally be used **only inside firmware code**.

### External identifier (`id`)

Each parameter also defines a unique **ID** in the configuration table:

```c
[ePAR_TEST_U8] = {
    .id = 0,
    ...
}
```

The **ID is intended for external access** to parameters.

Typical use cases include:

* CLI commands
* PC configuration tools
* communication protocols (UART / CAN / etc.)
* parameter import/export
* diagnostics or logging

To support these use cases, the module provides APIs that operate using parameter IDs:

* `par_set_by_id()`
* `par_get_by_id()`
* `par_save_by_id()`

Internally, these functions resolve the ID to the corresponding `par_num` before accessing the parameter.

### Design rationale

Separating internal and external identifiers provides several advantages:

* **Efficient internal access** through `par_num`
* **Stable external interface** through `id`
* the ability to **reorder or extend parameters without breaking external tools**

External systems should always reference parameters by **ID**, while firmware code should typically use **par_num**.

### ID allocation guidelines

Parameter IDs must be **unique across the entire parameter table**.

IDs do not need to be sequential, but it is recommended to group them by subsystem for clarity.

Example allocation:

| ID Range | Subsystem         |
| -------- | ----------------- |
| 0–99     | Channel 1         |
| 100–199  | Channel 2         |
| 200–299  | Channel 3         |
| 300–399  | Channel 4         |
| 10000+   | System parameters |

This approach simplifies integration with external tools and communication protocols.

## ID lookup using hash map

To improve parameter lookup by **ID**, the module builds a runtime hash map during `par_init()`.

This hash map is used by APIs such as:

- `par_get_num_by_id()`
- `par_set_by_id()`
- `par_get_by_id()`
- `par_save_by_id()`

Instead of scanning the full parameter table for every ID lookup, the module hashes the parameter ID and directly maps it to the corresponding `par_num`.

### Why a hash map is used

The parameter module supports two access paths:

- **`par_num`** for internal firmware access
- **`id`** for external access such as CLI, PC tools, and communication protocols

Internal access by `par_num` is naturally efficient because it uses the parameter enumeration as a direct table index.

External access by **ID** is different. Since IDs are user-defined and do not need to be sequential, converting an ID back to `par_num` would otherwise require a linear search through the full parameter table.

A hash map avoids that cost and provides near constant-time lookup for ID-based APIs.

### How it works

During `par_init()`, the module:

1. walks through the parameter configuration table
2. hashes each parameter ID into a bucket index
3. stores the mapping: `ID -> par_num`

Later, when an external API uses an ID, the module:

1. hashes the requested ID
2. checks the corresponding bucket
3. returns the mapped `par_num`
4. performs the actual parameter operation internally

Conceptually:

```sh
External ID
   |
   v
 hash(id)
   |
   v
 hash bucket
   |
   v
 par_num
   |
   v
 internal parameter API
```

### Why this is better than linear search

Compared to a linear scan of the parameter table, the hash map provides:

* lower lookup latency
* predictable runtime
* better scalability as the number of parameters grows
* lower overhead for frequently used ID-based APIs

This is especially useful when parameters are accessed repeatedly from:

* CLI commands
* host tools
* diagnostic services
* communication stacks

### Why this is preferred over binary search

Binary search would require the parameter table to be sorted by ID or an additional sorted lookup table.

That introduces extra maintenance constraints and reduces flexibility in parameter definition order.

The hash-based approach keeps the configuration table simple and preserves fast ID lookup without requiring sorted IDs.

### Collision policy

This implementation uses a strict one-entry-per-bucket hash map.

That means:

* duplicate IDs are rejected
* hash collisions are also rejected during initialization

If two different IDs map to the same hash bucket, initialization fails and a debug message is printed.

This design keeps runtime lookup logic simple, fast, and deterministic.

### What to do if a hash collision is reported

If initialization prints a message such as:

```sh
ERR, Hash collision: ID X conflicts with ID Y at bucket Z!
ERR, Please regenerate IDs or adjust hash parameters.
```

then two different parameter IDs were mapped to the same hash bucket.

Recommended actions:

1. change one or more parameter IDs so they no longer collide
2. keep subsystem-based ID allocation, but avoid problematic values

In practice, the preferred solution is to **regenerate or reassign the conflicting IDs**.

### Recommended usage

When defining parameter IDs manually:

* keep IDs unique across the entire table
* keep IDs stable across firmware versions
* group IDs by subsystem when possible
* avoid changing IDs unless external compatibility is intentionally broken

### Notes for generated parameter tables

In future workflows, parameter definitions and IDs can be generated by script tools.

When IDs are generated automatically, collisions can be checked during generation, which means:

* duplicate IDs can be prevented before build time
* hash collisions can be avoided before firmware is compiled
* the runtime hash map remains simple and fast
* manual ID maintenance is reduced

With script-generated parameter tables, hash collision problems should normally not occur.

### Design tradeoff

This implementation intentionally favors:

* **fast lookup**
* **simple runtime logic**
* **deterministic behavior**

over:

* runtime collision resolution
* more complex lookup structures

Compared to alternatives such as linear search, linear probing, or maintaining a sorted structure for binary search, this approach gives better lookup performance for normal operation.

The main tradeoff is that a rare hash collision must be resolved by reassigning IDs or regenerating the parameter table. For this module, that tradeoff is considered better than paying additional runtime cost or complexity on every ID lookup.


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
| **par_set_u8_fast** 			| Set u8 parameter value fast						| par_status_t par_set_u8_fast(const par_num_t par_num, const uint8_t val) |
| **par_set_i8_fast** 			| Set i8 parameter value fast 						| par_status_t par_set_i8_fast(const par_num_t par_num, const int8_t val) |
| **par_set_u16_fast** 			| Set u16 parameter value fast						| par_status_t par_set_u16_fast(const par_num_t par_num, const uint16_t val) |
| **par_set_i16_fast** 			| Set i16 parameter value fast						| par_status_t par_set_i16_fast(const par_num_t par_num, const int16_t val) |
| **par_set_u32_fast** 			| Set u32 parameter value fast						| par_status_t par_set_u32_fast(const par_num_t par_num, const uint32_t val) |
| **par_set_i32_fast** 			| Set i32 parameter value fast						| par_status_t par_set_i32_fast(const par_num_t par_num, const int32_t val) |
| **par_set_f32_fast** 			| Set f32 parameter value fast						| par_status_t par_set_f32_fast(const par_num_t par_num, const float32_t val) |
| **par_set_to_default** 		| Set parameter to default value 					| par_status_t par_set_to_default (const par_num_t par_num) |
| **par_set_all_to_default** 	| Set all parameters to default value 				| par_status_t par_set_all_to_default (void) |
| **PAR_SET** 					| Set generic parameters value 						| #define PAR_SET(par_num, value) |

 ## **Getting parameter value API**
| API Functions | Description | Prototype |
| --- | ----------- | ----- |
| **par_get** 					| Get parameter value 								| par_status_t par_get (const par_num_t par_num, void *const p_val)|
| **par_get_id** 				| Get parameter ID number 							| par_status_t par_get_id(const par_num_t par_num, uint16_t *const p_id) |
| **par_get_u8** 				| Get u8 parameter value  							| uint8_t par_get_u8(const par_num_t par_num) |
| **par_get_i8** 				| Get i8 parameter value  							| int8_t par_get_i8(const par_num_t par_num) |
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

## **Usage**

### 1. Define parameters in `par_table.def`

All parameters should be defined in the single-source definition file:`par_table.def`

Example:

```c
/*
 * Parameter single-source list for X-Macro expansion.
 *
 * This file is intentionally included multiple times.
 * Do not add include guards and do not place executable code here.
 *
 * Supported item macros:
 *   PAR_ITEM_U8 (enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)
 *   PAR_ITEM_U16(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)
 *   PAR_ITEM_U32(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)
 *   PAR_ITEM_I8 (enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)
 *   PAR_ITEM_I16(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)
 *   PAR_ITEM_I32(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)
 *   PAR_ITEM_F32(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)
 *
 * Field description:
 *   enum_   - Parameter enum index (used as designated initializer index)
 *   id_     - Unique external parameter ID
 *   name_   - Parameter display name
 *   min_    - Minimum allowed value
 *   max_    - Maximum allowed value
 *   def_    - Default value, must be within [min_, max_]
 *   unit_   - Engineering unit string, or NULL if not applicable
 *   access_ - External access type: ePAR_ACCESS_RO / ePAR_ACCESS_RW
 *   pers_   - Persistence flag: true if stored to NVM, otherwise false
 *   desc_   - Human-readable description
 *
 * Formatting note:
 *   Keep items grouped by subsystem/channel and aligned for readability.
 */


/* ============================================================================================================================================================= */
/*  enum_                            id_     name_                           min_        max_        def_        unit_   access_          pers_   desc_ */
/* ============================================================================================================================================================= */


/* ============================================================================================================================= */
/*  CHANNEL 1                                                                                                                     */
/* ============================================================================================================================= */

/* Channel 1 control */
PAR_ITEM_U8 (ePAR_CH1_CTRL,         0,      "Ch1 Control",                  0U,         2U,         2U,         NULL,   ePAR_ACCESS_RW,  false,  "Channel 1 control: 0-Normal | 1-Short | 2-Open. NOTE: If status is <Not Connected> then it will force to Open!")
PAR_ITEM_U8 (ePAR_CH1_AFE_MEAS_EN,  1,      "Ch1 AFE Measurement Control",  0U,         1U,         1U,         NULL,   ePAR_ACCESS_RW,  true,   "Channel 1 control AFE measurement usage for calculations: 0-Disable | 1-Enable")
PAR_ITEM_U8 (ePAR_CH1_TEST_MODE_EN, 2,      "Ch1 Test Mode",                0U,         1U,         0U,         NULL,   ePAR_ACCESS_RW,  false,  "Channel 1 test mode enable: 0-Disable | 1-Enable. NOTE: This will put channel to Normal state - ignoring detection logic!")
PAR_ITEM_U8 (ePAR_CH1_REF_SEL,      3,      "Ch1 Reference Selection",      0U,         4U,         0U,         NULL,   ePAR_ACCESS_RW,  true,   "Channel 1 reference selection: 0-Temperature | 1-Resistance | 2-Vout | 3-Isink | 4-Vset")
PAR_ITEM_F32(ePAR_CH1_REF_VAL,      4,      "Ch1 Reference Value",         -1E6f,       1E6f,       0.0f,       NULL,   ePAR_ACCESS_RW,  true,   "Channel 1 reference value based on control variable set")
```

This definition will be expanded through the X-Macro system to generate:

* `par_num_t` enumeration
* compile-time validation checks
* `g_par_table[]` configuration table

### 2. Configure the module

The main package configuration is defined in:`parameters/src/par_cfg.h`

Platform-specific overrides and hooks are provided in:`port/par_cfg_port.h`

| Configuration | Description |
| --- | --- |
| **PAR_CFG_NVM_EN**            | Enable/Disable usage of NVM for persistent parameters. |
| **PAR_CFG_NVM_REGION**        | Select NVM region for Device Parameter storage space. | 
| **PAR_CFG_DEBUG_EN**          | Enable/Disable debugging mode. |
| **PAR_CFG_ASSERT_EN**         | Enable/Disable asserts. Shall be disabled in release build!  | 
| **PAR_DBG_PRINT**             | Definition of debug print. |
| **PAR_ASSERT**                | Definition of assert. |
| **PAR_ATOMIC_BACKEND**        | Select atomic backend implementation. |
| **PAR_CFG_TABLE_ID_CHECK_EN** | Enable or disable parameter table unique ID checking for NVM compatibility workflows. |
| **PAR_CFG_MUTEX_EN**          | Enable or disable mutex protection.                                                   |
| **PAR_CFG_MUTEX_TIMEOUT_MS**  | Mutex timeout in milliseconds.                                                        |
| **PAR_CFG_IF_PORT_EN**        | Enable platform-specific `par_if` backend.                                            |
| **PAR_CFG_PORT_HOOK_EN**      | Enable platform log/assert hooks.                                                     |

### **3. Build integration**

The package build script collects source files from:

* package root
* `parameters/src/`

and adds the following include paths:

* package root
* `parameters/src/`
* `port/`

If you add or replace port-specific files, keep them under `port/` so they remain visible to the build system.

### **4. Call **par_init()** function**

```C
// Init parameters
if ( ePAR_OK != par_init())
{
    PROJECT_CONFIG_ASSERT( 0 );
}
```
**NOTICE: NVM module will be initialized as a part of Device Parameters initialization routine in case of usage (*PAR_CFG_NVM_EN = 1*)!**

### **5. Setting/Getting parameter value**

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

### **6.Normal and fast parameter setting API**
When choosing an API for setting parameter values, you must decide between the safe (normal) API and the fast API (with suffix *_fast*). The choice depends on whether your priority is data integrity and system observability or raw execution speed.

```C
// --- NORMAL API ---
// Use this for 90% of your code. It prevents errors and triggers callbacks.
par_set_f32( ePAR_TARGET_TEMP, 25.5f );

// --- FAST API ---
// Use this in high frequency control loops where every microsecond counts.
// WARNING: No safety checks are performed!
par_set_f32_fast( ePAR_MOTOR_PWM, 0.85f );
```

### **7. Store to NVM**

```C
// Store all paramters to NVM
if ( ePAR_OK != par_save_all())
{
	// Storing to NVM error...
	// Further actions here...
}
```

### **8. On-change callback usage**

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

### **9. Parameter value validation usage**

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
