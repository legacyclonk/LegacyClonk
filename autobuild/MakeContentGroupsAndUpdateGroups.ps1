param(
    [string] $OutDir
)

$PSNativeCommandUseErrorActionPreference = $true
$ErrorActionPreference = "Stop"

$groups = ($Env:LC_GROUPS | ConvertFrom-Json -AsHashtable)["content"]

$parts = $groups["parts"]
$tempDir = [System.IO.Directory]::CreateTempSubdirectory()

$downloadedParts = [System.Collections.Generic.List[object]]::new()

foreach ($part in $parts) {
    Write-Output "Downloading $part..."
    $job = Start-ThreadJob -ScriptBlock {
        $partName = Split-Path -Path $using:part -Leaf
        $partPath = Join-Path $using:tempDir $partName
        Invoke-WebRequest -Uri "$using:part" -OutFile $partPath
        @{ PartPath = $partPath; Part = $partName }
    }

    $downloadedParts.Add($job)
}

$groupPaths = [System.Collections.Generic.List[object]]::new()

foreach ($group in $groups.GetEnumerator() | Where-Object { $_.Name -like "*.c4?" }) {
    Write-Host "Packing $($group.Name)..."
    & $Env:C4GROUP "$($group.Name)" -p
    if ($OutDir) {
        Move-Item -Path $group.Name -Destination $OutDir
        $groupPaths.Add((Resolve-Path -Path (Join-Path $OutDir $group.Name)))
    }
    else {
        $groupPaths.Add((Resolve-Path -Path $group.Name))
    }
}

$partsDir = Join-Path $tempDir 'parts'
New-Item -Path $partsDir -ItemType Directory | Out-Null

Wait-Job $downloadedParts | Out-Null

$c4group = Resolve-Path -Path $Env:C4GROUP

foreach ($partJob in $downloadedParts) {
    $partData = Receive-Job $partJob

    Remove-Item (Join-Path $partsDir "*")
    [System.IO.Compression.ZipFile]::ExtractToDirectory($partData.PartPath, $partsDir)

    Write-Output "`nAdding support for $($partData.Part)..."

    foreach ($groupPath in $groupPaths) {
        $groupName = Split-Path -Path $groupPath -Leaf
        $partPath = Join-Path $partsDir $groupName

        if (!(Test-Path $partPath)) {
            Write-Host "Part for $groupName not found, skipping"
            continue
        }

        Write-Output "$groupName..."

        $updatePath = $groupPath.Path + ".c4u"
        $allowMissingTarget = $groups[$groupName].'allow-missing-target'
        $updateOption = $allowMissingTarget ? '-ga' : '-g'

        Push-Location -Path $partsDir

        try {
            & $c4group "$updatePath" $updateOption "$groupName" "$groupPath" $Env:OBJVERSION
        }
        finally {
            Pop-Location
        }
    }
}

$combinedGroup = "lc_$Env:OBJVERSIONNODOTS.c4u"

if ($OutDir) {
    $combinedGroup = Join-Path $OutDir $combinedGroup
}

New-Item -Path $combinedGroup -ItemType Directory | Out-Null

foreach ($groupPath in $groupPaths) {
    Copy-Item -Path $($groupPath.Path + ".c4u") -Destination $combinedGroup
}

& $Env:C4GROUP "$combinedGroup" -p
