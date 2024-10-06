param(
    [Parameter(Mandatory = $true)]
    [string] $PlatformSuffix,

    [Parameter(Mandatory = $true)]
    [string] $EngineUpdatePath,

    [string] $OutDir
)

$PSNativeCommandUseErrorActionPreference = $true
$ErrorActionPreference = "Stop"

if (!(Test-Path $EngineUpdatePath)) {
    Write-Error "Engine update file not found: $EngineUpdatePath"
    exit 1
}

$updatePath = $Env:MAJOR_UPDATE ? "lc_${Env:OBJVERSIONNODOTS}_${PlatformSuffix}.c4u" : "lc_${Env:OBJVERSIONNODOTS}_${Env:VERSION}_${PlatformSuffix}.c4u"

if ($OutDir) {
    $updatePath = Join-Path $OutDir $updatePath
}

New-Item -Path $updatePath -ItemType Directory | Out-Null
Move-Item -Path $EngineUpdatePath -Destination "$updatePath"
Move-Item -Path "lc_${Env:OBJVERSIONNODOTS}.c4u" -Destination "$updatePath"
Copy-Item -Path $Env:C4GROUP -Destination "$updatePath"

& $Env:C4GROUP "$updatePath" -p
