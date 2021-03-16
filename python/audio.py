import numpy as np
import matplotlib.pyplot as plt
import wave
import struct

SampleRate = 44000
SampleTime = 1 / SampleRate
BatchSize = 128
BatchTime = BatchSize / SampleRate

FadeBatchSize = 200
FadeBatchTime = FadeBatchSize * BatchTime

class Signal(object):
    def __init__(self):
        self.data = []
        self.phase = 0

    def update_samples(self, f):
        phase_inc = 2 * np.pi * f / SampleRate
        x = np.arange(0, BatchSize, phase_inc)
        self.data.append(np.sin(self.phase + x))
        print (f"phase: {self.phase}, inc: {phase_inc}, final: {x[-1] + phase_inc}")
        self.phase += x[-1] + phase_inc

    def plot(self):
        full_data = []
        for d in self.data:
            full_data.extend(d.tolist())

        x = np.arange(0, len(full_data)) / SampleTime

        plt.plot(x, np.array(full_data))
        plt.show()

    def write(self, name):
        full_data = []
        for d in self.data:
            full_data.extend(d.tolist())

        f = wave.open("/tmp/test.wav", 'wb')
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(SampleRate)
        f.setnframes(len(full_data))

        for d in full_data:
            value = int((2 ** 15 - 1) * max(min(d, 1.0), -1.0))
            data = struct.pack('<h', value)
            f.writeframesraw(data)

        f.close()


s = Signal()

for i in range(10):
    s.update_samples(440)

for i in range(10):
    s.update_samples(1000)

# for i in range(200):
#     s.update_samples(1000, (300 + i) * BatchTime)
# for i in range(100):
#     s.update_samples(5500, (400 + i) * BatchTime)
s.write("/tmp/out.wav")
s.plot()

# for i in range(10):
#     s.update_samples(500, (10 + i) * BatchTime)

#s.plot()
