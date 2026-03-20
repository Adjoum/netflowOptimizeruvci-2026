@echo off
:: ══════════════════════════════════════════════════════════════════
::  NetFlow Optimizer — Lanceur Windows (double-cliquer pour lancer)
::  ALC2101 — UVCI 2025-2026
:: ══════════════════════════════════════════════════════════════════
title NetFlow Optimizer - ALC2101 UVCI
color 0B

echo.
echo  ==============================================
echo   NetFlow Optimizer ^& Security Analyzer
echo   ALC2101 ^| UVCI 2025-2026
echo  ==============================================
echo.

:: ─── 1. Compilation C ────────────────────────────────────────────
echo  [1/3] Compilation du projet C avec GCC...
where gcc >nul 2>&1
if %errorlevel% neq 0 (
    echo.
    echo  ERREUR : gcc introuvable.
    echo  Solution : Installez MinGW-w64
    echo  Lien     : https://github.com/niXman/mingw-builds-binaries/releases
    echo  Puis ajoutez C:\mingw64\bin au PATH Windows
    echo.
    pause & exit /b 1
)

gcc -std=c11 -Isrc ^
    src\graphe.c src\liste_chainee.c src\dijkstra.c ^
    src\securite.c src\utils.c src\api.c src\main.c ^
    -o netflow.exe -lm

if %errorlevel% neq 0 (
    echo.
    echo  ERREUR de compilation. Verifiez les messages ci-dessus.
    pause & exit /b 1
)
echo  [OK] netflow.exe compile.
echo.

:: ─── 2. Vérification Python ──────────────────────────────────────
echo  [2/3] Verification de Python...
python --version >nul 2>&1
if %errorlevel% neq 0 (
    python3 --version >nul 2>&1
    if %errorlevel% neq 0 (
        echo  ERREUR : Python 3 introuvable.
        echo  Installez Python depuis https://python.org
        echo  (Cochez "Add Python to PATH" lors de l'installation)
        pause & exit /b 1
    )
    set PY=python3
) else (
    set PY=python
)

:: Installer Flask si manquant
%PY% -c "import flask" >nul 2>&1
if %errorlevel% neq 0 (
    echo  Installation de Flask...
    %PY% -m pip install flask flask-cors --quiet
    if %errorlevel% neq 0 (
        echo  ERREUR : pip install flask flask-cors a echoue.
        echo  Lancez manuellement : pip install flask flask-cors
        pause & exit /b 1
    )
)
echo  [OK] Python + Flask disponibles.
echo.

:: ─── 3. Lancement ────────────────────────────────────────────────
echo  [3/3] Lancement du serveur web...
echo.
echo  ============================================
echo   Ouvrez votre navigateur sur :
echo.
echo      http://localhost:5000
echo.
echo   (Ctrl+C pour arreter le serveur)
echo  ============================================
echo.

:: Ouvrir Chrome/Edge automatiquement dans 2 secondes
start "" cmd /c "timeout /t 2 /nobreak >nul && start http://localhost:5000"

%PY% server.py

echo.
echo  Serveur arrete.
pause
