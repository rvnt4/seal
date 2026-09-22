$projectFolder = "build"
$slnName = "seal"

function Get-CMakeVSGenerator {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    
    if (-not (Test-Path $vswhere)) {
        Write-Error "vswhere.exe not found. Is Visual Studio installed?."
        exit 1
    }

    $vsVersion = &$vswhere -latest -property installationVersion
    $vsMajor =$vsVersion.Split('.')[0]

    switch ($vsMajor) {
        "18" { return "Visual Studio 18 2026" }
        "17" { return "Visual Studio 17 2022" }
        "16" { return "Visual Studio 16 2019" }
        "15" { return "Visual Studio 15 2017" }
        default { 
            Write-Error "Unsupported or unknown Visual Studio version: $vsMajor"
            exit 1
        }
    }
}

function Initialize-Project {
    $cmakeGenerator = Get-CMakeVSGenerator
    Write-Host "Auto-detected generator: $cmakeGenerator" -ForegroundColor Green

    Write-Host "Generating project..."
    Set-Location $projectFolder

    cmake .. -G $cmakeGenerator -A x64 `
    -DCMAKE_CONFIGURATION_TYPES="Debug;Release" `
    -DSEAL_TEST=ON
    
    $cmakeExit =$LASTEXITCODE
    if ($cmakeExit -ne 0) {
        Write-Host "CMake generation failed with exit code: $cmakeExit" -ForegroundColor Red
        exit 1
    }
    
    Set-Location ..

    try {
        $solutionPath = ""
        if (Test-Path "$projectFolder\$slnName.slnx") {
            $solutionPath = "$projectFolder\$slnName.slnx"
        } elseif (Test-Path "$projectFolder\$slnName.sln") {
            $solutionPath = "$projectFolder\$slnName.sln"
        } else {
            throw "Solution file ($slnName.sln or $slnName.slnx) not found in$projectFolder."
        }

        $solutionFile = Get-Item -Path $solutionPath -ErrorAction Stop
        $solutionFullName =$solutionFile.FullName
        
        $vsProcesses = Get-CimInstance -Query "SELECT CommandLine FROM Win32_Process WHERE Name = 'devenv.exe'"
        $pattern = [Regex]::Escape($solutionFullName)
        $isOpen =$vsProcesses | Where-Object { $_.CommandLine -match$pattern }

        if (-not $isOpen) {
            Write-Host "Starting Visual Studio..." -ForegroundColor Cyan
            Start-Process -FilePath $solutionFullName
        }
        else {
            Write-Host "Solution '$($solutionFile.Name)' is already open." -ForegroundColor Yellow
        }
    }
    catch {
        Write-Error "Failed to open solution: $($_.Exception.Message)"
    }
}

if (-not (Test-Path $projectFolder)) {
    New-Item -ItemType Directory -Path $projectFolder | Out-Null
}

Initialize-Project