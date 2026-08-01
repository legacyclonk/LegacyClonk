param(
    [Parameter(Mandatory = $true)]
    [string] $OS,

    [Parameter(Mandatory = $true)]
    [string] $Arch,

    [Parameter(Mandatory = $true)]
    [string] $PlatformSuffix,

    [Parameter(Mandatory = $true)]
    [string] $Manifest,

    [Parameter(Mandatory = $true)]
    [string] $Tag
)

$PSNativeCommandUseErrorActionPreference = $true
$ErrorActionPreference = "Stop"

if (!$Env:BUTLER_API_KEY) {
    Write-Error "Env:BUTLER_API_KEY not set"
    exit 1
}

$osLower = $OS.ToLower()

Copy-Item -Path $Manifest -Destination lc_full/.itch.toml

$butlerRelease = switch ("$osLower-$Arch") {
    { $_ -like 'windows-*' } { 'windows-amd64'; break }
    'mac-universal' { 'darwin-universal'; break }
    'linux-x64' { 'linux-amd64'; break }
    'linux-aarch64' { 'linux-arm64'; break }
    default {
        Write-Error "Invalid architecture"
        exit 1
    }
}

Invoke-WebRequest -Uri "https://broth.itch.zone/butler/$butlerRelease/LATEST/archive/default" -OutFile butler.zip
[System.IO.Compression.ZipFile]::ExtractToDirectory('butler.zip', 'butler')

if (!$IsWindows) {
    chmod +x butler/butler
}

$channelName = "$osLower-$Arch"

if ($Tag -ne "v${Env:VERSION}") {
    $channelName = "${Tag}-${channelName}"
}

$dryRun = [bool]::Parse($env:BUTLER_DRY_RUN) ? '--dry-run' : $null

butler/butler login
butler/butler push $dryRun --fix-permissions lc_full fulgen/legacyclonk:$channelName --userversion "$Env:OBJVERSION [$Env:VERSION]"
