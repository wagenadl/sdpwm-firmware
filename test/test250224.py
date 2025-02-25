import serial
import time
import ppersist
import numpy as np
import matplotlib.pyplot as plt
plt.ion()

ser = serial.Serial("/dev/ttyACM1", timeout=.2)


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

report()
send("10000")
time.sleep(0.1)
send("0")

#%%
vv = []
v = 0
for i in range(100):
    vv.append(v)
    v += 4577
    if v > 32767:
        v -= 2*32767

vv = np.floor(np.random.random(200) * 65535 - 32767).astype(int)

plt.figure(1)
plt.clf()
plt.plot(vv, '.')

plt.figure(2)
plt.clf()
plt.hist(vv, bins=np.arange(-32768, 32768, 64));
#%%
logkk = np.arange(4, 12)
modes = ["pwm", "sdm", "sdpwm"]

ppersist.save("/home/wagenaar/Desktop/nidaqtest250224-vv.pkl", vv, logkk, modes)

for mode in modes:
    command(f"{mode}")
    for logk in logkk:
        per = 2048>>logk
        print(f"{mode} logk={logk} per={per}")
        command(f"logk {logk}")
        command(f"period {per}")
        for v in vv:
            send("32767")
            time.sleep(0.001)
            send("-32767")
            time.sleep(0.001)
            send(f"{v}")
            time.sleep(0.1)
        send("0")
        time.sleep(0.2)
    time.sleep(0.2)
    
