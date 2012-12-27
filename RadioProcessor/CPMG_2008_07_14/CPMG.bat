@echo off
set SpectrometerFrequency=10.783
set SpectralWidth=100
set P2_Time=14
set ringdown_time=100
set P2_phase=0.0
set P1_phase=90.0
set tau=2000
set echo_points=0
set echo_loops=100
set scans=1
set FileName=Out2
set BypassFIR=1
set adcFrequency=75.0
set wait_time=1.0

CPMG %SpectrometerFrequency% %SpectralWidth% %P2_Time% %ringdown_time% %P2_phase% %P1_phase% %tau% %echo_points% %echo_loops% %scans% %FileName% %BypassFIR% %adcFrequency% %wait_time%

pause