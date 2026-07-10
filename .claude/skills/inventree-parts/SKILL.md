---
name: inventree-parts
description: Sync parts into InvenTree with stable IPNs and full supplier/cost data. Use when adding or updating a part in InvenTree, assigning an IPN, importing an LCSC part number (C####) with inventree-part-import, importing an Alibaba/AliExpress order (toggles/rotaries/encoders/buttons), linking a backorder-alternate SKU, filling per-panel cost, or reconciling KiCad LCSC labels against InvenTree. Owns the IPN scheme, the import workflow, and the non-LCSC supplier handling.
---

# InvenTree Parts — IPN + import + cost

InvenTree (`http://192.168.85.85`, token at `~/.config/inventree/token`) is the parts + costing
service. Goal: every part has a **stable descriptive IPN** (identity) plus complete supplier
data (MPN, datasheet, parameters, **price**) so per-panel cost rolls up and BOMs stop churning
when an LCSC SKU backorders. Full history/spec: GitHub issue **#238**.

## Two identities, never conflate
- **IPN** = the canonical part identity (our descriptive scheme). What a BOM/design references.
- **LCSC C# / MPN** = the *buyable* SupplierPart SKU. Resolved at order time; can change with
  availability without touching the IPN.

## IPN scheme (descriptive)
- **R:** `RES-<val>-<tol>-<pkg>` — val in R/K/M (`10K`,`4K7`,`100R`), milliohm shunt `0R010`;
  tol `1PCT`/`0P1PCT`. e.g. `RES-10K-1PCT-0805`.
- **C:** `CAP-<CER|C0G|ELEC>-<val>-<volt>-<pkg>` — val `100NF`/`2P2UF`; pkg `0805`/`D4`/`D5`
  (elec cans; **D4=Ø4×5.4 for 16–25V, D5=Ø5×5.4 for 35V** — size scales with voltage).
- **IC:** `IC-<part>-<pkg>` (`IC-STM32F103C8-LQFP`, `IC-MCP23017-SSOP`). **Q/D/LED/TVS/XTAL/
  FB/NTC/FUSE/SW/MOD/CONN** similar; `CONN-<XH|MF|HDR>-<pins>P`; module `MOD-RP2040-ZERO`;
  on-board tact `SW-TACT-6MM`; fuse **holder** `FUSE-HOLDER-BLADE-DIP4`.
- Off-board controls (encoders/switches/pots/OLED/stepper) get **no IPN in KiCad** (no
  footprint) but **do** get an IPN in InvenTree for cost (`SW-TOGGLE-*`, `ENC-EC11`, …).

## The import tool (third-party — do NOT fork)
`inventree-part-import` (https://github.com/30350n/inventree-part-import), run as
`~/ipi-venv/bin/python -m inventree_part_import -o lcsc <C#…>`. Config dir:
`~/Library/Application Support/inventree_part_import/` (`inventree.yaml` host/token,
`categories.yaml` LCSC→our-category aliases, `config.yaml`, `suppliers.yaml` LCSC=pk1). **The
config is version-controlled at `tools/inventree-part-import/`** (minus the token) — copy those
into the config dir on a new machine. When the tool errors "failed to match category", add the
LCSC leaf category to `categories.yaml` `_aliases` (we alias into our flat tree, not
create-on-the-fly).
It imports supplier/MPN/datasheet/parameters/**price**. **It has NO IPN capability**
(`ApiPart` has no IPN field; `get_part_data()` emits only name(=MPN)/description/link/
active/component/purchaseable; `hooks.py` can't inject IPN). So **IPN is our job**, set on a
placeholder the tool then enriches.

## Adding/syncing a single LCSC part — the workflow (PROVEN)
1. **Create placeholder** InvenTree part: `name` = the **MPN**, `IPN` = our scheme, `category`
   set. (MPN is on the KiCad symbol's `MP` field.) One `POST /api/part/`.
2. **RUN THE TOOL** (required — placeholder alone gives IPN+category only, NO cost/params/
   datasheet): `~/ipi-venv/bin/python -m inventree_part_import -o lcsc <C#>`. It finds the part
   by **name == MPN** (`get_part`), **updates in place** (`update_object_data` PATCHes only its
   keys → **IPN preserved**), and **skips category matching** (only runs when creating new).
3. **Verify** the part now has a supplier part + price + params (not just IPN).

## Duplicate-avoidance (critical)
The tool matches an existing part by: (a) an existing SupplierPart-with-a-**manufacturer_part**
by SKU, else (b) a ManufacturerPart by MPN, else (c) `get_part(MPN)` by **name**, else it
**CREATES NEW**. So a part that has a SupplierPart but **no manufacturer_part AND a
non-MPN (descriptive) name** will **DUPLICATE** on a tool run. Before re-syncing such a part,
**rename it to its MPN** (or add a manufacturer part) so it matches. **Dry mode (`--dry`) does
NOT reliably predict match-vs-create** — don't trust it; check the part's mfr-part/name instead.
Side effect: a successful sync **normalizes the part `name` to the MPN** (IPN stays the identity).

## Alternates = churn-proofing (one IPN, many SupplierParts)
The tool auto-enriches only the **name-matched primary**, BUT it will also enrich an
**alternate in place** if the alternate's SupplierPart already exists **with a manufacturer_part
linked** — the tool matches by SKU → its manufacturer_part → the part, and updates. So the clean
alternate procedure is a **stub + tool run** (don't hand-copy fields):
1. Create the stub on the **target IPN'd part** (only the identity bits — MPN + SKU):
   - company (mfr): find or `POST /api/company/` `{name, is_manufacturer:true}`
   - `POST /api/company/part/manufacturer/` `{part, manufacturer, MPN}`
   - `POST /api/company/part/` `{part, supplier:1, SKU:<C#>, manufacturer_part:<mfr_pk>}`
     — the **manufacturer_part link is REQUIRED** (a bare SupplierPart with no mfr link makes the
     tool create a duplicate instead of matching).
2. `~/ipi-venv/bin/python -m inventree_part_import -o lcsc <alt C#>` → matches the stub by SKU →
   **updates in place**, filling price/packaging/link/description/params/datasheet.
Do this **just-in-time** when a backorder/mismatch appears — not a bulk upfront pass.

**Don't fuss over the part `name`.** Nothing keys off it — BOMs use the **IPN**, ordering uses
the **SKU**, cost rolls up by part. The tool forces `name=MPN` on every import, so an alternate
run flips the name to the alternate's MPN + overwrites the description (IPN + supplier links are
unaffected). That's fine — the name is a cosmetic UI label; the **IPN is the stable identity**.
Only reset the name (`PATCH /api/part/<pk>/ {name, description}`) if a part labelled by an
obscure alternate MPN bothers you visually — it's optional, not required.

**Flag an out-of-stock supplier part** (keep the IPN, steer ordering to the alternate):
`PATCH /api/company/part/<sp>/` `{active:false, note:"OUT OF STOCK (date) — use <C#>"}`.

## Non-LCSC suppliers (Alibaba/AliExpress)
Toggles/rotaries/EC11/buttons come from **Alibaba** (e.g. Wenzhou Wintai), never LCSC. Model:
create the vendor as a Company (supplier+manufacturer), then Parts (Controls category) with our
IPN + a SupplierPart + price. **Fold shipping into unit cost** (order shipping ÷ total units,
added to each $/ea → landed price break). LCSC field stays empty for these.

## InvenTree API cheat-sheet (urllib + Token; retry/backoff on ≥500; **report every write**)
- Part: `POST /api/part/` `{name, category, IPN, description, component:true, purchaseable:true, active:true}`
- SupplierPart: `POST /api/company/part/` `{part, supplier, SKU}` (LCSC supplier = **pk1**)
- ManufacturerPart: `POST /api/company/part/manufacturer/` `{part, manufacturer, MPN}`
- Price break: `POST /api/company/price-break/` `{part:<supplierpart_pk>, quantity, price, price_currency}`
- Company: `POST /api/company/` `{name, is_supplier, is_manufacturer, is_customer, website, description}`
- Set IPN on existing: `PATCH /api/part/<pk>/` `{IPN}`
- Categories: Passives=7, Semiconductors=8, Connectors=9, Crystals=11, Modules=10, Controls=2.

## Rules
- **Never source new LCSC SKUs** — use only parts already selected (in a schematic/InvenTree);
  gap-list the rest for the user. **Report every InvenTree write for review.**
- Missing an LCSC **category alias** → the tool errors on *create* ("failed to match category").
  Add the alias to `categories.yaml` (or use the placeholder path, which skips category match).
- KiCad symbols carry **IPN + LCSC co-equal** (see `pcb-design`); this skill keeps InvenTree in
  sync with those.
