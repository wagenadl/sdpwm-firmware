#%%
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

#%%
command("pwm")
report()
send("3000")
time.sleep(0.1)
send("-3000")

#%%
destdir = "."

FS_HZ = 250e3

logkk = np.arange(4, 12)
modes = ["pwm", "sdm", "sdpwm"]
fprod_Hz = int(125e6 / 2048)

#%% Sine waves
sdata = {}
freqs_Hz = [250, 500, 1000, 2000, 4000]
dur_s = 0.2 # 200 ms, i.e., about 12,000 samples at 61 kHz
acqscans = int(dur_s * FS_HZ)

for mode in modes:
    command(f"{mode}")
    for logk in logkk:
        per = 2048 >> logk
        print(f"{mode} logk={logk} per={per}")
        command(f"logk {logk}")
        command(f"period {per}")
        key = (mode, logk)
        sdata[key] = []
        for freq in freqs_Hz:
            tt_s = np.arange(0, dur_s, 1 / fprod_Hz)
            vv_V = 9 * np.sin(2*np.pi*tt_s * freq)
            vv_bin = np.round(32768 * vv_V/10)
            with nidaqmx.Task() as task:
                task.ai_channels.add_ai_voltage_chan("Dev1/ai0", 
                        terminal_config=TerminalConfiguration.RSE,
                        min_val=-10, max_val=10)
                task.timing.cfg_samp_clk_timing(FS_HZ,
                        sample_mode=AcquisitionType.CONTINUOUS)
                # I'd like to commit the task,
                # but that does not appear to be supported in python
                send("0")
                command("pause")
                for v in vv_bin:
                    send(f"{int(v)}")
                send("0")
                command("go")
                task.start()
                dat = task.read(number_of_samples_per_channel=acqscans)
                sdata[key].append(np.array(dat, dtype=np.float32))
                
ofn = f"{destdir}/sine.pkl"
ppersist.save(ofn, modes, logk, freqs_Hz, sdata)


