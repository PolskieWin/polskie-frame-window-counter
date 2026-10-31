# Build script for Polskie Frame Window Counter
param(
    [string]$Platform = "android64"  # Options: android64, android32, win
)

Write-Host "=================================================" -ForegroundColor Cyan
Write-Host "   Polskie Frame Window Counter - Build Script   " -ForegroundColor Yellow
Write-Host "=================================================" -ForegroundColor Cyan
Write-Host "Cel platformy: $Platform" -ForegroundColor Green

# Check if geode CLI is available
$geodeCmd = Get-Command geode -ErrorAction SilentlyContinue

if (-not $geodeCmd) {
    Write-Host "[!] Geode CLI nie zostalo znalezione w sciezce PATH." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Mozesz zainstalowac Geode CLI automatycznie uzywajac:" -ForegroundColor White
    Write-Host "  winget install GeodeSDK.GeodeCLI" -ForegroundColor Cyan
    Write-Host "lub pobrac z oficjalnej strony: https://geode-sdk.org/install" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "LUB mozesz uzyc GitHub Actions - wystarczy wypchnac repozytorium na GitHub," -ForegroundColor White
    Write-Host "a plik .github/workflows/build.yml automatycznie skompiluje plik .geode!" -ForegroundColor Green
    exit 1
}

Write-Host "[+] Budowanie modyfikacji dla $Platform..." -ForegroundColor Green
geode build -p $Platform

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "[SUCCESS] Kompilacja zakonczona sukcesem!" -ForegroundColor Green
    Write-Host "Plik .geode znajduje sie w folderze build/" -ForegroundColor Yellow
} else {
    Write-Host ""
    Write-Host "[ERROR] Blad podczas kompilacji. Kod bledu: $LASTEXITCODE" -ForegroundColor Red
}

