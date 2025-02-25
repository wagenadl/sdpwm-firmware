import serial
import time
import ppersist
import numpy as np
import matplotlib.pyplot as plt
import daw.peakx
import scipy.signal

plt.ion()

pars = ppersist.load("/home/wagenaar/cntc/jobs/internal/250224-sdpwm/nidaqtest250224-vv.pkl")
meta = ppersist.load("/home/wagenaar/cntc/jobs/internal/250224-sdpwm/acquire250224meta.pkl", trusted=True)
with open("/home/wagenaar/cntc/jobs/internal/250224-sdpwm/acquire250224.raw", "rb") as fd:
    bts = fd.read()
    data = np.frombuffer(bts, dtype=np.float32).reshape(meta.siz)


dat1 = data.reshape(-1, 100, 250).mean(-1)

plt.figure(1)
plt.clf()
plt.imshow(dat1.T, aspect='auto')

#%%
dat1 = data.flatten()
dy = dat1[:-250] - dat1[250:]
b, a = scipy.signal.butter(1, .02)
z = scipy.signal.filtfilt(b, a, dy)

ion, iof = daw.peakx.schmitt(z, 5, 0)

dion = np.diff(ion)
di0 = np.median(dion)
idion = np.round(dion / di0).astype(int)

plt.figure(4)
plt.clf()
plt.plot(ion[1:], (dion / di0), '.')
plt.plot(ion[1:], np.round(dion / di0).astype(int), '.')

ionrec = [ion[0]]
for delta, idelta in zip(dion, idion):
    i0 = ionrec[-1]
    for k in range(idelta):
        ionrec.append(int(i0 + (k+1)*delta/idelta))

avg = []
sd = []
for i in ionrec:
    xx = dat1[i + 250*10 : i + 250*95]
    avg.append(np.mean(xx))
    sd.append(np.std(xx))
    
plt.figure(1)
plt.clf()
plt.plot(avg, '.')
