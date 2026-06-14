[CmdletBinding()]
param(
    [ValidateSet('x64')]
    [string] $Target = 'x64',
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release', 'MinSizeRel')]
    [string] $Configuration = 'RelWithDebInfo'
)

$ErrorActionPreference = 'Stop'

if ( $DebugPreference -eq 'Continue' ) {
    $VerbosePreference = 'Continue'
    $InformationPreference = 'Continue'
}

if ( $env:CI -eq $null ) {
    throw "Package-Windows.ps1 requires CI environment"
}

if ( ! ( [System.Environment]::Is64BitOperatingSystem ) ) {
    throw "Packaging script requires a 64-bit system to build and run."
}

if ( $PSVersionTable.PSVersion -lt '7.2.0' ) {
    Write-Warning 'The packaging script requires PowerShell Core 7. Install or upgrade your PowerShell version: https://aka.ms/pscore6'
    exit 2
}

function Package {
    trap {
        Write-Error $_
        exit 2
    }

    $ScriptHome = $PSScriptRoot
    $ProjectRoot = Resolve-Path -Path "$PSScriptRoot/../.."
    $BuildSpecFile = "${ProjectRoot}/buildspec.json"

    $UtilityFunctions = Get-ChildItem -Path $PSScriptRoot/utils.pwsh/*.ps1 -Recurse

    foreach( $Utility in $UtilityFunctions ) {
        Write-Debug "Loading $($Utility.FullName)"
        . $Utility.FullName
    }

    $BuildSpec = Get-Content -Path ${BuildSpecFile} -Raw | ConvertFrom-Json
    $ProductName = $BuildSpec.name
    $ProductVersion = $BuildSpec.version

    $OutputName = "${ProductName}-${ProductVersion}-windows-${Target}"

    $RemoveArgs = @{
        ErrorAction = 'SilentlyContinue'
        Path = @(
            "${ProjectRoot}/release/${ProductName}-*-windows-*.zip"
            "${ProjectRoot}/release/${ProductName}-*-windows-*.exe"
        )
    }

    Remove-Item @RemoveArgs

    # Drop a plain-text install guide next to the files for users who prefer the
    # loose-files zip over the double-click installer.
    $InstallNote = @"
Scene Tree - OBS Studio plugin
==============================

EASIEST: run the .exe installer from the Releases page instead - it copies
everything to the right place automatically (no admin needed).

Manual install (this zip):
  1. Close OBS Studio.
  2. Copy the "${ProductName}" folder from this zip into your OBS per-user
     plugins folder. Paste this into the Explorer address bar:
         %AppData%\obs-studio\plugins\
     You should end up with:
         %AppData%\obs-studio\plugins\${ProductName}\bin\64bit\${ProductName}.dll
         %AppData%\obs-studio\plugins\${ProductName}\data\locale\en-US.ini
  3. Start OBS, then enable the "Scene Tree" dock from the Docks menu.

Source & help: https://github.com/jcocano/obs-scene-tree
"@
    Set-Content -Path "${ProjectRoot}/release/${Configuration}/INSTALL.txt" -Value $InstallNote -Encoding utf8

    Log-Group "Archiving ${ProductName}..."
    $CompressArgs = @{
        Path = (Get-ChildItem -Path "${ProjectRoot}/release/${Configuration}" -Exclude "${OutputName}*.*")
        CompressionLevel = 'Optimal'
        DestinationPath = "${ProjectRoot}/release/${OutputName}.zip"
        Verbose = ($Env:CI -ne $null)
    }
    Compress-Archive -Force @CompressArgs
    Log-Group

    Build-Installer $ProjectRoot $ProductName $ProductVersion $OutputName $Configuration
}

function Build-Installer {
    param(
        [string] $ProjectRoot,
        [string] $ProductName,
        [string] $ProductVersion,
        [string] $OutputName,
        [string] $Configuration
    )

    Log-Group "Building installer for ${ProductName}..."

    # Locate the Inno Setup compiler; install it via Chocolatey if the runner
    # image does not ship it.
    $Iscc = (Get-Command 'iscc.exe' -ErrorAction SilentlyContinue).Source
    if ( ! $Iscc ) {
        foreach ( $Candidate in @(
            "${Env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
            "${Env:ProgramFiles}\Inno Setup 6\ISCC.exe"
        ) ) {
            if ( Test-Path $Candidate ) { $Iscc = $Candidate; break }
        }
    }
    if ( ! $Iscc ) {
        Write-Information 'Inno Setup not found; installing via Chocolatey...'
        choco install innosetup --no-progress --yes
        $Iscc = "${Env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
    }

    $SourceDir = "${ProjectRoot}/release/${Configuration}/${ProductName}"
    $IssFile = "${ProjectRoot}/build-aux/windows/installer.iss"
    $IconFile = "${ProjectRoot}/branding/icon.ico"

    $IsccArgs = @(
        '/Qp'
        "/DAppName=${ProductName}"
        "/DAppVersion=${ProductVersion}"
        "/DSourceDir=$((Resolve-Path $SourceDir).Path)"
        "/DOutputDir=$((Resolve-Path "${ProjectRoot}/release").Path)"
        "/DOutputBaseName=${OutputName}"
    )
    if ( Test-Path $IconFile ) {
        $IsccArgs += "/DSetupIcon=$((Resolve-Path $IconFile).Path)"
    }
    $IsccArgs += (Resolve-Path $IssFile).Path

    & $Iscc @IsccArgs
    if ( $LASTEXITCODE -ne 0 ) {
        throw "Inno Setup compiler failed with exit code ${LASTEXITCODE}."
    }
    Log-Group
}

Package
