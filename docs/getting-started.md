# Getting started

To build and preview the docs locally (Windows PowerShell):

1. Create a virtual environment and install dependencies:

```powershell
python -m venv .venv
. .\.venv\Scripts\Activate.ps1
pip install --upgrade pip
pip install -r requirements.txt
```

2. Build the site:

```powershell
./docs_build.ps1
```

3. Or preview with the MkDocs dev server:

```powershell
python -m venv .venv
. .\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
mkdocs serve
```

If you plan to deploy to GitHub Pages, enable GitHub Actions and push to the `main` branch — the included workflow will publish the `site/` output.
