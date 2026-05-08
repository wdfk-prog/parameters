from building import *
import rtconfig

cwd = GetCurrentDir()

src = Split('''
parameters/src/par.c
parameters/src/object/par_object.c
parameters/src/object/par_object_api.c
parameters/src/scalar/par_scalar_api.c
parameters/src/def/par_def.c
parameters/src/def/par_id_map_static.c
parameters/src/layout/par_layout.c
parameters/src/port/par_if.c
port/par_if_port.c
''')

if GetDepend('AUTOGEN_PM_USING_MSH_TOOL'):
    src += Split('''
    port/par_shell_tool.c
    ''')

path = [
    cwd,
    cwd + '/backend',
    cwd + '/port',
    cwd + '/parameters/include',
    cwd + '/parameters/src',
    cwd + '/parameters/src/def',
    cwd + '/parameters/src/detail',
    cwd + '/parameters/src/layout',
    cwd + '/parameters/src/nvm',
    cwd + '/parameters/src/nvm/backend',
    cwd + '/parameters/src/nvm/object',
    cwd + '/parameters/src/nvm/object/addr',
    cwd + '/parameters/src/nvm/object/store',
    cwd + '/parameters/src/nvm/scalar',
    cwd + '/parameters/src/nvm/scalar/layout',
    cwd + '/parameters/src/nvm/scalar/store',
    cwd + '/parameters/src/object',
    cwd + '/parameters/src/port',
    cwd + '/parameters/src/scalar',
    cwd + '/parameters/generated',
]

if GetDepend('AUTOGEN_PM_LAYOUT_SOURCE_SCRIPT'):
    src += Split('''
    parameters/generated/par_layout_static.c
    ''')

if GetDepend('AUTOGEN_PM_ENABLE_GENERATED_INFO'):
    src += Split('''
    parameters/generated/par_generated_info.c
    ''')

if GetDepend('AUTOGEN_PM_USING_NVM'):
    scalar_backend_needed = (
        GetDepend('AUTOGEN_PM_NVM_SCALAR') or
        (GetDepend('AUTOGEN_PM_NVM_OBJECT') and
         GetDepend('AUTOGEN_PM_NVM_OBJECT_STORE_SHARED'))
    )

    src += Split('''
    parameters/src/nvm/par_nvm.c
    parameters/src/nvm/par_nvm_table_id.c
    parameters/src/nvm/hash_32a.c
    ''')

    src += Split('''
    parameters/src/nvm/scalar/store/par_nvm_scalar_store.c
    ''')

    if GetDepend('AUTOGEN_PM_NVM_SCALAR'):
        src += Split('''
        parameters/src/nvm/scalar/par_nvm_scalar.c
        ''')

        if GetDepend('AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE'):
            src += Split('''
            parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_with_size.c
            ''')
        elif GetDepend('AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE'):
            src += Split('''
            parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_no_size.c
            ''')
        elif GetDepend('AUTOGEN_PM_NVM_RECORD_LAYOUT_COMPACT_PAYLOAD'):
            src += Split('''
            parameters/src/nvm/scalar/layout/par_nvm_layout_compact_payload.c
            ''')
        elif GetDepend('AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_PAYLOAD_ONLY'):
            src += Split('''
            parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_payload_only.c
            ''')
        elif GetDepend('AUTOGEN_PM_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY'):
            src += Split('''
            parameters/src/nvm/scalar/layout/par_nvm_layout_grouped_payload_only.c
            ''')
        else:
            src += Split('''
            parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_with_size.c
            ''')

    if GetDepend('AUTOGEN_PM_NVM_OBJECT'):
        src += Split('''
        parameters/src/nvm/object/par_nvm_object.c
        parameters/src/nvm/object/store/par_nvm_object_store_shared.c
        parameters/src/nvm/object/store/par_nvm_object_store_dedicated.c
        parameters/src/nvm/object/addr/par_nvm_object_addr_after_scalar.c
        parameters/src/nvm/object/addr/par_nvm_object_addr_fixed.c
        parameters/src/nvm/object/addr/par_nvm_object_addr_dedicated.c
        ''')

    if scalar_backend_needed and GetDepend('AUTOGEN_PM_USING_RTT_AT24CXX_BACKEND'):
        src += Split('''
        backend/par_store_backend_rtt_at24cxx.c
        ''')

    if scalar_backend_needed and GetDepend('AUTOGEN_PM_USING_FLASH_EE_BACKEND'):
        src += Split('''
        parameters/src/nvm/backend/par_store_backend_flash_ee.c
        ''')

        if GetDepend('AUTOGEN_PM_FLASH_EE_PORT_FAL'):
            src += Split('''
            backend/par_store_backend_flash_ee_fal.c
            ''')
        elif GetDepend('AUTOGEN_PM_FLASH_EE_PORT_NATIVE'):
            src += Split('''
            backend/par_store_backend_flash_ee_native.c
            ''')

group = DefineGroup(
    'autogen_parameter_manager',
    src,
    depend=['PKG_USING_AUTOGEN_PARAMETER_MANAGER'],
    CPPPATH=path,
)

Return('group')
