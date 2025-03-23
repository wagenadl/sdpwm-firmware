import serial
import time
import ppersist
import numpy as np
import matplotlib.pyplot as plt
plt.ion()

ser = serial.Serial("/dev/ttyACM1", timeout=.2)


def report(expect="", tries=0):
    for k in range(tries):
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

def command(x, tries=1):
    send(x)
    report(x.split(" ")[0], tries)

report()
send("10000")
time.sleep(0.1)
send("0")
command("sdpwm")
command("logk 4")
per = 125e6 / 44100 / 2**4
command(f"period {per}")

sine = (np.sin(np.arange(256)*2*np.pi/256) * 32000).astype(np.int16)
end = np.zeros(64, np.uint16)
end[:] = 0x8080

command("pause")
for k in range(32):
    for y in sine:
        send(f"{y}")
command("go")
command("bin")
for k in range(1000):
    n = ser.write(sine.tobytes())
    print(k, n)
n = ser.write(end.tobytes())
print("end", n)
for k in range(10):
    command("x")
   
 
