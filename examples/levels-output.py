#%%

import matplotlib.pyplot as plt
plt.ion()
import nidaqmx
import numpy as np
from nidaqmx.constants import AcquisitionType, TerminalConfiguration
import ppersist
import time
import serial
import time

#%%
def report(expect=""):
    while True:
        x = ser.readline()
        if x:
            print(x)
        else:
            break
        if expect:
            if bytes(expect, "utf8") in x:
                return
    if expect:
        raise Exception("Expected string not found")

def send(x):
    ser.write(bytes(x + "\n", "utf8"))

def command(x):
    send(x)
    report(x.split(" ")[0])

#%%
ser = serial.Serial("COM3", timeout=.2)

report()
send("10000")
time.sleep(0.1)
send("0")

#%%
destdir = "."

NVOLTAGES = 5000 # Number of voltages to use
ACQDUR_S = 0.1
FS_HZ = 250e3
ACQSCANS = int(ACQDUR_S * FS_HZ)

vv = np.floor(np.random.random(NVOLTAGES) * 65000 - 32500).astype(int)
logkk = np.arange(4, 12)
modes = ["pwm", "sdm", "sdpwm"]

#%%
plt.figure(1)
plt.clf()
plt.plot(vv, '.')

plt.figure(2)
plt.clf()
plt.hist(vv, bins=np.arange(-32768, 32768, 64));

#%% DC levels

means = {}
sds = {}
for mode in modes:
    command(f"{mode}")
    for logk in logkk:
        per = 2048 >> logk
        print(f"{mode} logk={logk} per={per}")
        command(f"logk {logk}")
        command(f"period {per}")
        mm = []
        ss = []
        for v in vv:
            send(f"{v}")            
            with nidaqmx.Task() as task:
                task.ai_channels.add_ai_voltage_chan("Dev1/ai0", 
                        terminal_config=TerminalConfiguration.RSE,
                        min_val=-10, max_val=10)
                task.timing.cfg_samp_clk_timing(FS_HZ,
                        sample_mode=AcquisitionType.CONTINUOUS)
                task.start()
                dat = task.read(number_of_samples_per_channel=ACQSCANS)
                print(mode, logk, v, np.mean(dat), np.std(dat))
                d1 = dat[5000:-1000] # skip 20 ms at start and 4 at end
                mm.append(np.mean(d1))
                ss.append(np.std(d1))
        means[mode, logk] = np.array(mm)
        sds[mode, logk] = np.array(ss)
        ofn = f"{destdir}/levels.pkl"
        ppersist.save(ofn, modes, logkk, vv, means, sds)
