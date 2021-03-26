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

    def sum(self, signal):
        for (batch, rhs_batch) in zip(self.data, signal.data):
            batch += rhs_batch

    def populate_samples(self, f):
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

    def fft(self, max_freq):
        full_data = []
        for d in self.data:
            full_data.extend(d.tolist())

        fft_wave = np.fft.rfft(full_data)
        fft_freq = np.fft.rfftfreq(len(full_data), d=1.0/SampleRate)

        plt.plot(fft_freq[:max_freq], np.array(fft_wave)[:max_freq])

    def write(self, name):
        full_data = []
        for d in self.data:
            full_data.extend(d.tolist())

        f = wave.open(name, 'wb')
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(SampleRate)
        f.setnframes(len(full_data))

        for d in full_data:
            value = int((2 ** 15 - 1) * max(min(d, 1.0), -1.0))
            data = struct.pack('<h', value)
            f.writeframesraw(data)

        f.close()

def intermediate(fs, f0, db_gain, s):
    A = 10. ** (db_gain / 40.)
    w = 2 * np.pi * f0 / fs
    alpha = 0.5 * np.sin(w) * np.sqrt((A + 1 / A) * (1 / s - s) + 2)
    print (f"Q: {1 / np.sqrt((A + 1 / A) * (1 / s - s) + 2)}")
    return w, alpha

# https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html for the coeff calculations
class BiQuadFilter(object):
    def __init__(self, coeff):
        print (f"BiQuadFilter Coefficients: {coeff}")
        self.coeff = coeff

    @staticmethod
    def lpf(fs, f0, db_gain, s):
        print (f"{f0}, {db_gain}, {s}")
        w, alpha = intermediate(fs, f0, db_gain, s)
        b0 = (1 - np.cos(w)) * 0.5
        b1 = 2 * b0
        b2 = b0
        a0 = 1 + alpha
        a1 = -2 * np.cos(w)
        a2 = 1 - alpha
        return BiQuadFilter([b0 / a0, b1 / a0, b2 / a0, a0 / a0, a1 / a0, a2 / a0])

    @staticmethod
    def hpf(fs, f0, db_gain, s):
        print (f"{f0}, {db_gain}, {s}")
        w, alpha = intermediate(fs, f0, db_gain, s)
        b0 = (1 + np.cos(w)) * 0.5
        b1 = -2 * b0
        b2 = b0
        a0 = 1 + alpha
        a1 = -2 * np.cos(w)
        a2 = 1 - alpha
        return BiQuadFilter([b0 / a0, b1 / a0, b2 / a0, a0 / a0, a1 / a0, a2 / a0])

    def process(self, signal):
        xn_1 = 0
        xn_2 = 0
        yn_1 = 0
        yn_2 = 0

        b0, b1, b2, a0, a1, a2 = np.copy(self.coeff)

        output = Signal()
        for batch in signal.data:
            output_batch = []
            for sample in batch:
                xn = sample
                yn = b0 * xn + b1 * xn_1 + b2 * xn_2 - a1 * yn_1 - a2 * yn_2
                print (f"xn: {xn}, yn: {yn}")

                output_batch.append(yn)

                xn_2 = xn_1
                xn_1 = xn

                yn_2 = yn_1
                yn_1 = yn

            output.data.append(np.array(output_batch))

        return output

s = Signal()

t = np.linspace(0.0, 1.0, SampleRate)[:5]
data = np.cos(2 * np.pi * 1000 * t)
#for f in range(100, 2000, 10):
#    data += np.cos(2 * np.pi * f * t)

s.data.append(data)

f = BiQuadFilter.hpf(SampleRate, 2000, 3, 1.0)
s.fft(2000)
s = f.process(s)

# s.write("/tmp/out.wav")
# s.fft(2000)
# plt.show()

