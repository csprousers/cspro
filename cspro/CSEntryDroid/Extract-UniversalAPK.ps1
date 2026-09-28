# build-and-extract-apk.ps1
param(
    [Parameter(Mandatory=$true)]
    [string]$Password
)

$releaseDir = ".\app\release\"
$bundle = Join-Path $releaseDir "app-release.aab"
$outputApks = Join-Path $releaseDir "app-release-universal.apks"
$outputZip = Join-Path $releaseDir "app-release-universal.zip"
$extractDir = Join-Path $releaseDir "extracted"

$bundletoolJar = "bundletool-all-1.18.3.jar"

# Signing config - adjust these
$keystore = ".csentrydroidkeystore"
$keyAlias = "gov.census.cspro.csentrydroid"
$keyPass = "pass:$Password"

# 1. Build universal APK set from the bundle
java -jar $bundletoolJar build-apks `
  --bundle=$bundle `
  --output=$outputApks `
  --mode=universal `
  --ks=$keystore `
  --ks-key-alias=$keyAlias `
  --ks-pass=$keyPass `
  --overwrite

# 2. Rename .apks -> .zip so Expand-Archive recognizes it
if (Test-Path $outputZip) { Remove-Item $outputZip }
Move-Item $outputApks $outputZip

# 3. Extract
if (Test-Path $extractDir) { Remove-Item $extractDir -Recurse -Force }
Expand-Archive -Path $outputZip -DestinationPath $extractDir

Write-Host "Done. APK at: $extractDir\universal.apk"