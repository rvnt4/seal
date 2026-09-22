Write-Host "Formatting sources..." 

$extensions = "*.cpp", "*.h", "*.cs"
Get-ChildItem -Path "src" -Include $extensions -Recurse | ForEach-Object {
    Write-Host "Formatting: $($_.FullName)"
    clang-format -i "$($_.FullName)"
}

Write-Host "Done!"