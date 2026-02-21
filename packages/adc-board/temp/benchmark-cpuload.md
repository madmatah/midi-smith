
# Tests de charge 


## Parametre = ANALOG_ACQUISITION_SEQUENCES_PER_HALF_BUFFER

### Profil debug (-Og) et ANALOG_ACQUISITION_SEQUENCES_PER_HALF_BUFFER = 1
adc-board> ps
name cpu% stack_free_b prio state runtime_delta window_ms=250
AnalogAcq 75.3 1564 3 B 1385
IDLE 22.7 464 0 Y 419
SensorRtt 0.8 836 1 B 16
defaultTask 0.6 3996 24 B 12
ShellTask 0.3 1588 1 R 6
Tmr Svc 0.0 912 2 B 0

### Profil debug (-Og) et ANALOG_ACQUISITION_SEQUENCES_PER_HALF_BUFFER = 4 (un peu plus nerveux, on partira peut-être plus sur cette valeur)

adc-board> ps
name cpu% stack_free_b prio state runtime_delta window_ms=250
AnalogAcq 53.5 1564 3 B 985
IDLE 44.7 464 0 Y 823
SensorRtt 0.8 836 1 Y 15
defaultTask 0.4 3996 24 B 8
ShellTask 0.3 1444 1 R 6
Tmr Svc 0.0 912 2 B 0

### Profil debug (-Og) et ANALOG_ACQUISITION_SEQUENCES_PER_HALF_BUFFER = 8 (idéal)

adc-board> ps
name cpu% stack_free_b prio state runtime_delta window_ms=250
AnalogAcq 50.5 1564 3 B 928
IDLE 48.1 464 0 Y 885
SensorRtt 0.5 836 1 Y 11
defaultTask 0.4 3996 24 B 8
ShellTask 0.2 1460 1 R 5
Tmr Svc 0.0 912 2 B 0


### Profil debug (-Og) et ANALOG_ACQUISITION_SEQUENCES_PER_HALF_BUFFER = 16
adc-board> ps
name cpu% stack_free_b prio state runtime_delta window_ms=250
AnalogAcq 49.3 1564 3 B 910
IDLE 49.2 464 0 Y 909
SensorRtt 0.5 876 1 Y 11
defaultTask 0.4 3996 24 B 9
ShellTask 0.3 1588 1 R 6
Tmr Svc 0.0 912 2 B 0


## Parametre = flags de compilation

Paramètres de base :
-ANALOG_ACQUISITION_SEQUENCES_PER_HALF_BUFFER = 4

### Profil release (-Os)

adc-board> ps
name cpu% stack_free_b prio state runtime_delta window_ms=250
IDLE 54.9 432 0 Y 1010
AnalogAcq 43.7 1580 3 B 804
SensorRtt 0.5 828 1 Y 10
defaultTask 0.4 4028 24 B 8
ShellTask 0.3 1564 1 R 6
Tmr Svc 0.0 888 2 B 0

### Profil release (-O3)
adc-board> ps
name cpu% stack_free_b prio state runtime_delta window_ms=250
IDLE 55.6 432 0 Y 1019
AnalogAcq 43.1 1604 3 B 789
SensorRtt 0.6 820 1 Y 12
defaultTask 0.4 3980 24 B 9
ShellTask 0.0 1476 1 R 1
Tmr Svc 0.0 880 2 B 0


### Profil release (-O3 -flto -Wl,-u,vTaskSwitchContext)
adc-board> ps
name cpu% stack_free_b prio state runtime_delta window_ms=250
IDLE 54.2 432 0 Y 994
AnalogAcq 44.8 1612 3 B 821
SensorRtt 0.4 836 1 Y 9
defaultTask 0.3 3992 24 B 6
ShellTask 0.0 1460 1 R 0
Tmr Svc 0.0 884 2 B 0


### Profil release (-O3 -flto  -Wl,-u,vTaskSwitchContext -ffast-math)
adc-board> ps
name cpu% stack_free_b prio state runtime_delta window_ms=250
IDLE 56.1 432 0 Y 1029
AnalogAcq 42.9 1612 3 B 786
SensorRtt 0.4 836 1 Y 8
defaultTask 0.2 3992 24 B 5
ShellTask 0.1 1476 1 R 3
Tmr Svc 0.0 884 2 B 0


## Parametre = Stages ADC

Paramètres de base :
-ANALOG_ACQUISITION_SEQUENCES_PER_HALF_BUFFER = 8
FilteringStage = LowPass(600) + LowPass(1000)
Workflow<
    FilteringStage,
    CaptureState<&SensorState::last_filtered_adc_value>,
    TiaCurrentConverter,
    CaptureState<&SensorState::last_current_ma>,
    AnalogSensorLinearizer,
    CaptureState<&SensorState::last_normalized_position>,
    Tap<HammerSpeedStage>,
    app::telemetry::SensorRttStreamTap
>;

### Profil debug (-Og), Full workflow

adc-board> ps
name cpu% stack_free_b prio state runtime_delta window_ms=250
AnalogAcq 50.5 1564 3 B 928
IDLE 48.1 464 0 Y 885
SensorRtt 0.5 836 1 Y 11
defaultTask 0.4 3996 24 B 8
ShellTask 0.2 1460 1 R 5
Tmr Svc 0.0 912 2 B 0


### Profil debug (-Og), Sans filtering stage (stage commenté)

adc-board> ps
name cpu% stack_free_b prio state runtime_delta window_ms=250
IDLE 70.2 464 0 Y 1287
AnalogAcq 28.3 1604 3 B 519
SensorRtt 0.5 836 1 Y 10
defaultTask 0.4 3996 24 B 9
ShellTask 0.3 1460 1 R 6
Tmr Svc 0.0 912 2 B 0

### Profil debug (-Og), avec SIGNAL_FILTERING_ENABLED = false (IdentityFilter)

adc-board> ps
name cpu% stack_free_b prio state runtime_delta window_ms=250
IDLE 68.7 464 0 Y 1259
AnalogAcq 29.7 1604 3 B 545
SensorRtt 0.7 836 1 Y 14
defaultTask 0.4 3996 24 B 8
ShellTask 0.2 1460 1 R 5
Tmr Svc 0.0 912 2 B 0


### Profil debug (-Og), avec un seul LowPass(600)

adc-board> ps
name cpu% stack_free_b prio state runtime_delta window_ms=250
IDLE 59.7 464 0 Y 1099
AnalogAcq 38.7 1580 3 B 713
SensorRtt 0.6 836 1 Y 12
defaultTask 0.4 3996 24 B 9
ShellTask 0.2 1496 1 R 5
Tmr Svc 0.0 912 2 B 0

