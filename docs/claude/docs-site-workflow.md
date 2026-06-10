# Docs Site Workflow

OpenSkyhawk uses MkDocs Material for the documentation site and mkdoxy/Doxygen for generated API reference pages.

## Local Setup

Install the local documentation dependencies:

```bash
pip install mkdocs-material mkdoxy
```

Doxygen must also be available on the local system. On macOS, install it with Homebrew or the official Doxygen package.

## Build Check

Run a strict build before opening a PR:

```bash
mkdocs build -f docs/mkdocs.yml --strict
```

This generates the local `site/` directory. The directory is build output only, is gitignored, and should not be committed.

## Local Preview

Run the local preview server from the repository root:

```bash
mkdocs serve -f docs/mkdocs.yml -a 127.0.0.1:8010
```

Open:

```text
http://127.0.0.1:8010/OpenSkyhawk/
```

The `/OpenSkyhawk/` prefix matches the configured GitHub Pages path and catches broken absolute links before publishing.

## Generated API Reference

API pages are generated during the MkDocs build by mkdoxy. The checked-in `docs/api/index.md` page is the stable overview page; generated API detail pages remain ephemeral and are ignored by git.

If API navigation looks stale, rebuild from a clean generated state:

```bash
mkdocs build -f docs/mkdocs.yml --strict
```

## Publishing

The source branch contains documentation source files only. The generated site is published separately by the GitHub Actions workflow:

```bash
mkdocs gh-deploy -f docs/mkdocs.yml --force --remote-branch gh-pages
```

That command builds the site and pushes the rendered output to the `gh-pages` branch. Do not commit the local `site/` directory to a source branch.

## Asset Notes

Source mock images used while designing the site can live under:

```text
docs/assets/source-mocks/
```

That folder is gitignored and excluded from the published site. Commit only the cropped or optimized assets that the site actually references.
