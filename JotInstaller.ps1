<#
Simple user-scope installer for Jot.exe

There are no plans to support system-wide installation at this time.

Usage:
  - Install for current user (no admin required):
	  .\JotInstaller.ps1

  - Force reinstall:
	  .\JotInstaller.ps1 -Force

  - Uninstall for current user:
	  .\JotInstaller.ps1 -Uninstall

This script copies `Jot.exe` (located beside this script) into `%LOCALAPPDATA%\Programs\Jot`
and adds/removes that folder from the current user's PATH.
#>

param(
	[switch]$Uninstall,
	[switch]$Force
)

function Add-ToPath([string]$folder) {
	$targetScope = 'User'
	$currentPath = [Environment]::GetEnvironmentVariable('Path', $targetScope)
	if (-not $currentPath) { $currentPath = '' }
	$pathParts = $currentPath -split ';' | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne '' }
	if ($pathParts -contains $folder) {
		Write-Host "Path already contains: $folder"
		return
	}
	$newPath = $currentPath.TrimEnd(';') + ";$folder"
	[Environment]::SetEnvironmentVariable('Path', $newPath, $targetScope)
	Write-Host "Added $folder to user PATH. You may need to open a new shell to see it."
}

function Remove-FromPath([string]$folder) {
	$targetScope = 'User'
	$currentPath = [Environment]::GetEnvironmentVariable('Path', $targetScope)
	if (-not $currentPath) { return }
	$parts = $currentPath -split ';' | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne '' }
	$remaining = $parts | Where-Object { $_ -ne $folder }
	$newPath = ($remaining -join ';')
	[Environment]::SetEnvironmentVariable('Path', $newPath, $targetScope)
	Write-Host "Removed $folder from user PATH (if present)."
}

function Perform-Install {
	$destBase = Join-Path $env:LOCALAPPDATA 'Programs\Jot'
	$sourceExePath = Join-Path $PSScriptRoot 'Jot.exe'
	if (-not (Test-Path $sourceExePath)) {
		Write-Error "Could not find Jot.exe at: $sourceExePath. Place `Jot.exe` next to this script and try again."
		exit 1
	}
	if (-not (Test-Path $destBase)) {
		New-Item -ItemType Directory -Path $destBase -Force | Out-Null
	}
	$destExePath = Join-Path $destBase 'Jot.exe'
	if ((Test-Path $destExePath) -and -not $Force) {
		Write-Host "Jot already installed at $destExePath. Use -Force to overwrite."
		return
	}
	Copy-Item -Path $sourceExePath -Destination $destExePath -Force:$Force
	Write-Host "Copied Jot.exe -> $destExePath"
	Add-ToPath -folder $destBase
	Write-Host "Installation finished. You can run 'Jot.exe' from any new shell."
}

function Perform-Uninstall {
	$destBase = Join-Path $env:LOCALAPPDATA 'Programs\Jot'
	if (Test-Path $destBase) {
		try {
			Remove-Item -Path $destBase -Recurse -Force
			Write-Host "Removed folder: $destBase"
		} catch {
			Write-Warning ("Failed to remove {0}: {1}" -f $destBase, $_)
		}
	} else {
		Write-Host "No install found at $destBase"
	}
	Remove-FromPath -folder $destBase
	Write-Host "Uninstall complete. You may need to open a new shell to see PATH changes."
}

function Show-Help {
	Write-Host "Usage: .\JotInstaller.ps1 [-Uninstall] [-Force]"
}

if ($Uninstall) { Perform-Uninstall; exit }

Perform-Install


