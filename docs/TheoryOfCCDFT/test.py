import struct
import numpy as np
from scipy import signal as sg
import soundfile as sf

Fs = 44100
freq = 40
chop = 200
samples = 44100

x = np.arange(samples)
y = 10000 * np.sin(2 * np.pi * freq * x / Fs)
c = sg.square(2 * np.pi * chop * x / Fs)

sf.write("chop.wav", (y * c).astype(np.int16), samplerate = Fs)

