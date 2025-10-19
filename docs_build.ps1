<#
Simple PowerShell script to create a virtual environment, install requirements and build the MkDocs site.
#>

$venvPath = "./.venv"
if (-not (Test-Path $venvPath)) {
    python -m venv $venvPath
}

& "$venvPath/Scripts/Activate.ps1"
pip install --upgrade pip
pip install -r requirements.txt

mkdocs build --clean

Write-Host "MkDocs build finished. See site/ directory for output." -ForegroundColor Green
