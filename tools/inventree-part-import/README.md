# inventree-part-import config

Version-controlled config for the third-party bulk importer
[`inventree-part-import`](https://github.com/30350n/inventree-part-import) (30350n), which pulls
LCSC supplier/MPN/datasheet/parameter/**price** data into our InvenTree. See the
**`inventree-parts`** skill for the full workflow (IPN scheme, placeholder-first import,
duplicate rules, alternates).

The tool reads its config from a machine-local dir
(`~/Library/Application Support/inventree_part_import/` on macOS —
`python -m inventree_part_import --show-config-dir`). These files are the source of truth;
**copy them into that dir** on a new machine.

## Files
- **`categories.yaml`** — maps LCSC leaf categories → our flat InvenTree tree
  (Passives / Semiconductors / Connectors / Crystals / Modules / Controls) via `_aliases`.
  We **alias, not create-on-the-fly**: our parts fall in a small stable set, and mirroring
  LCSC's deep taxonomy would bloat the tree. Add a new `_aliases` entry when a new LCSC
  category appears (the tool errors "failed to match category" on an unmapped *create*).
- **`config.yaml`** — currency/language/interactive settings.
- **`suppliers.yaml`** — LCSC supplier (`_primary_key: 1`).
- **`inventree.yaml.template`** — host + token placeholder. **The real `inventree.yaml`
  (with the token) is NOT committed** — copy the template, fill the token locally.

## Run
```
~/ipi-venv/bin/python -m inventree_part_import -o lcsc <C#…>
```
Reminder: the tool sets everything **except IPN**. Create a placeholder (name=MPN, IPN, category)
first, or set/verify the IPN after — see the `inventree-parts` skill.
