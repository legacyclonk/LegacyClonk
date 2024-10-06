param(
    [Parameter(Mandatory = $true)]
    [string] $PlatformSuffix,

    [Parameter(Mandatory = $true)]
    [string] $RequireVersion,

    [string] $OutDir
)

$PSNativeCommandUseErrorActionPreference = $true
$ErrorActionPreference = "Stop"

$updatePath = "lc_${Env:VERSION}_${PlatformSuffix}.c4u"

if ($OutDir) {
    $updatePath = Join-Path $OutDir $updatePath
}

New-Item -Path $updatePath -ItemType Directory | Out-Null

Move-Item -Path 'clonk*' -Destination "$updatePath"
Copy-Item -Path 'c4group*' -Destination "$updatePath"
Move-Item -Path @('System.c4g.c4u', 'Graphics.c4g.c4u') -Destination "$updatePath"

@"
Update
Name=$Env:OBJVERSION
RequireVersion=$($RequireVersion -replace '\.', ',')
"@ | Out-File -FilePath "$(Join-Path $updatePath AutoUpdate.txt)"

& $Env:C4GROUP "$updatePath" -p
