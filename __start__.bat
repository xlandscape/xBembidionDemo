@echo off

rem Pfad zum zu leerenden Ordner
set folder_path=D:\_Git_\24019_BAY_Bembidion\Test\bembid\run

rem Prüfen, ob der Ordner existiert
if exist %folder_path% (
    echo Leere den Ordner: %folder_path%
    del /q "%folder_path%\*" >nul 2>&1
    for /d %%i in ("%folder_path%\*") do rmdir /s /q "%%i"
    echo Ordner %folder_path% wurde geleert.
) else (
    echo Ordner %folder_path% existiert nicht.
)

rem Modell starten
"D:\_Git_\24019_BAY_Bembidion\Test\bembid\model\core\bin\python-3.9.7-amd64\python.exe" -u "D:\_Git_\24019_BAY_Bembidion\Test\bembid\model\core\init.py" %*
