param(
    [Parameter(Position = 0, Mandatory = $true)]
    [ValidateSet('build', 'fullclean', 'flash', 'monitor', 'clean', 'menuconfig')]
    [string]$Action
)

$ErrorActionPreference = 'Stop'

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Resolve-Path (Join-Path $scriptRoot '..')

$env:IDF_PATH = 'C:\esp\v6.0.2\esp-idf'

& "$env:IDF_PATH\export.ps1" | Out-Null

Push-Location $projectRoot
try {
    switch ($Action) {
        'build' { idf.py build }
        'fullclean' { idf.py fullclean }
        'flash' { idf.py -p COM3 flash }
        'monitor' { idf.py -p COM3 monitor }
        'clean' { idf.py clean }
        'menuconfig' { idf.py menuconfig }
    }
}
finally {
    Pop-Location
}
