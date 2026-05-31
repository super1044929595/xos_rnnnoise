@echo off
chcp 65001 >nul
cd /d "%~dp0"
echo ========================================
echo   WebEval 服务器启动中...
echo   请勿关闭此窗口
echo ========================================
echo.
python app.py
echo.
echo 服务器已停止。
pause
