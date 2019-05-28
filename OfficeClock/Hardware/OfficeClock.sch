EESchema Schematic File Version 4
LIBS:OfficeClock-cache
EELAYER 29 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L MCU_Module:WeMos_D1_mini U1
U 1 1 5CD37896
P 4200 4100
F 0 "U1" H 4200 3211 50  0000 C CNN
F 1 "WeMos_D1_mini" H 4200 3120 50  0000 C CNN
F 2 "D1_Mini:D1-MINI" H 4200 2950 50  0001 C CNN
F 3 "https://wiki.wemos.cc/products:d1:d1_mini#documentation" H 2350 2950 50  0001 C CNN
	1    4200 4100
	1    0    0    -1  
$EndComp
$Comp
L Displays:FC-16 U3
U 1 1 5CD53206
P 7100 3750
F 0 "U3" H 8880 3921 50  0000 L CNN
F 1 "FC-16" H 8880 3830 50  0000 L CNN
F 2 "OfficeClock:FC-16-Short" H 7100 3400 50  0001 C CNN
F 3 "https://datasheets.maximintegrated.com/en/ds/MAX7219-MAX7221.pdf" H 7500 3450 50  0001 C CNN
	1    7100 3750
	1    0    0    -1  
$EndComp
$Comp
L Sensor_Optical:BP103B Q1
U 1 1 5CD5CAF1
P 3200 4250
F 0 "Q1" H 3391 4296 50  0000 L CNN
F 1 "BP103B" H 3391 4205 50  0000 L CNN
F 2 "Opto-Devices:PhotoTransistor_Osram_LPT80A" H 3680 4110 50  0001 C CNN
F 3 "http://www.b-kainka.de/Daten/Sensor/bp103bf.pdf" H 3200 4250 50  0001 C CNN
	1    3200 4250
	1    0    0    -1  
$EndComp
$Comp
L Device:R_US R1
U 1 1 5CD5DA30
P 3300 3800
F 0 "R1" H 3368 3846 50  0000 L CNN
F 1 "R_US" H 3368 3755 50  0000 L CNN
F 2 "Resistors_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal" V 3340 3790 50  0001 C CNN
F 3 "~" H 3300 3800 50  0001 C CNN
	1    3300 3800
	1    0    0    -1  
$EndComp
$Comp
L 74xx:74LS367 U2
U 1 1 5CD61090
P 5650 3850
F 0 "U2" H 5650 4731 50  0000 C CNN
F 1 "74HCT367" H 5650 4640 50  0000 C CNN
F 2 "Housings_DIP:DIP-16_W10.16mm_LongPads" H 5650 3850 50  0001 C CNN
F 3 "http://www.ti.com/lit/gpn/sn74HCT367" H 5650 3850 50  0001 C CNN
	1    5650 3850
	1    0    0    -1  
$EndComp
Wire Wire Line
	5650 4550 5050 4550
Wire Wire Line
	5050 4550 5050 4250
Wire Wire Line
	5050 3750 5150 3750
Connection ~ 5650 4550
Wire Wire Line
	5050 3850 5150 3850
Connection ~ 5050 3850
Wire Wire Line
	5050 3850 5050 3750
Wire Wire Line
	5050 3950 5150 3950
Connection ~ 5050 3950
Wire Wire Line
	5050 3950 5050 3850
Wire Wire Line
	5050 4150 5150 4150
Connection ~ 5050 4150
Wire Wire Line
	5050 4150 5050 3950
Wire Wire Line
	5050 4250 5150 4250
Connection ~ 5050 4250
Wire Wire Line
	5050 4250 5050 4150
Wire Wire Line
	5650 4550 7100 4550
Wire Wire Line
	7100 4550 7100 4000
Wire Wire Line
	5050 4900 4200 4900
Connection ~ 5050 4550
$Comp
L Switch:SW_Push SW1
U 1 1 5CD5BB7B
P 2850 4900
F 0 "SW1" H 2850 5185 50  0000 C CNN
F 1 "SW_Push" H 2850 5094 50  0000 C CNN
F 2 "Buttons_Switches_THT:Push_E-Switch_KS01Q01" H 2850 5100 50  0001 C CNN
F 3 "~" H 2850 5100 50  0001 C CNN
	1    2850 4900
	1    0    0    -1  
$EndComp
Wire Wire Line
	4650 3800 4600 3800
Wire Wire Line
	5650 3150 4100 3150
Wire Wire Line
	4100 3150 4100 3300
Wire Wire Line
	5650 3150 7100 3150
Wire Wire Line
	7100 3150 7100 3250
Connection ~ 5650 3150
Wire Wire Line
	3300 3950 3300 4000
Wire Wire Line
	3300 4450 3300 4900
Wire Wire Line
	3300 4900 4200 4900
Connection ~ 4200 4900
Wire Wire Line
	3300 3650 3300 3250
Wire Wire Line
	3300 3250 4300 3250
Wire Wire Line
	4300 3250 4300 3300
Wire Wire Line
	4600 3600 4600 3050
Wire Wire Line
	4600 3050 3100 3050
Wire Wire Line
	3100 3050 3100 4000
Wire Wire Line
	3100 4000 3300 4000
Connection ~ 3300 4000
Wire Wire Line
	3300 4000 3300 4050
Wire Wire Line
	5050 4550 5050 4900
Wire Wire Line
	3050 4900 3300 4900
Connection ~ 3300 4900
Wire Wire Line
	2650 4900 2650 2900
Wire Wire Line
	2650 2900 4650 2900
Wire Wire Line
	4650 2900 4650 3800
Wire Wire Line
	4600 4400 4800 4400
Wire Wire Line
	4800 4400 4800 3450
Wire Wire Line
	4800 3450 5150 3450
Wire Wire Line
	4600 4200 4750 4200
Wire Wire Line
	4750 4200 4750 3550
Wire Wire Line
	4750 3550 5150 3550
Wire Wire Line
	4600 4500 4850 4500
Wire Wire Line
	4850 4500 4850 3650
Wire Wire Line
	4850 3650 5150 3650
Wire Wire Line
	6150 3450 6700 3450
Wire Wire Line
	6150 3550 6450 3550
Wire Wire Line
	6450 3550 6450 3600
Wire Wire Line
	6450 3600 6700 3600
Wire Wire Line
	6150 3650 6300 3650
Wire Wire Line
	6300 3650 6300 3750
Wire Wire Line
	6300 3750 6700 3750
$Comp
L power:GND #PWR0101
U 1 1 5CDDF4F0
P 5050 5100
F 0 "#PWR0101" H 5050 4850 50  0001 C CNN
F 1 "GND" H 5055 4927 50  0000 C CNN
F 2 "" H 5050 5100 50  0001 C CNN
F 3 "" H 5050 5100 50  0001 C CNN
	1    5050 5100
	1    0    0    -1  
$EndComp
Wire Wire Line
	5050 5100 5050 4900
Connection ~ 5050 4900
$Comp
L power:VCC #PWR0102
U 1 1 5CDE091D
P 7100 3000
F 0 "#PWR0102" H 7100 2850 50  0001 C CNN
F 1 "VCC" H 7117 3173 50  0000 C CNN
F 2 "" H 7100 3000 50  0001 C CNN
F 3 "" H 7100 3000 50  0001 C CNN
	1    7100 3000
	1    0    0    -1  
$EndComp
Wire Wire Line
	7100 3000 7100 3150
Connection ~ 7100 3150
NoConn ~ 6150 3750
NoConn ~ 6150 3850
NoConn ~ 6150 3950
NoConn ~ 4600 3700
NoConn ~ 4600 3900
NoConn ~ 4600 4000
NoConn ~ 4600 4100
NoConn ~ 4600 4300
NoConn ~ 3800 4100
NoConn ~ 3800 4000
NoConn ~ 3800 3700
$EndSCHEMATC
