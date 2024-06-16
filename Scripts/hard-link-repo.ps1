# Call this script as such:
# .\hard-link-repo.ps1 "path to Unreal project folder"
# File and folder symbolic links will be created.

param (
	[Parameter(HelpMessage="Target directory when files and folders should be hard linked to.")]
	[ValidateNotNullOrEmpty()]
	[string]$target=$(
		throw "`"-target`" is mandatory, please provide a value (path to a folder)."
	)
)

##############
### Config ###

$plugin_name = "WASP"
$plugin_uplugin_name = "WASP.uplugin"
$source_directory = "../Plugins/$plugin_name" | Resolve-Path
$target_project_root = $target | Resolve-Path
$target_directory = Join-Path $target_project_root "Plugins/$plugin_name"

##############

Add-Type -AssemblyName System.Windows.Forms

function Show-Error([string]$msg) {
    [Console]::ForegroundColor = 'red'
    [Console]::BackgroundColor = 'black'
    [Console]::Error.WriteLine($msg)
    [Console]::ResetColor()
}

function Link-File([string]$src_file, [string]$tgt_file) {
    if (-Not (Test-Path -Path "$src_file" -PathType Leaf)) {
        Write-Error "The source file does not exist: `"$src_file`""
        return
    }

    if (Test-Path -Path "$tgt_file") {
        Show-Error "The target file `"$tgt_file`" already exists. Skipping..."
        return
    }

    Write-Host "Linking $src_file"
    Write-Host "     to $tgt_file"
    New-Item -ItemType SymbolicLink -Path "$tgt_file" -Target "$src_file"
}

function Link-Folder([string]$src_folder, [string]$tgt_folder) {
    if (-Not (Test-Path -Path "$src_folder" -PathType Container)) {
        Write-Error "The source folder does not exist: `"$src_folder`""
        return
    }

    if (Test-Path -Path "$tgt_folder") {
        Show-Error "The target folder `"$tgt_folder`" already exists. Skipping..."
        return
    }

    Write-Host "Linking $src_folder"
    Write-Host "     to $tgt_folder"
    New-Item -ItemType SymbolicLink -Path "$tgt_folder\" -Target "$src_folder\"
}

if (-Not (Test-Path -Path "$source_directory" -PathType Container)) {
    Show-Error "The source path is not a directory: `"$source_directory`""
    Exit
}

if (-Not (Test-Path -Path "$target_project_root" -PathType Container)) {
    Show-Error "The target path is not a directory: `"$target_project_root`""
    Exit
}

# Create plugin directory if it does not exist
New-Item -ItemType Directory -Force -Path $target_directory

Write-Host ""
Link-File (Join-Path $source_directory $plugin_uplugin_name) (Join-Path $target_directory $plugin_uplugin_name)
Link-Folder (Join-Path $source_directory "Config") (Join-Path $target_directory "Config")
Link-Folder (Join-Path $source_directory "Content") (Join-Path $target_directory "Content")
Link-Folder (Join-Path $source_directory "Resources") (Join-Path $target_directory "Resources")
Link-Folder (Join-Path $source_directory "Source") (Join-Path $target_directory "Source")
