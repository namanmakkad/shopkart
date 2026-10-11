# build.ps1 - ShopKart build script for Windows
# Usage: .\build.ps1 [clean|test|sanitize]

$ErrorActionPreference = "Stop"

$CC = "gcc"
$CFLAGS = "-Wall -Wextra -O2 -std=c99"
$LDFLAGS = "-lws2_32"
$TARGET = "shopkart.exe"
$SRCDIR = "src"
$OBJDIR = "obj"

$Sources = @(
    "$SRCDIR/main.c",
    "$SRCDIR/server.c",
    "$SRCDIR/http_utils.c",
    "$SRCDIR/render.c",
    "$SRCDIR/product.c",
    "$SRCDIR/sort.c",
    "$SRCDIR/search.c",
    "$SRCDIR/hashmap.c",
    "$SRCDIR/auth.c",
    "$SRCDIR/cart.c",
    "$SRCDIR/stack.c",
    "$SRCDIR/queue.c",
    "$SRCDIR/linkedlist.c",
    "$SRCDIR/heap.c",
    "$SRCDIR/bst.c"
)

function Build-Project {
    if (-not (Test-Path $OBJDIR)) {
        New-Item -ItemType Directory -Path $OBJDIR | Out-Null
    }

    $objects = @()
    foreach ($src in $Sources) {
        $objName = [System.IO.Path]::GetFileNameWithoutExtension($src) + ".o"
        $objPath = "$OBJDIR/$objName"
        $objects += $objPath
        Write-Host "Compiling $src ..." -ForegroundColor Cyan
        & $CC $CFLAGS.Split(" ") -c -o $objPath $src
        if ($LASTEXITCODE -ne 0) {
            Write-Host "ERROR: Compilation failed for $src" -ForegroundColor Red
            exit 1
        }
    }

    Write-Host "Linking $TARGET ..." -ForegroundColor Cyan
    & $CC $CFLAGS.Split(" ") -o $TARGET @objects $LDFLAGS.Split(" ")
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Linking failed" -ForegroundColor Red
        exit 1
    }

    Write-Host "Build successful: $TARGET" -ForegroundColor Green
}

function Clean-Project {
    if (Test-Path $OBJDIR) { Remove-Item -Recurse -Force $OBJDIR }
    if (Test-Path $TARGET) { Remove-Item -Force $TARGET }
    if (Test-Path "test_dsa.exe") { Remove-Item -Force "test_dsa.exe" }
    Write-Host "Clean complete." -ForegroundColor Green
}

function Build-Tests {
    Write-Host "Building tests..." -ForegroundColor Cyan
    $testSources = @(
        "tests/test_dsa.c",
        "$SRCDIR/product.c",
        "$SRCDIR/sort.c",
        "$SRCDIR/search.c",
        "$SRCDIR/hashmap.c",
        "$SRCDIR/stack.c",
        "$SRCDIR/queue.c",
        "$SRCDIR/linkedlist.c",
        "$SRCDIR/heap.c",
        "$SRCDIR/bst.c"
    )
    & $CC $CFLAGS.Split(" ") -o "test_dsa.exe" @testSources $LDFLAGS.Split(" ")
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Test build failed" -ForegroundColor Red
        exit 1
    }
    Write-Host "Running tests..." -ForegroundColor Cyan
    & "./test_dsa.exe"
    if ($LASTEXITCODE -ne 0) {
        Write-Host "TESTS FAILED" -ForegroundColor Red
        exit 1
    }
    Write-Host "All tests passed!" -ForegroundColor Green
}

# Main
$action = if ($args.Count -gt 0) { $args[0] } else { "build" }

switch ($action) {
    "clean"    { Clean-Project }
    "test"     { Build-Tests }
    "build"    { Build-Project }
    default    { Write-Host "Usage: .\build.ps1 [build|clean|test]"; exit 1 }
}
