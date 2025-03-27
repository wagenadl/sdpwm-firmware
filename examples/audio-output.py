
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
import wave
import os

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
#ser = serial.Serial("/dev/ttyACM0", timeout=.2)

#%%
command("pwm")
report()
send("3000")
time.sleep(0.1)
send("-3000")

#%%
destdir = R"../../sdpwm-audio"
#destdir = "."

with wave.open(f"{destdir}/goldberg1-original.wav") as f:
    audio = f.readframes(-1)
audio = np.frombuffer(audio, np.int16).astype(np.float32)
audio = audio - np.mean(audio)
audio = audio * 8.5 / np.max(np.abs(audio))
fs_audio = 44100
FS_HZ = 250e3

logkk = np.arange(4, 12)
modes = ["pwm", "sdm", "sdpwm"]
fprod_Hz = int(125e6 / 2048)

#%% Output audio waves
sdata = {}
dur_s = len(audio) / fs_audio
acqscans = int(dur_s * FS_HZ)

for mode in ["sdpwm", "sdm", "pwm"]:
    command(f"{mode}")
    for logk in logkk:
        ofn = destdir + f"/goldberg1-{mode}-{logk}.pkl"
        if os.path.exists(ofn):
            continue
        per = int(125e6 / fs_audio / 2**logk)
        over = 0
        while per > 15 and over < 2:
            per >>= 1
            over += 1
        print(f"{mode} logk={logk} per={per} over={over}")
        command(f"logk {logk}")
        command(f"period {per}")
        command(f"over {over}")
        send("0")
        efffrq = 125e6/2**logk/per/2**over
        T = len(audio)
        tt = np.arange(T)
        T1 = int(efffrq/44100 * T)
        N1 = int(efffrq/10)
        tt1 = np.arange(T1, dtype=float) * 44100.0/efffrq
        vv_bin = np.round(np.interp(tt1, tt, audio)*32768/10).astype(np.int16)
        end = np.zeros(96, np.uint16) + 0x8080
        end[:24] = 0
        end[24:36] = 30000
        end[36:48] = 65536 - 30000
        end[48:72] = 0

        with nidaqmx.Task() as task:
            task.ai_channels.add_ai_voltage_chan("Dev1/ai0", 
                    terminal_config=TerminalConfiguration.RSE,
                    min_val=-10, max_val=10)
            task.in_stream.input_buf_size = 30*250_000
            task.timing.cfg_samp_clk_timing(FS_HZ, sample_mode=AcquisitionType.CONTINUOUS)
            # I'd like to commit the task, but that does not appear to be supported in python
        
            t0 = time.time()
            print(f"{mode} {logk}: prewrite")
            command("pause")
            for k in range(72):
                send(f"{end[k]}")
            for v in vv_bin[:44100]:
                send(f"{int(v)}")
            dat = []
            task.start()
            time.sleep(0.1)
            command("go")
            command("bin")
            for k in range(44100, len(vv_bin), N1):
                print(f"{mode} {logk}: {time.time() - t0:.3f} {k} {len(vv_bin)}")
                dat.append(task.read(number_of_samples_per_channel=25_000))
                print(ser.write(vv_bin[k:k+N1]))
            ser.write(end)
            print("reading")
            dat.append(task.read(number_of_samples_per_channel=3*250_000))
            print("got it")
            sdata = np.concatenate(dat).astype(np.float32)
            for k in range(10):
                try:
                    command("x")
                except Exception as e:
                    print(e)
            ppersist.save(ofn, sdata)    


