[中文](./csv-generator.zh-CN.md)

# CSV parameter generator

`tools/pargen.py` converts the editable CSV parameter table into generated artifacts used by the C build.

## Inputs

| File | Purpose |
| --- | --- |
| `schema/par_table.csv` | Parameter rows and metadata. |
| `schema/pargen.json` | ID range policy and generator options. |
| `schema/par_id_lock.json` | Stable mapping from parameter enum names to external IDs. |
| `template/*.htmp` / `template/*.deftmp` | Output templates. |

## Common command

```sh
python3 tools/pargen.py
```

Run it from the repository root after changing parameter rows, ID ranges, templates, or generator logic.

## CSV columns

| Column | Meaning |
| --- | --- |
| `group` | Logical group, also used for ID range policy. |
| `section` | Documentation or UI grouping. |
| `condition` | Optional compile-time condition for the row. |
| `enum` | Generated `par_num_t` enum symbol. |
| `id` | Stable external ID. |
| `type` | Scalar or object type. |
| `name` | Human-readable parameter name. |
| `min` / `max` | Scalar range or object length/capacity policy depending on type. |
| `default` | Default scalar value or object default encoding. |
| `unit` | Optional display unit. |
| `access` | Access mode, typically `RO` or `RW`. |
| `read_roles` / `write_roles` | Optional role-policy metadata. |
| `persistent` | Whether the parameter should be persisted when NVM is enabled. |
| `desc` | Human-readable description. |
| `comment` | Maintainer note not necessarily compiled into runtime metadata. |

## ID policy

Use `schema/pargen.json` to reserve ID ranges by group. Use `schema/par_id_lock.json` to keep existing enum-to-ID mappings stable across table edits.

Recommended rules:

- Never recycle an ID for a different semantic parameter unless old persistent records and external tools are intentionally invalidated.
- Add new parameters inside the assigned group range.
- Review diffs of `par_id_lock.json` during code review.
- Treat hash collisions as build-time errors that must be resolved by changing IDs or hash geometry.

## Generated outputs

Typical generated artifacts include:

- generated parameter definition data
- generated ID map data
- generated layout information for script-layout mode
- generated summary information for diagnostics or review

The exact file set depends on generator configuration and templates.

## Static layout mode

When using generated layout data, configure the build consistently:

```c
#define PAR_CFG_LAYOUT_SOURCE         PAR_CFG_LAYOUT_SCRIPT
#define PAR_CFG_LAYOUT_STATIC_INCLUDE "par_layout_static.h"
```

This mode makes persistent layout changes easier to review because offsets and capacities are generated deterministically.

## Change workflow

1. Edit `schema/par_table.csv`.
2. Run `python3 tools/pargen.py`.
3. Review generated source/header/json diffs.
4. Build the project.
5. Run generator tests when Python test dependencies are available.
6. For persistent parameters, review layout and ID compatibility before release.
